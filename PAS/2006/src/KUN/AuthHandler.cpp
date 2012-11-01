#include "AuthHandler.h"
#include "AuthAgent.h"
#include "Mutex.h"
#include "Config.h"

#include <ace/SOCK_Connector.h>

int errCount = 0;
long errTime = 0;

AuthHandler::AuthHandler(ACE_Reactor* pReactor) : PasHandler( pReactor, HT_AuthHandler )
{
	init();
}

void AuthHandler::init()
{
	puserInfo = NULL;

	/// ���� �ʱ���� ����
	/// Auth ������ ������� ���� ���� ���� �Ϸ�� ���·� ����
	/// �̷��� �ϴ� ������, isRemovable ���� false �� ���ϵǹǷ� ��ü�� �������� �ʴ´�.
	Config *pConf = Config::instance();
	if( pConf->auth.enable )
		authState = AS_NONE;
	else
	{
		authState = AS_RESPONSED;
		setJobDone();
	}
}

void AuthHandler::set(UserInfo *userInfo)
{
	puserInfo = userInfo;

	/// ���� �ʱ���� ����
	/// Auth ������ ������� ���� ���� ���� �Ϸ�� ���·� ����
	/// �̷��� �ϴ� ������, isRemovable ���� false �� ���ϵǹǷ� ��ü�� �������� �ʴ´�.
	Config *pConf = Config::instance();
	if( pConf->auth.enable == false )
	{
		puserInfo->changeAuthState( AS_RESPONSED );
		setJobDone();
	}
}


AuthHandler::~AuthHandler(void)
{
	PAS_TRACE1("Auth dtor [%X]", this);
	PHTR_DEBUG("Auth dtor");

	// Auth �� ������� �ʴ� ��� ���⼭ ���� �߻�(removeEvent)
	close();
}

void AuthHandler::onReceived()
{
	PAS_TRACE1("AuthHandler::onReceived() [%X]", this);
	PHTR_DEBUG("AuthHandler::onReceived()");
	PAS_DEBUG2("Data Received %d bytes [%X]", recvBuffer.length(), this);
	PHTR_DEBUG1("Data Received %d bytes", recvBuffer.length());

	// ���⼭ responseBody �� üũ�ϴ� ������
	// responseBody �ȿ� Header �� ���ԵǾ� �ֱ� �����̴�.
	if(recvBuffer.length() >= sizeof(responseBody))
	{
		ACE_ASSERT(recvBuffer.length() == sizeof(responseBody));

		memcpy(&responseBody, recvBuffer.rd_ptr(), sizeof(responseBody));

		recvBuffer.reset();

		onCompletedReceiveResponse();
		requestClose();
	}
}

void AuthHandler::close()
{
	PAS_TRACE1("AuthHandler::close [%X]", this);
	PHTR_DEBUG("AuthHandler::close");
	
	PAS_TRACE1("AuthHandler::close >> size of sendQueue size[%d]", sendQueue.size());

	PasHandler::close();

	setJobDone();
}

/// recv �� connection close �� ������ ��� ȣ���
void AuthHandler::onRecvFail()
{
	close();
}

/// send �� connection close �� ������ ��� ȣ���
void AuthHandler::onSendFail()
{
	responseBody.ackMin[0] = '\0';
	responseBody.status = 0;
	responseBody.type = 0;

//	responseLog("Connection Closed");

	close();
}

int AuthHandler::onCompletedReceiveResponse()
{
	PAS_DEBUG2("Response Receive Completed, fd[%d] [%X]", get_handle(), this);
	PHTR_DEBUG1("Response Receive Completed, fd[%d]", get_handle());

	switch(responseBody.status)
	{
	case PAS_AUTH_FAIL_NOT_ADDR:
		PAS_DEBUG1("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_NOT_ADDR [%X]", this);
		PHTR_DEBUG("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_NOT_ADDR");
		authState = AS_FAILED;
		break;

	case PAS_AUTH_FAIL_AT_NOT_FOUND:
		PAS_DEBUG1("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_AT_NOT_FOUND [%X]", this);
		PHTR_DEBUG("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_AT_NOT_FOUND");
		authState = AS_FAILED;
		break;

	case PAS_AUTH_FAIL_MSISDN:
		PAS_DEBUG1("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_MSISDN [%X]", this);
		PHTR_DEBUG("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_MSISDN");
		authState = AS_FAILED;
		break;

	case PAS_AUTH_FAIL_IPADDR:
		PAS_DEBUG1("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_IPADDR [%X]", this);
		PHTR_DEBUG("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_IPADDR");
		authState = AS_FAILED;
		break;

	case PAS_AUTH_FAIL_INVALID_IP_A_CLASS:
		PAS_WARNING1("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_INVALID_IP_A_CLASS [%X]", this);
		PHTR_DEBUG("AuthHandler::onCompletedReceiveResponse => PAS_AUTH_FAIL_INVALID_IP_A_CLASS");
		authState = AS_FAILED;
		break;

	default:
		PAS_WARNING2("AuthHandler::onCompletedReceiveResponse => Success Auth Code [0x%X] [%X]", responseBody.status, this);
		PHTR_DEBUG1("AuthHandler::onCompletedReceiveResponse => Success Auth Code [0x%X]", responseBody.status);
		authState = AS_RESPONSED;
		break;
	}

	stopTimeTick();

//	pRequester->onCommand(CID_Auth_Completed, this, (void*)&responseBody);

	puserInfo->changeAuthState( authState );

	setJobDone();

	// auth �α� ���Ͽ� ��� 
//	responseLog();

	return 0;
}

int AuthHandler::start(const int seq, const char* pMin, const unsigned int ip, const int port, const int startConn, const int newBrowser, int g3GCode, const char* pMdn, const char* pMsModel)
{
	ACE_ASSERT(pMin != NULL);
	ACE_ASSERT(port > 0);

	// init header
	//requestHeader.type = AUTH::REQUEST_AUTHORIZE_CODE;

	// init body
	requestBody.type = AUTH::REQUEST_AUTHORIZE_CODE;
	requestBody.seq = seq;
	STRNCPY(requestBody.min, pMin, sizeof(requestBody.min));
	requestBody.accessIP = ip;
	requestBody.port = port;
	requestBody.startConn = startConn;
	requestBody.newBrowser = newBrowser;
	requestBody.g3GCode = g3GCode;

	if(pMdn)
	{
		STRNCPY(requestBody.mdn, pMdn, sizeof(requestBody.mdn));
	}
	else
		memset(requestBody.mdn, 0x00, sizeof(requestBody.mdn));


	if(pMsModel)
	{
		STRNCPY(requestBody.msModel, pMsModel, sizeof(requestBody.msModel));
	}
	else
		memset(requestBody.msModel, 0x00, sizeof(requestBody.msModel));

	puserInfo->changeAuthState( AS_REQUESTED );

	int res = connect();
	return res;
}

int AuthHandler::connect()
{
//	ACE_ASSERT(pRequester != NULL);

	const Config* pConfig = Config::instance();
	
	const host_t& host = pConfig->auth.host;
	const int port = pConfig->auth.port;
		
	ACE_INET_Addr addr(port, (const char*)host);
	ACE_SOCK_Connector connector;
	ACE_SOCK_Stream newSock;

	PAS_DEBUG3("AuthHandler:: Try connect to Auth[%s:%d] [%X]", (const char*)host, port, this);
	PHTR_DEBUG2("AuthHandler:: Try connect to Auth[%s:%d]", (const char*)host, port);
	if(connector.connect(newSock, addr, &ACE_Time_Value::zero) < 0)
	{
		if(errno != EWOULDBLOCK)
		{
			PAS_WARNING2("AuthHandler::connect >> Fail Connect to PasAuth, fd[%d] [%X]", get_handle(), this);
			PHTR_INFO1("AuthHandler::connect >> Fail Connect to PasAuth, fd[%d]", get_handle());
//			responseLog("connection failed");
			authState = AS_FAILED;

			return -1;
		}
	}

	//newSock.enable(ACE_NONBLOCK);

	set_handle(newSock.get_handle());
	
	startTimeTick(Config::instance()->auth.timeoutCheckInterval);
	startConnectTimer();

	addEvent(CONNECT_MASK | WRITE_MASK);

	return 0;
}

int AuthHandler::request()
{
	// auth �α� ���Ͽ� ���
//	requestLog();

	//enSendQueue((const char*)&requestHeader, sizeof(requestHeader));
	enSendQueue((const char*)&requestBody, sizeof(requestBody));

	return 0;
}

void AuthHandler::onConnect()
{
	PAS_TRACE1("AuthHandler::onConnect [%X]", this);
	PHTR_DEBUG("AuthHandler::onConnect");

	stopConnectTimer();	
	request();
	startReceiveTimer();

	addEvent(READ_MASK);

	PAS_DEBUG2("AuthHandler::onConnect >> Request Auth, fd[%d] [%X]", get_handle(), this);
	PHTR_DEBUG1("AuthHandler::onConnect >> Request Auth, fd[%d] [%X]", get_handle());
}

int AuthHandler::handle_timeout(const ACE_Time_Value& current_time, const void* /* act */)
{
	PAS_TRACE2("AuthHandler::handle_timeout >> fd[%d] [%X]", get_handle(), this);
	PHTR_DEBUG1("AuthHandler::handle_timeout >> fd[%d]", get_handle());

	const AuthConfig& authConfig = Config::instance()->auth;
	ACE_Time_Value connTimeout(authConfig.connectionTimeout);
	ACE_Time_Value recvTimeout(authConfig.receiveTimeout);
	

	
	if( isConnectTimeOut(current_time, connTimeout))
	{

		PAS_DEBUG2("AuthHandler::handle_timeout >> ���ӿ�û�� ���Ӵ�� �ð� �ʰ�, fd[%d] [%X]", get_handle(), this);
		PHTR_INFO1("AuthHandler::handle_timeout >> ���ӿ�û�� ���Ӵ�� �ð� �ʰ�, fd[%d]", get_handle());
		close();
	}
	else	if( isIdle(current_time, recvTimeout) )
	{
	
		PAS_DEBUG2("AuthHandler::handle_timeout >> ������ ���Ŵ�� �ð� �ʰ�, fd[%d] [%X]", get_handle(), this);
		PHTR_INFO1("AuthHandler::handle_timeout >> ������ ���Ŵ�� �ð� �ʰ�, fd[%d]", get_handle());
		close();
	
	}
	

	return 0;
}
 
AuthState AuthHandler::getState()
{
	return puserInfo->getAuthState();
}

UserInfo *AuthHandler::getUserInfo()
{
	return puserInfo;
}

bool AuthHandler::isPassed()
{
	return ( puserInfo->getAuthState() != AS_FAILED );
}

bool AuthHandler::isRemovable()
{
	return jobDone;
}

char* AuthHandler::setMyInfo()
{
/*
���� �Ͻ�	������ ��¥/�ð�	2006-09-05 15:34
���� �ڵ�	��û ���� �ڵ�	#DIV/0!
ó������	ó������ ���ڿ�/Port	relJob/7090
Sequence ID	Sequence ID	2532228
�ܸ�������ڵ�(Debug��)	�ܸ��� �����ڵ�	i13228
��ȭ��ȣ(IMSI)	������ ��ȭ��ȣ(IMSI)	4.5E+14
���� IP	�ܸ��� ���� IP	10.112.2.22
�۾� ť ���� ����/�������	�۾� ť ���� ���� / �������	S15991/4
�۾� ť ���� ����/��������	�۾� ť ���� ����/ ��������	C15994/0
��û����	��û ����	R
Duration	�ҿ� �ð�	0.004196
*/
	if(puserInfo == NULL)
	{
		snprintf(myinfo, MYID_LEN, "AuthHandler[%X] State[%d] jobDone[%d] fd[%d]",
			this, getState(), jobDone, get_handle());
	}
	else
	{
		snprintf(myinfo, MYID_LEN, "AuthHandler[%X] State[%d] jobDone[%d] MDN[%s] fd[%d]",
			this, getState(), jobDone, puserInfo->getPhoneNumber(), get_handle());
	}

	return myinfo;
}

AUTH::RequestBody *AuthHandler::getRequestBody()
{
	return &requestBody;
}
