#include <algorithm>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include "ClientHandler.h"
#include "AuthHandler.h"
#include "SantaEventHandler.h"
#include "Common.h"
#include "HttpRequestHeader.h"
#include "HttpTypes.h"
#include "CpEventHandler.h"
#include "PasLog.h"
#include "Util.h"
#include "HashKey.h"
#include "Mutex.h"
#include "AclRouter.h"
#include "HotNumber.h"
#include "SantaTypes.h"
#include "Config.h"
#include "Util2.h"
#include "PhoneTraceMng.h"
#include "ClientRemover.h"
#include "StatFilterMng.h"
#include "HttpKTFSSLResult.h"
#include "KtfInfo.h"
#include "AuthAgent.h"
#include "ReactorInfo.h"
#include "SisorProxy.h"
#include "SisorQueue.h"
#include "Exception.h"


#include "MonitorReporter.h"
#include "WatchReporter.h"

#define DEFAULT_MULTI_PROXY "KUN00"

extern int errno;

using namespace std;

//const int MaxHashKeySize = 20;
//const int MaxBillInfoKeySize = 12;

unsigned int ClientHandler::numTransactionAlloc = 0;
unsigned int ClientHandler::numClientHandlers = 0;
unsigned int ClientHandler::numRequests = 0;
unsigned int ClientHandler::numResponses = 0;

unsigned int ClientHandler::sentDirectCnt = 0;
unsigned int ClientHandler::sentByQueCnt = 0;
	
ClientHandler::ClientHandler(ReactorInfo* rInfo)
: PasHandler(rInfo->pReactor, HT_ClientHandler), 
	santaHandler(rInfo->pReactor, this), 
	authHandler(rInfo->pReactor)
{
	myinfo[0] = 0;

	closeFuncExecuted = false;
	everRecvPacket = false;
	
	myReactor = rInfo;

	_pCP = NULL;
}

// construct ���Ŀ� �����ϱ� ����.
void ClientHandler::init(int sockfd)
{
	myReactor->numClients++;
	numClientHandlers++;
	
	sock.set_handle(sockfd);

	// set SO_LINGER
/*	linger lingtime;
	lingtime.l_onoff = 1;
	lingtime.l_linger = 5;
	sock.set_option( SOL_SOCKET, SO_LINGER, (void*)&lingtime, sizeof(linger) );
*/
	startSession();
	
	changeState(CS_WAIT_REQUEST_HEADER);

	userInfo = NULL;
	currTransaction =  NULL;
	currHttpRequest = NULL;

	connectedFlag = true;
	
	startTimeTick(Config::instance()->client.timeoutCheckInterval);

	// 2006.12.19 �����ϰ� �ٷ� ���� ���.
	startReceiveTimer(); // �ð����� �����ϴ� ��.
	
	paslog =  PasDataLog::instance();
	// 2006.12.19 ȭ�� ������ �ʹ� ������ �ƴ��� �ǽ�. reopen ���� ����.
	//paslog->openFiles(true);  // Ȥ�� �α� ������ �����ǰų�  �� ��� ������ reopen (flag== true)

	setMyInfo();
	
	#ifdef HTTP_LOG
	httplog = new MyLog();
	httplog->open((char*)"./", (char*)"httpsize");
	#endif

	#ifdef HTTP_DUMP
	filedump = new FileDump();
	#endif

	ACE_ASSERT(myReactor->pReactor == reactor());

}

ClientHandler::~ClientHandler(void)
{
// !!! do NOTHING -- because this objected is deleted in itself.
}

/**
�ܸ��� �ּ�, ���� socket ���� sesssion ������ ����.
*/
void ClientHandler::startSession()
{
	
	ACE_INET_Addr peer;
	sock.get_remote_addr( peer );

	session.start(peer.get_host_addr(), peer.get_port_number(), get_handle());
	session.setLastTransactionTime();  // IDR �α׸� ���� ��  (���� ���� �α� )
	PHTR_DEBUG3("Session - Client from %s:%d", peer.get_host_addr(), peer.get_port_number(), get_handle());
}

void ClientHandler::finishSession()
{
	session.finish();	
}

void ClientHandler::changeState(ClientState _state)
{
	PAS_TRACE3("ChangeState from %d to %d ; %s", state, _state, session.getNumber());
	PHTR_DEBUG3("ChangeState from %d to %d ; %s", state, _state, session.getNumber());
	state = _state;
}

void ClientHandler::close()
{
	PasHandler::close();

	if(Config::instance()->sessionInfoUpdate.enable)
		updateSessionInfo();

	//_cp.close();
	if(_pCP != NULL)
	{
		delete _pCP;
		_pCP = NULL;
	}

	writeAbnormalCloseLog();

	deleteSendQueue();
	
	if (closeFuncExecuted)
	{
		PAS_DEBUG1("Close function called more then once. %s", getMyInfo());	
		return;
	}

	closeFuncExecuted = true;
	
	PAS_DEBUG1("Close session. %s", getMyInfo());
	PHTR_DEBUG("ClientHandler::close >> Close session.");

	finishSession();
	
	if (userInfo)
		userInfo->onClientClose();

	if(!transactionQueue.empty())
	{
		PAS_DEBUG2("Still %d transaction left in queue. %s", transactionQueue.size(), getMyInfo());
		PHTR_DEBUG2("Still %d transaction left in queue. %s", transactionQueue.size(), getMyInfo());
	}
	
	while(!transactionQueue.empty())
	{
		freeTransaction(FREE_FRONT, NULL);
	}

	if (myReactor->numClients > 0)
		myReactor->numClients--;

	if (numClientHandlers > 0)
		numClientHandlers--;

	#ifdef HTTP_LOG
	if (httplog)
		delete httplog;
	#endif 
	
	#ifdef HTTP_DUMP
	if( filedump )
		delete filedump;
	#endif

	PAS_INFO1("Phone Closed. %s", getMyInfo());
	
	if (this->tracelog != NULL)
	{
		delete this->tracelog;
		this->tracelog = NULL;
	}
	delete this;

	return;
}

void ClientHandler::onCloseByPeer()
{
	pSysStat->clientCloseByPeer(1);
	close();
}

void ClientHandler::onRecvFail()
{
	if(isConnected())
		pSysStat->clientCloseByHost(1);
	close();
}

/// send �߿� connection close �� ������ ��� ȣ���
void ClientHandler::onSendFail()
{
	PAS_INFO1("Can't send to Phone. %s", getMyInfo());
	// socket close ���� (fd == -1) �� ����.
	// send fail ó������ ����..  TB solaris 8 ó�� ���� �ý��ۿ��� ���� �߻� . 2006.11.01  -- handol
	//sock.close();
}

void ClientHandler::onReceived()
{
	PAS_TRACE2("ClientHandler::onReceived >> Received %d bytes, fd[%d]", recvBuffer.length(), get_handle());
	PHTR_DEBUG1("ClientHandler::onReceived >> Received %d bytes", recvBuffer.length());

	ReactorBusyGuard reactorBusyGuard(myReactor);

	everRecvPacket = true;
	
	startReceiveTimer(); // �ð����� �����ϴ� ��.
	
	if (get_handle() < 0)
	{
		PAS_NOTICE1("ClientHandler::onReceived - SOCK closed - %s", getMyInfo());
		if(isConnected())
			pSysStat->clientCloseByHost(1);
		close();
		return;
	}
	
	ACE_ASSERT(recvBuffer.length() > 0);
	

	int	result = 0;
	while(true)
	{
		// �۾� �� ���� ������
		const size_t oldRecvLength = recvBuffer.length();

		switch(state)
		{
		case CS_WAIT_REQUEST_HEADER:
			#ifdef HTTP_DUMP
			filedump->init("PHONE-REQU-HEAD", 1);
			filedump->before(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			result = onReceivedRequestHeader();

			#ifdef HTTP_DUMP
			filedump->after(recvBuffer.length());
			#endif
			break;

		case CS_WAIT_REQUEST_BODY:
			#ifdef HTTP_DUMP
			filedump->init("PHONE-REQU-BODY", 1);
			filedump->before(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			result = onReceivedRequestBody();

			#ifdef HTTP_DUMP
			filedump->after(recvBuffer.length());
			#endif
			break;

		case CS_SSL_CONNECTING:
			 /// SSL ������ ���� ���̸� ���� ��� 
			PAS_INFO1("ClientHandler::onReceived() >> new packet while SSL connecting. Number[%s]", session.getNumber());
			return;
		case CS_SANTA_WAITING:
			PAS_INFO1("ClientHandler::onReceived() >> new packet while waiting SANTA result. Number[%s]", session.getNumber());
			return;	

		default:
			// @todo ���� �ʿ�.  �̷� ��� �߻��ϸ� �ȵȴ�.
			PAS_ERROR2("ClientHandler::onReceived() >> strange state=[%d] Number[%s] ", state, session.getNumber());
			recvBuffer.reset();
			changeState (CS_WAIT_REQUEST_HEADER );
			return;
		}

		if (result < 0)
		{
			// 2006.12.16
			PAS_NOTICE1("ClientHandler::onReceived >> CLOSE PHONE because of some error - %s", getMyInfo());
			if(isConnected())
				pSysStat->clientCloseByHost(1);
			close();
			return;
		}
		
		/// ��û Herder/Body�� ���� �Ϸ�
		if(state == CS_RECEIVED_REQUEST)
		{
			result = onCompletedReceiveRequest();
		}

		if (result < 0)
		{
			// 2006.12.16
			PAS_INFO1("ClientHandler:: CLOSE PHONE because of some error - ", getMyInfo());
			if(isConnected())
				pSysStat->clientCloseByHost(1);
			close();
			return;
		}
		
		// �۾� �� ���� ������
		const size_t newRecvLength = recvBuffer.length();

		// ó���� �����Ͱ� ���ٸ�
		bool noDataProcessed = (oldRecvLength == newRecvLength);
		if( noDataProcessed || recvBuffer.length() == 0 )
			break;
	}

	// ���� ���� ����
	// recvBuffer.reset() ���� �ʴ°�? --> �ϸ� �ȵô�. �� ó���� recvBuffer�� crunch()�ϸ� reset()�� ���� ȿ��. 2006.09.22
	recvBuffer.crunch();

	// ��� �߻� -- ��� ���ŵǴ� recvBuff �� consume(read) ���� �ʾƼ� ���۰� full �� ���. 
	if(recvBuffer.space() == 0)
	{
		//ACE_ASSERT(false);
		PAS_NOTICE1("recvBuff full. %s", getMyInfo());
		// ���� ���� �ʱ�ȭ
		changeState (CS_WAIT_REQUEST_HEADER);
		recvBuffer.reset();
	}
}

/**
Ȥ�� �ܸ����� �������� �߸� ������ ��쿡,
��Ŷ�� �Ծ� ���ֱ� ����.
*/
int	ClientHandler::consumeRecvBuffer()
{
	char *ptr = recvBuffer.rd_ptr();
	int	i=0;
	int	end = MIN(4, recvBuffer.length() );
	while(i < end)
	{
		if (*ptr != '\r' && *ptr != '\n')
			break;
		ptr++;
		i++;
	}

	if (i>0) 
	{
		recvBuffer.rd_ptr(i);
		
		PAS_NOTICE2("WRONG HTTP MESG: %s, cosumed %d bytes of wrong http", 
			session.getNumber(), i);
		PHTR_NOTICE2("WRONG HTTP MESG: %s, cosumed %d bytes of wrong http", 
			session.getNumber(), i);
		
	}
	return i;
}

/**
HTTP mesg ���� �˻�.
*/
bool ClientHandler::isHttpHeaderCandidate(char *buff, int size)
{
	if (size < 3)
		return false;

	if ((buff[0] & 0x80) == 0 && isalpha(buff[0])	
		&& (buff[1] & 0x80) == 0 && isalpha(buff[1])	
		&& (buff[2] & 0x80) == 0 && isalpha(buff[2])	
	)
		return true;
	else
		return false;
}

int ClientHandler::onReceivedRequestHeader()
{
	bool trAllocated = false;
	
	// ���ŵ� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return 0;

	int	orgRecvLeng = recvBuffer.length();

	HTTP::RequestHeader httpRequestHeader;

	HTTP::header_t header;
	int findHeader = 0;

	if (isHttpHeaderCandidate(recvBuffer.rd_ptr(), recvBuffer.length()))
	{
		findHeader = httpRequestHeader.getHeader(&header, recvBuffer.rd_ptr(), recvBuffer.length());
		PAS_TRACE1("Request header from phone.\n%s", header.c_str());
	}
	else
	{
		findHeader = -1;

		if (session.isSSL() == false)
		{
			// �̻��� ��Ŷ�� �ܸ����� �ö�� ����̴�.
			
			PAS_NOTICE3("ClientHandler::onReceivedRequestHeader >> Strange Header from PHONE: Non Alphabet - %s Buffer=%d Qsize=%d", 
					getMyInfo(), recvBuffer.length(), transactionQueue.size());
			PHTR_NOTICE3("ClientHandler::onReceivedRequestHeader >> Strange Header from PHONE: Non Alphabet - %s Buffer=%d Qsize=%d",
					getMyInfo(), recvBuffer.length(), transactionQueue.size());
			PAS_NOTICE_DUMP("Strange Header from PHONE: Non Alphabet", recvBuffer.rd_ptr(), recvBuffer.length());

			// ���ۿ� ���ŵ� ��� ������ ��������. 2006.12.16
			recvBuffer.rd_ptr( recvBuffer.length() );
			
			if (transactionQueue.size() == 0)
			{
				// �̻��ϰ� ������ �ܸ��� �߶� ������. �� ���� ó�� ���� transaction �� ���� ��쿡��. 2006.12.16
				PAS_INFO1("CLOSE PHONE. Because of strange header. %s", getMyInfo());
				return -1;
			}
			else
			{
				printRecentTransaction();
				return 0;
			}
		}
	}

	PAS_TRACE3("Request received from client. MesgLen[%d] FindHeader[%d] isSSL[%d]", 
		recvBuffer.length(), findHeader, session.isSSL());
	PHTR_DEBUG3("Request received from client. MesgLen[%d] FindHeader[%d] isSSL[%d]", 
		recvBuffer.length(), findHeader, session.isSSL());

	int setHeader = 0;
	if(session.isSSL())
	{
		if (findHeader < 0)
		{
			onReceivedSSLData(currTransaction);
			return 0;
		}	

		// advance read pointer
		if (static_cast<int>(header.size()) <= orgRecvLeng)
		{
			recvBuffer.rd_ptr(header.size());
		}
		else
		{
			recvBuffer.rd_ptr(orgRecvLeng);
			PAS_INFO4("ClientHandler:: Odd Length Buffer=%d header=%d Qsize=%d %s", 
				orgRecvLeng, header.size(), transactionQueue.size(), getMyInfo());
			PHTR_INFO4("ClientHandler:: Odd Length Buffer=%d header=%d Qsize=%d %s", 
				orgRecvLeng, header.size(), transactionQueue.size(), getMyInfo());
		}
		

		PAS_DEBUG4("ClientHandler::after header parsing: Buffer=%d header=%d Qsize=%d %s", 
				recvBuffer.length(), header.size(), transactionQueue.size(), getMyInfo());
		PHTR_DEBUG4("ClientHandler::after header parsing: Buffer=%d header=%d Qsize=%d %s", 
				recvBuffer.length(), header.size(), transactionQueue.size(), getMyInfo());
		
		//�� Transaction �� ���õɷ��� �Ѵ�.
		// ����  Transaction �� �Ҵ�޾�  currTransaction,  currHttpRequest �� �����Ѵ�.
		if (currTransaction==NULL)
		{
			trAllocated = true;
			allocTransaction();
			if (currTransaction==NULL)
			{
				PAS_ERROR1("ClientHandler::onReceivedRequestHeader >> Memory alloc failed: Transaction - %s", setMyInfo());
				return -1;
			}
		}
		
		setHeader = currHttpRequest->setHeader(header);
		if(setHeader < 0)
		{
			PAS_INFO4("ClientHandler::onReceivedRequestHeader SSL >> Parsing Fail. %s Buffer=%d header=%d Qsize=%d", 
					getMyInfo(), recvBuffer.length(), header.size(), transactionQueue.size());
			PHTR_INFO4("ClientHandler::onReceivedRequestHeader SSL >> Parsing Fail. %s Buffer=%d header=%d Qsize=%d",
					getMyInfo(), recvBuffer.length(), header.size(), transactionQueue.size());
			PAS_INFO_DUMP("Strange Header from PHONE: SSL", header.toStr(), header.size());

			if (trAllocated)
				freeTransaction(FREE_BACK, currTransaction);

			if (transactionQueue.size() == 0)
			{
				// �̻��ϰ� ������ �ܸ��� �߶� ������. �� ���� ó�� ���� transaction �� ���� ��쿡��. 2006.12.16
				PAS_INFO1("ClientHandler:: CLOSE PHONE because of Strange Header", getMyInfo());
				return -1;
			}
			else
			{
				printRecentTransaction();
				return 0;
			}
		}
		else
		{
			HTTP::RequestMethod reqMethod = currHttpRequest->getHeader()->getMethod();

			// SSL ���� ��û
			if(reqMethod == HTTP::RM_CONNECT)
			{
				//PAS_DEBUG_DUMP("SSL CONNECT", header.toStr(), header.size());
				startSSL(currTransaction);

				// �Ƹ��� free �ϴ°� ����. �׷��� �׽�Ʈ �ʿ�. ���⼭ free ���� �ʾƵ�  close() ���� �ϰ� ó��. 2006.12.15
				// freeTransaction(FREE_BACK, currTransaction);
				return 0;
			}
			else if(reqMethod == HTTP::RM_RESULT)
			{
				//PAS_DEBUG_DUMP("SSL RESULT", header.toStr(), header.size());
				finishSSL(currTransaction, true);
				
				if (trAllocated)
					freeTransaction(FREE_BACK, currTransaction);
				return 0;
			}
			else		
			{
				// RESULT ���� �Ϲ����� request �� ���� ��� �̴�.
				finishSSL(currTransaction);
				session.endSSL(); // recursive �� ���ϱ� ����.
				// ** return �ϸ� �ȵȴ�.  �Ʒ��� �����ؾ� �Ѵ�.
			}
		}
	}

	
	/* SSL �ƴ� �Ϲ� ��� */	
	{
		if (findHeader < 0)
		{
			// ��� �κ��� ��Ŷ�� �и��Ǿ� ���ŵǴ� ����̴�.  ������ �ѹ� �� �ؾ� �Ѵ�.
			PAS_DEBUG1("ClientHandler::waiting more packet to complete Http Header. curr leng=%d", recvBuffer.length());
			PHTR_DEBUG1("ClientHandler::waiting more packet to complete Http Header. curr leng=%d", recvBuffer.length());
			return 0;
		}

		PAS_TRACE2("ClientHandler:: findHeader=%d, header.size()=%d", findHeader, header.size());
		PHTR_DEBUG2("ClientHandler:: findHeader=%d, header.size()=%d", findHeader, header.size());
		
		// advance read pointer
		if (static_cast<int>(header.size()) <= orgRecvLeng)
		{
			recvBuffer.rd_ptr(header.size());
		}
		else
		{
			recvBuffer.rd_ptr(orgRecvLeng);
			PAS_INFO4("ClientHandler:: Odd Length Buffer=%d header=%d Qsize=%d %s", 
				orgRecvLeng, header.size(), transactionQueue.size(), getMyInfo());
			PHTR_INFO4("ClientHandler:: Odd Length Buffer=%d header=%d Qsize=%d %s", 
				orgRecvLeng, header.size(), transactionQueue.size(), getMyInfo());
		}

		PAS_TRACE4("ClientHandler::after header parsing: Buffer=%d header=%d Qsize=%d %s", 
				recvBuffer.length(), header.size(), transactionQueue.size(), getMyInfo());
		PHTR_DEBUG4("ClientHandler::after header parsing: Buffer=%d header=%d Qsize=%d %s", 
				recvBuffer.length(), header.size(), transactionQueue.size(), getMyInfo());
				
		// �� Transaction �� ���õɷ��� �Ѵ�.
		// ���� Transaction �� �Ҵ�޾�  currTransaction,  currHttpRequest �� �����Ѵ�.
		if (currTransaction==NULL)
		{
			allocTransaction();
			trAllocated = true;
			if (currTransaction==NULL)
			{
				PAS_ERROR1("ClientHandler::onReceivedRequestHeader >> Memory alloc failed: Transaction - %s", setMyInfo());
				return -1;
			}
		}

	
		setHeader = currHttpRequest->setHeader(header);
		
		if(setHeader < 0)
		{
			PAS_INFO4("ClientHandler:: Strange Header from PHONE: Not ssl - %s Buffer=%d header=%d Qsize=%d", 
					getMyInfo(), recvBuffer.length(), header.size(), transactionQueue.size());
			PHTR_INFO4("ClientHandler:: Strange Header from PHONE: Not ssl - %s Buffer=%d header=%d Qsize=%d",
					getMyInfo(), recvBuffer.length(), header.size(), transactionQueue.size());
			PAS_INFO_DUMP("Strange Header from PHONE: Not ssl", header.toStr(), header.size());

			if (trAllocated)
				freeTransaction(FREE_BACK, currTransaction);

			if (transactionQueue.size() == 0)
			{
				// �̻��ϰ� ������ �ܸ��� �߶� ������. �� ���� ó�� ���� transaction �� ���� ��쿡��. 2006.12.16
				PAS_INFO1("ClientHandler:: CLOSE PHONE because of Strange Header", getMyInfo());
				return -1;
			}
			else
			{
				printRecentTransaction();
				return 0;
			}
			
		}

		else if(currHttpRequest->getHeader()->getMethod() == HTTP::RM_CONNECT)
		{
			PAS_INFO_DUMP("SSL CONNECT", header.toStr(), header.size());
			startSSL(currTransaction);
			// �Ƹ��� free �ϴ°� ����. �׷��� �׽�Ʈ �ʿ�. ���⼭ free ���� �ʾƵ�  close() ���� �ϰ� ó��. 2006.12.15
			// freeTransaction(FREE_BACK, currTransaction);
			return 0;
		}
		else if(currHttpRequest->getHeader()->getMethod() == HTTP::RM_RESULT)
		{
			// just ignore
			PAS_INFO1("ClientHandler::RECV SSL RESULT NOT in SSL mode - %s", getMyInfo());
			PHTR_INFO1("ClientHandler::RECV SSL RESULT NOT in SSL mode - %s", getMyInfo());
			PAS_INFO_DUMP("SSL RESULT", header.toStr(), header.size());
			
			if (trAllocated)
				freeTransaction(FREE_BACK, currTransaction);
			return 0;
		}

		// �Ϲ� HTTP ������ ��û		
		else
		{			
			//PAS_DEBUG_DUMP("PHONE REQ HEAD", currHttpRequest->getRawHeader()->rd_ptr(), currHttpRequest->getHeadLeng());

			// content �� 10�ް��� �Ѿ��, �α׸� ������
			if(currHttpRequest->getContentLength() >= 1024*1024)
			{
				PAS_INFO1("Big content is requested. URL is [%s]", currHttpRequest->getHeader()->getUrl().toStr());
			}
			
			// has body?
			if(currHttpRequest->getContentLength() > 0)
			{
				PAS_DEBUG2("ClientHandler::RECV REQ HEAD - Head=%d, ContentLength=%d", header.size(), currHttpRequest->getContentLength());
				PHTR_DEBUG2("ClientHandler::RECV REQ HEAD - Head=%d, ContentLength=%d", header.size(), currHttpRequest->getContentLength());

				changeState ( CS_WAIT_REQUEST_BODY);
			}
			else
				changeState ( CS_RECEIVED_REQUEST);
		}
	}

	return 0;
	
}


bool ClientHandler::isCompletedReceiveRequestBody()
{
	return (static_cast<int>(currHttpRequest->getContentLength()) == currHttpRequest->getBodyLeng());
}

int ClientHandler::onReceivedRequestBody()
{
	static int appendCount = 0; //  currHttpRequest->appendBody() �� �󸶳� ȣ��Ǵ��� ���� ����. �α׿�.
	
	ACE_ASSERT(currHttpRequest != NULL);
	ACE_ASSERT(currHttpRequest->getContentLength() > 0);
	ACE_ASSERT(state == CS_WAIT_REQUEST_BODY);

	// ���ŵ� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return 0;

	// copy recvBuffer to requestBody
	int remainSize = currHttpRequest->getContentLength() - currHttpRequest->getBodyLeng();
	int appendSize = std::min(remainSize, (int)recvBuffer.length());
	int resultAppend = currHttpRequest->appendBody(recvBuffer.rd_ptr(), appendSize);
	appendCount++;
	
	PAS_DEBUG2("ClientHandler::RECV REQ BODY - recv=%d, remain=%d", recvBuffer.length(), remainSize);
	PHTR_DEBUG2("ClientHandler::RECV REQ BODY - recv=%d, remain=%d", recvBuffer.length(), remainSize);
	
	if(resultAppend < 0)
	{
		PAS_ERROR("ClientHandler::onReceivedRequestBody >> recvBuffer ���� ����");
		PHTR_NOTICE("ClientHandler::onReceivedRequestBody >> recvBuffer ���� ����");
		return -1;
	}

	recvBuffer.rd_ptr(appendSize);

	if(isCompletedReceiveRequestBody())
	{
		PAS_DEBUG2("ClientHandler: appendBody() called %d times for %d bytes", 
			appendCount, currHttpRequest->getBodyLeng());
			
		appendCount = 0;
				
		changeState ( CS_RECEIVED_REQUEST );
		ACE_ASSERT(currHttpRequest != NULL);

		PAS_DEBUG2("ClientHandler::RECV REQ BODY Complete - recv=%d, ContentLength=%d", 
			currHttpRequest->getBodyLeng(), currHttpRequest->getContentLength() );
		PHTR_DEBUG2("ClientHandler::RECV REQ BODY Complete - recv=%d, ContentLength=%d", 
			currHttpRequest->getBodyLeng(), currHttpRequest->getContentLength() );

		if (currHttpRequest->getRawBody() != NULL) {
		//int dump_len = MIN(currHttpRequest->getBodyLeng(), 160);
		//PAS_DEBUG_DUMP("PHONE REQ BODY", currHttpRequest->getRawBody()->rd_ptr(), dump_len);
		}
		
	}

	return 0;
}




int ClientHandler::onCompletedReceiveRequest()
{
	ACE_ASSERT(state == CS_RECEIVED_REQUEST);

	PAS_DEBUG("+-----------------------------------------------------------------------+");
	PAS_DEBUG("|                         Transaction start                             |");
	PAS_DEBUG("+-----------------------------------------------------------------------+");

	PHTR_DEBUG("+-----------------------------------------------------------------------+");
	PHTR_DEBUG("|                         Transaction start                             |");
	PHTR_DEBUG("+-----------------------------------------------------------------------+");

	_sentBodySizeByStream = 0;
	
	PAS_DEBUG3("Session Req(%d) Res(%d) New Request. Request URL is [%s]", 
		session.reqNums, session.respNums, currHttpRequest->getHeader()->getUrl().toStr());
	PHTR_DEBUG3("Session Req(%d) Res(%d) New Request. Request URL is [%s]", 
		session.reqNums, session.respNums, currHttpRequest->getHeader()->getUrl().toStr());	

	session.lastRequestURL = currHttpRequest->getHeader()->getUrl();

	logHttpReq("[PHONE REQ]", currTransaction, true);

	// �ϳ��� Transaction �� ���õ� ��.  Transaction Q �� �߰�.
	if (startTransaction(currTransaction) < 0)
		return -1;

	// currTransaction �� �� ����Ŭ�� recv �� �Ϸ�Ǹ� ���� NULL �� �ʿ��ϴ�.
	currTransaction = NULL;
	currHttpRequest = NULL;
	return 0;

}


/**
SSL ���� ����/ ���� ��� ���  ���⼭ ó�� 
*/
void ClientHandler::onCompletedCPConnection(CpHandler* pCP, int isSucc)
{
	if (state == CS_SSL_CONNECTING)
		changeState ( CS_WAIT_REQUEST_HEADER );

	if(session.isSSL())
	{
		PAS_DEBUG2("CONNECT complete, Succ=%d; %s",  isSucc, setMyInfo());
		PHTR_DEBUG2("CONNECT complete, Succ=%d; %s", isSucc,  getMyInfo());
		
		if ( !isSucc )
		{
			responseSSLConnectionFail();
			return;
		}
		// SSL����� �ϱ� ���� CP���� Ȯ��
		else if(session.getSSLHost() == pCP->getHost() && session.getSSLPort() == pCP->getPort())
		{
			// SSL��ſ� CP ������ �Ϸ� ������ Client���� �˸�
			responseSSLConnectionSuccess();
			return;
		}
		
	}

		/*
		// �Ϲ� (SSL �ƴ� ��) ��  connect ��� �뺸�ȴ�.
	PAS_INFO1("Unexpected CONNECT complete; %s",  setMyInfo());
	PHTR_INFO1("Unexpected: CONNECT complete; %s",  getMyInfo());
		*/
}


void ClientHandler::onCompletedCPResponse(CpHandler* pCP, Transaction *tr)
{
	ACE_ASSERT(pCP != NULL);
	ACE_ASSERT(tr != NULL);

	HTTP::Response* pRes  = tr->getResponse();

	if(pCP->getReceiveBodySize() != pRes->getContentLength())
	{
		PAS_NOTICE2("Size mismatch. ReceiveBodySize[%d] != ContentLength[%d]", pCP->getReceiveBodySize(), pRes->getContentLength());
		PAS_NOTICE_DUMP("CP RESP HEAD", pRes->getRawHeader()->rd_ptr(), pRes->getHeadLeng());
		PAS_NOTICE_DUMP("CP RESP BODY", pRes->getRawBody()->rd_ptr(), pRes->getBodyLeng());
	}

	PAS_DEBUG4("Received Response from CPProxy. HeadSize[%d] StoreBody[%d] StreamBody[%d] %s", pRes->getHeadLeng(), pRes->getBodyLeng(), _sentBodySizeByStream, getMyInfo());
	PHTR_DEBUG4("ClientHandler::onCompletedCPResponse >> RECV CP RESP - Head[%d] StoreBody[%d] StreamBody[%d] %s", pRes->getHeadLeng(), pRes->getBodyLeng(), _sentBodySizeByStream, getMyInfo());

	if(pRes->getContentLength() >= 1024*1024)
	{
		PAS_INFO3("Big contents found. Contents size is %d Bytes. URL is [%s] %s", 
			pRes->getContentLength(), tr->getRequest()->getHeader()->getUrl().toStr(), getMyInfo());
	}
	
	PHTR_HEXDUMP(pRes->getRawHeader()->rd_ptr(), pRes->getHeadLeng(), "CP RESP HEAD");
	if (pRes->getRawBody()) {
		int dump_len = MIN(pRes->getBodyLeng(), 160);
		//PAS_TRACE_DUMP("CP RESP BODY", pRes->getRawBody()->rd_ptr(), dump_len);
		PHTR_HEXDUMP(pRes->getRawBody()->rd_ptr(), dump_len, "CP RESP BODY");
	}
	
	#ifdef HTTP_LOG
	logHttpResp("[CP RESP]", tr, false);
	#endif

	afterCpTransaction(tr);
}

void ClientHandler::onCommand(CommandID cid, PasHandler* pEH, void* arg1, void* arg2)
{
	PAS_TRACE1("ClientHandler::onCommand >> fd[%d]", get_handle());
	PHTR_DEBUG("ClientHandler::onCommand");

	ACE_ASSERT(pEH != NULL);

	switch(cid)
	{
	// CP receive complete
	case CID_CP_Completed:
	{
		PAS_TRACE1("CID_CP_Completed, fd[%d]", get_handle());
		PHTR_DEBUG("ClientHandler::onCommand >> CID_CP_Completed");
		CpHandler* pCP = dynamic_cast<CpHandler*>(pEH);
		onCommandCPCompleted(pCP, (Transaction *)arg1);
		break;
	}

	// CP Receive SSL Data
	case CID_CP_SSLData:
	{
		PAS_TRACE1("CID_CP_SSLData, fd[%d]", get_handle());
		PHTR_DEBUG("ClientHandler::onCommand >> CID_CP_SSLData");
		CpHandler* pCP = dynamic_cast<CpHandler*>(pEH);
		onCommandCPReceivedSSLData(pCP, (char*)arg1, *(size_t*)arg2);
		break;
	}

	// CP Connection established
	case CID_CP_Connected:
	{
		PAS_TRACE1("CID_CP_Connected, fd[%d]", get_handle());
		PHTR_DEBUG("ClientHandler::onCommand >> CID_CP_Connected");
		CpHandler* pCP = dynamic_cast<CpHandler*>(pEH);
		
		ACE_ASSERT(arg1 != NULL);
		int	succFlag = *(static_cast<int*>(arg1));
		onCommandCPConnected(pCP, succFlag);
		break;
	}

	// CP Close
	case CID_CP_Closed:
	{
		PAS_TRACE1("CID_CP_Closed, fd[%d]", get_handle());
		PHTR_DEBUG("ClientHandler::onCommand >> CID_CP_Closed");
		CpHandler* pCP = dynamic_cast<CpHandler*>(pEH);
		onCommandCPClosed(pCP);
		break;
	}

	case CID_CP_Received_Header:
	{
		PAS_TRACE1("CID_CP_Received_Header, fd[%d]", get_handle());
		PHTR_DEBUG("ClientHandler::onCommand >> CID_CP_Received_Header");
		CpHandler* pCP = dynamic_cast<CpHandler*>(pEH);
		CpResponseData* pData = static_cast<CpResponseData*>(arg1);
		onReceivedHeaderFromCP(pCP, pData);
		break;
	}

	case CID_CP_Received_PartOfBody:
	{
		PAS_TRACE1("CID_CP_Received_Header, fd[%d]", get_handle());
		PHTR_DEBUG("ClientHandler::onCommand >> CID_CP_Received_Header");
		CpHandler* pCP = dynamic_cast<CpHandler*>(pEH);
		CpResponseData* pData = static_cast<CpResponseData*>(arg1);
		onReceivedPartialBodyFromCP(pCP, pData);
		break;
	}

	// Santa Complete
	case CID_Santa_Completed:
	{
		PAS_TRACE1("CID_Santa_Completed, fd[%d]", get_handle());
		SantaHandler* pSanta = dynamic_cast<SantaHandler*>(pEH);
		onCommandSantaCompleted(pSanta, (const SANTA::Response*)arg1);
		break;
	}

	// Santa Close
	case CID_Santa_Closed:
	{
		PAS_TRACE1("CID_Santa_Closed, fd[%d]", get_handle());
		SantaHandler* pSanta = dynamic_cast<SantaHandler*>(pEH);
		onCommandSantaClosed(pSanta);
		break;
	}

	case CID_Santa_TimeOut:
	{
		PAS_TRACE1("CID_Santa_TimeOut, fd[%d]", get_handle());
		SantaHandler* pSanta = dynamic_cast<SantaHandler*>(pEH);
		onCommandSantaTimeOut(pSanta);
		break;
	}

	// Unknown
	default:
		PAS_ERROR2("onCompleted() >> Unknown CommandID[%d], fd[%d]", (int)cid, get_handle());
		PHTR_DEBUG2("ClientHandler::onCompleted >> Unknown CommandID[%d], fd[%d]", (int)cid, get_handle());
	}
}

/** ������ Request/Response ���� �����ϴ� ���� Transaction �ϳ��� ������. */
int ClientHandler::onCommandCPCompleted(CpHandler* pCP, Transaction *resTransaction)
{
	PAS_TRACE("onCommandCPCompleted");
	PHTR_DEBUG("ClientHandler::onCommandCPCompleted");

	ACE_ASSERT(pCP != NULL);
	//ACE_ASSERT(pCP == &_cp);
	//ACE_ASSERT(cpList.isExist(pCP));
	ACE_ASSERT(resTransaction != NULL);

	onCompletedCPResponse(pCP, resTransaction);

	return 0;
}

int ClientHandler::onCommandCPConnected(CpHandler* pCP, int isSucc)
{
	PAS_TRACE("onCommandCPConnected");
	PHTR_DEBUG("ClientHandler::onCommandCPConnected");

	ACE_ASSERT(pCP != NULL);
	//ACE_ASSERT(pCP == &_cp);
	//ACE_ASSERT(cpList.isExist(pCP));

	onCompletedCPConnection(pCP, isSucc);

	return 0;
}

int ClientHandler::onCommandCPClosed(CpHandler* pCP)
{
	PAS_TRACE("onCommandCPClosed");
	PHTR_DEBUG("ClientHandler::onCommandCPClosed");

	ACE_ASSERT(pCP != NULL);
	//ACE_ASSERT(pCP == &_cp);
	//ACE_ASSERT(cpList.isExist(pCP));

	return 0;
}



/**
�ܸ��κ��� SSL ����Ÿ ���� ���� CP�� ����.
*/

int ClientHandler::onReceivedSSLData(Transaction *tr)
{
	PAS_TRACE2("SSL Data from phone - sock[%d] %d bytes", get_handle(), recvBuffer.length());
	PHTR_DEBUG2("SSL Data from phone - sock[%d] %d bytes", get_handle(), recvBuffer.length());

	// ���ŵ� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return 0;

	// SSL Data Relay
	// send to CP
	CpHandler* cp = pickCpHandler(session.getSSLHost(), session.getSSLPort());

	if (cp == NULL)
	{
		PHTR_NOTICE1("ClientHandler::onReceivedSSLData - SSL CP Closed [%s]", getMyInfo());
		return -1;
	}
	#ifdef HTTP_DUMP
	filedump->init("PHONE-REQU-SSL", 1);
	filedump->write(recvBuffer.rd_ptr(), recvBuffer.length());
	#endif
	
	if (userInfo)
		userInfo->updateReqTime();
	
	if (cp->sendSSLData(recvBuffer.rd_ptr(), recvBuffer.length()) < 0)
	{
		PHTR_NOTICE1("ClientHandler::onReceivedSSLData - SSL CP Closed [%s]", getMyInfo());
		return -1;
	}
		

	if (session.sslReqBytes==0) {
		session.beginSSL(); // SSL connect/response �� �ϳ��� tr �� �����ϰ�, SSL ����Ÿ �������� SSL ���� ������ ������ ����.
		tr->recvPhoneReq();
	}
		
	tr->sendCpReq();
	tr->onSslDataRecv(recvBuffer.length());
	session.onSslDataRecv(recvBuffer.length());

	
	
	// �α� �ۼ�.
	paslog->writeSslDataReq(userInfo, &session, recvBuffer.length());
	recvBuffer.reset();

	return 0;
}

/**
CP �κ��� ���� SSL ����Ÿ�� �ܸ��� ����.
*/
int ClientHandler::onCommandCPReceivedSSLData(CpHandler* pCP, const char* buf, size_t bufSize)
{
	PAS_TRACE2("SSL Data from CP - sock[%d] %d bytes", pCP->get_handle(), bufSize);
	PHTR_DEBUG2("SSL Data from CP - sock[%d] %d bytes", pCP->get_handle(), bufSize);


	ACE_ASSERT(pCP != NULL);
	//ACE_ASSERT(pCP == &_cp);
	//ACE_ASSERT(cpList.isExist(pCP));

	enSendQueue(buf, bufSize);

	session.onSslDataSend(bufSize);

	#ifdef HTTP_DUMP
	filedump->init("PHONE-RESP-SSL", 4);
	filedump->write(buf, bufSize);
	#endif
	
	if (currTransaction)
		currTransaction->recvCpResp();
	// �α� �ۼ�.
	paslog->writeSslDataResp(userInfo, &session, bufSize);
	return 0;
}

int ClientHandler::onCommandSantaCompleted(SantaHandler* pSanta, const SANTA::Response* /* pResponse */)
{
	Transaction	*tr = NULL;
	
	ACE_ASSERT(pSanta != NULL);

	SANTA::MDN_t santaMDN;
	SANTA::IMSI_t santaIMSI;

	santaMDN = pSanta->getMDN();
	santaIMSI = pSanta->getIMSI();

	PAS_TRACE3("SANTA query result MDN[%s] IMSI[%s] - %s", santaMDN.toStr(), santaIMSI.toStr(), getMyInfo());
	PHTR_DEBUG3("SANTA query result MDN[%s] IMSI[%s]- %s", santaMDN.toStr(), santaIMSI.toStr(), getMyInfo());
	
	if (! transactionQueue.empty())
	{
		tr = transactionQueue.front();
	}
	else
	{
		tr = currTransaction;
	}

	ACE_ASSERT(tr != NULL);
		
	tr->santaEndTime = ACE_OS::gettimeofday();
		
	// SANTA ���� ����� ���������� �Դ��� üũ
	if( pSanta->isPassed() == false )
	{
	
		// SANTA ��ȸ ���� ������ ����
		ResponseBuilder::SantaFailed( tr->getResponse() );

		// Over10 �α� ����� ���� ���� �ڵ�
		tr->setErrorRespCode( RESCODE_SANTA );

		// @todo ��� transaction �� �����Ͽ��� �Ѵ�.
		afterCpTransaction(tr);

		return 0;
	}
	else 
	{
		session.isSantaNumber = true; // SANTA ��ȸ ����� session �� �����Ͽ����� ǥ��.
		// MDN �� IMSI �� ���� ������  �����Ѵ�.
		STRNCPY( session.phoneNumber, santaMDN.toStr(), LEN_PHONE_NUM );
		STRNCPY( session.IMSI, santaIMSI.toStr(), LEN_IMSI );
		session.santaTime = time(NULL);

		// SANTA_WAITING ���¸� ����.
		changeState(CS_WAIT_REQUEST_HEADER);

		/// SANTA ��ȸ�� ������ ���� ������ �۾���(Auth ����, CP�� ������ ��û��)�� �����Ѵ�.
		midTransaction(tr);
	}
	

	return 0;
}

int ClientHandler::onCommandSantaClosed(SantaHandler* /* pSanta */)
{
	PAS_TRACE("ClientHandler::onCommandSantaClosed");

	changeState(CS_WAIT_REQUEST_HEADER);

	return 0;
}

int ClientHandler::onCommandSantaTimeOut(SantaHandler* /* pSanta */)
{
	Transaction	*tr = NULL;

	PAS_INFO1("SANTA query failed (TimeOut). %s", getMyInfo());
	PHTR_DEBUG1("ClientHandler::SANTA query failed (TimeOut)- %s", setMyInfo());
	
	changeState(CS_WAIT_REQUEST_HEADER);
	
	if (! transactionQueue.empty())
	{
		tr = transactionQueue.front();
	}
	else
	{
	// @todo
		tr = currTransaction;
	}

	ACE_ASSERT(tr != NULL);
	
	/// SANTA DB ��ȸ ���� �޽��� ����
	ResponseBuilder::SantaFailed( tr->getResponse() );

	// Over10 �α� ����� ���� ���� �ڵ�
	tr->setErrorRespCode( RESCODE_SANTA );

	afterCpTransaction(tr);

	return 0;
}

int ClientHandler::handle_timeout(const ACE_Time_Value& current_time, const void* /* act */)
{
	PAS_TRACE1("ClientHandler::handle_timeout >> fd[%d]", get_handle());
	PHTR_DEBUG1("ClientHandler::handle_timeout >> fd[%d]", get_handle());

	if (!everRecvPacket)
	{
		/*
		�ƹ� �޽������� �ٷ� ������ �ܸ��� timeout������ ��ٸ��� �ʰ� 10�� ���� close �ϱ�
		2006.12.19
		*/
		time_t now = time(NULL);
		int idle = now - session.startSec;
		if (idle >= 20)
		{
			PAS_INFO2("Phone idle without any mesg for %d secs. %s", idle, getMyInfo());

			/* TB���� kunmulti.py �� �׽�Ʈ �� ��� core �߻� ����. */
			if(isConnected())
				pSysStat->clientCloseByHost(1);
			close();

			return 0;
		}
	}
	
	ACE_Time_Value recvTimeout(Config::instance()->client.receiveTimeout);

	if( isIdle(current_time, recvTimeout) )
	{
		if (recvBuffer.length() > 0)
		{
			PAS_INFO2("Phone is idle with %d bytes mesg. Force to close. %s", recvBuffer.length(), getMyInfo());
			PAS_INFO_DUMP("Mesg left in recvBuffer", recvBuffer.rd_ptr(), recvBuffer.length());
		}
		else
			PAS_INFO1("Phone is idle. Force to close. %s", getMyInfo());

		// increase statistic
		if(isConnected())
			pSysStat->clientCloseByHost(1);		
		close();
	}

	return 0;
}

int ClientHandler::handle_exception(ACE_HANDLE fd)
{
	PAS_TRACE1("ClientHandler::handle_exception >> fd[%d]", fd);
	PHTR_DEBUG1("ClientHandler::handle_exception >> fd[%d]", get_handle());
	
	return 0;
}



/**
MNC ���� 04, 08 �� �ƴ� ��� (�Ƹ��� 00) �� ���� ��ȣ�뿪���� �˻��ϰ� 
Santa ������ �����Ѵ�. -- 2006.10.13

@brief SANTA �� �����ϱ� ���� ��� ������ �˻��Ѵ�.
       ���� SANTA ������ �ʿ���� ���¶�� true �� �����Ͽ� ���� �ϷḦ �˷��ش�.
	   �� �ܿ� false �� ���� ������ �����Ѵ�.

@return �����Ϸ�(true), ��������(false)

*/
int  ClientHandler::checkSanta(Transaction *tr)
{
	#ifdef TEST_VERSION
	const Config* pConfig = Config::instance();
	if(pConfig->santa.enable == false)
	{
		PAS_DEBUG1("SANTA check disabled - %s", getMyInfo());
		santaHandler.setPassed();
		return 0;
	}
	#endif

	if (strcmp(tr->MNC, "04") == 0 || strcmp(tr->MNC, "08") == 0)
	{
		santaHandler.setPassed();
		return 0;
	}

	// MIN�� Anonymous �̸� SANTA ��ȸ�� �ʿ����.
	if( !strcmp(tr->MINnumber, "Anonymous") || !strcmp(tr->MINnumber, "N/A") )
	{
		santaHandler.setPassed();
		return 0;
	}

	// ���� ��ȣ �뿪�� �ƴϸ� SANTA ��ȸ�� �ʿ� ����.
	if (KtfInfo::isVirtualNumber(tr->phoneNumber) == false)
	{
		santaHandler.setPassed();
		return 0;
	}

	PAS_DEBUG1("ClientHandler::SANTA query start - %s", getMyInfo());
	PHTR_DEBUG1("ClientHandler::SANTA query start - %s", getMyInfo());

	// Santa �ڵ鷯 ���
	santaHandler.set( reactor(), this );
	santaHandler.setTraceLog( this->tracelog );

	if (santaHandler.start(tr) < 0)
	{
		PAS_WARNING1("ClientHandler::SANTA connection failed - %s", getMyInfo());
		PHTR_WARN1("ClientHandler::SANTA connection failed - %s", getMyInfo());
		return -1;
	}
	return 0;
}


/**
�� session ���� �ѹ��� �����ϸ� �ȴ�.
*/
int ClientHandler::checkAuth(Transaction *tr)
{
	const Config* pConfig = Config::instance();

	#ifdef TEST_VERSION
	if (pConfig->auth.enable == false) {
		return 0;
	}
	#endif
	if( authHandler.getUserInfo() == NULL )
		authHandler.set( userInfo );

	// ���������� ���, n�� �Ŀ� �������� �޾ƾ� �Ѵ�.
	if( userInfo->getAuthState() == AS_RESPONSED )
	{
		time_t currTime;
		time( &currTime );

		// ������ ���� �ð��� n�� �̻��̸� AuthState�� AS_NONE���� ���� �����ν�
		// �������� �޵��� �����Ѵ�.
		time_t delayTime = currTime - userInfo->getLatestAuthTime();
		if( delayTime >= pConfig->auth.authentication )	// 10��
			userInfo->changeAuthState( AS_NONE );
	}

	// ���� �ѹ� ���� ����
	if( authHandler.getState() == AS_NONE )
	{
		const host_t& host = pConfig->auth.host;
		const int port = pConfig->auth.port;

		/// Auth �� ���ӿ�û, AuthEventHandler ���� �� ���ӵ� ���� �ڵ� ����,
		/// Auth �� ��û�� ������ ����
		requestAuth(tr, host, port);

		return 0;
	}

	// Auth ������ �������и� ���� �ߴ�.
	if( authHandler.isPassed() == false )
	{
		// ���� ���н� ������ �������� �ޱ����� AS_NONE ���� ����
		userInfo->changeAuthState( AS_NONE );
		return -1;
	}

	return 0;
}


int ClientHandler::requestAuth(Transaction *tr, const host_t& /* host */, int /* port */)
{
	// Auth�� ���� ��û

	// Auth���� ��û�� ������ ����
	if( tr->phoneNumber[0] == '\0' )
	{
		PAS_NOTICE1("ClientHandler::AUTH >> Not Found HTTP_PHONE_NUMBER, fd[%d]", get_handle());
		PHTR_NOTICE("ClientHandler::AUTH >> Not Found HTTP_PHONE_NUMBER");
		return -1;
	}

	/// ���⼭ ���� Auth �� ��û�� ������ ����
	int isCaching = 0;
	if( KtfInfo::isNewBrowser(tr->browser) == false )
		isCaching = 1;

	/// PasGW ��Ʈ
	Config *pConf = Config::instance();
	int npasgwPort = pConf->network.listenPort;

	/// 3G �ܸ��� ����
	/*
	3G �ܸ� �Ǵ� ���� �߰�
	2007.5.7
	*/
	int n3G = (tr->is3G);

	/// Auth �� ��û�� ������ ���� �Ϸ�

	authHandler.setTraceLog( this->tracelog );

	AUTH::RequestBody reqBody;

	/// Auth �� ������ ��û
	if( n3G )
	{
		makeRequestAuth(reqBody, 1, tr->MINnumber, tr->nIPAddr, npasgwPort, 0, isCaching, n3G, tr->MDN, tr->msModel);

		PAS_DEBUG5( "ClientHandler::AUTH >> MIN[%s] IP[%d] PORT[%d] Caching[%d] 3G[%d]",
			tr->MINnumber, tr->nIPAddr, npasgwPort, isCaching, n3G );
	}

	else
	{
		makeRequestAuth(reqBody, 1, tr->IMSI, tr->nIPAddr, npasgwPort, 0, isCaching, n3G, tr->MDN, tr->msModel);

		PAS_DEBUG5( "ClientHandler::AUTH >> IMSI[%s] IP[%d] PORT[%d] Caching[%d] 3G[%d]",
			tr->IMSI, tr->nIPAddr, npasgwPort, isCaching, n3G );
	}

	// ���� ��û ���·� ����
	userInfo->changeAuthState( AS_REQUESTED );

	// AuthAgent �� ���� ��û
	AuthAgent *authAgent = AuthAgent::instance();
	int res = authAgent->putWork(reqBody, userInfo);
	session.pasauthTime = time(NULL);

	return res;
}

void ClientHandler::makeRequestAuth(AUTH::RequestBody &body, const int seq, const char* pMin, const unsigned int ip, const int port, const int startConn, const int newBrowser, int g3GCode, const char* pMdn, const char* pMsModel)
{
	// init body
	body.type = AUTH::REQUEST_AUTHORIZE_CODE;
	body.seq = seq;
	STRNCPY(body.min, pMin, sizeof(body.min));
	body.accessIP = ip;
	body.port = port;
	body.startConn = startConn;
	body.newBrowser = newBrowser;
	body.g3GCode = g3GCode;

	if(pMdn)
	{
		STRNCPY(body.mdn, pMdn, sizeof(body.mdn));
	}
	else
		memset(body.mdn, 0x00, sizeof(body.mdn));


	if(pMsModel)
	{
		STRNCPY(body.msModel, pMsModel, sizeof(body.msModel));
	}
	else
		memset(body.msModel, 0x00, sizeof(body.msModel));
}


/**
CP pRequest ����.
*/

bool ClientHandler::applyHotNumber(Transaction *tr)
{
	HTTP::Request* pRequest = tr->getRequest();

	ACE_ASSERT(pRequest != NULL);
	ACE_ASSERT(!pRequest->getHeader()->getUrl().isEmpty());

	HotNumber hotNumber;
	const url_t orgurl = pRequest->getHeader()->getOrgUrl();

	
	int hit = hotNumber.convert(orgurl, orgurl.size());
	if(hit)
	{
		int ret = pRequest->setUrl(hotNumber.getConverted());
		const url_t newurl = pRequest->getHeader()->getUrl();
		
		PAS_DEBUG2("HotNumber applied: ORG=%s, URL=%s", orgurl.toStr(), newurl.toStr());
		PHTR_DEBUG2("HotNumber applied: ORG=%s, URL=%s", orgurl.toStr(), newurl.toStr());
		
		if(ret < 0)
		{
			PAS_ERROR("ClientHandler::applyHotNumber >> HotNumber ���� ����");
			return false;
		}

		STRCPY(tr->realUrl, newurl.toStr(), MAX_URL_LEN);
		tr->hotNumberConverted = true;
		return true;
		
	}
	return false;
}


/**
CP pRequest ����. Hotnumber �Ŀ� 
*/
ClientHandler::AclResult ClientHandler::applyACL(HTTP::Request* pRequest, host_t& proxyHost, int& proxyPort)
{
	ACE_ASSERT(pRequest != NULL);
	ACE_ASSERT(!pRequest->getHeader()->getHost().isEmpty());

	const host_t host = pRequest->getHeader()->getHost();
	const int port = pRequest->getHeader()->getPort();

	AclRouter* pAcl = AclRouter::instance();

	char newHost[MAX_HOST_LEN+1] = "\0";
	int newPort;

	PAS_DEBUG2("ACL Query >> Query about %s:%d", host.toStr(), port);

	// ACL SWITCH �� OFF �� searchDNS �� �����ϰ�
	// ON �̸� searchALL �� �����Ѵ�.
	// ACL_FIND_DNS Ȥ�� ACL_NOT_FOUND �� ��� ������ ���� ID�� DEFAULT(KUN00)�� �ƴϸ�
	// KUN00 ���� MULTIPROXY �϶�� �˷��ش�.
	int searchResult;
	if( Config::instance()->acl.multiproxy == true )
	{
		searchResult = pAcl->searchALL(host, port, newHost, sizeof(newHost), newPort);
		if( searchResult == ACL_FIND_DNS || searchResult == ACL_NOT_FOUND )
		{
			if( Config::instance()->process.serverID != DEFAULT_MULTI_PROXY )
			{
				AclRouter::instance()->getHost( newHost, sizeof(newHost), newPort, DEFAULT_MULTI_PROXY );

				// MultiProxy �ؾ���
				searchResult = ACL_DENY_ACL;
			}
		}
	}

	else
	{
		// searchDNS ����
		searchResult = pAcl->searchDNS(host, port, newHost, sizeof(newHost), newPort);
	}

	const bool changedHostAndPort = (searchResult == ACL_FIND_DNS || searchResult == ACL_ALLOW_ACL);

	PAS_TRACE2("ACL searchResult[%d] changedHostAndPort[%d]", searchResult, changedHostAndPort);
	PHTR_DEBUG2("ACL searchResult[%d] changedHostAndPort[%d]", searchResult, changedHostAndPort);
		
	switch(searchResult)
	{
	case ACL_NOT_FOUND:
		PAS_TRACE("applyACL >> ACL_NOT_FOUND");
		break;

	case ACL_FIND_DNS:
		PAS_TRACE("applyACL >> ACL_FIND_DNS");
		break;

	case ACL_ALLOW_ACL:
		PAS_TRACE("applyACL >> ACL_ALLOW_ACL");
		break;

	case ACL_DENY_ACL:
		PAS_TRACE("applyACL >> ACL_DENY_ACL");
		break;

	case ACL_INPUT_ERR:
		PAS_TRACE("applyACL >> ACL_INPUT_ERR");
		break;

	default:
		PAS_NOTICE("applyACL >> Unknown ACL Response Code");
		break;
	}

	// ����� host�� port ����
	if(changedHostAndPort)
	{
		/***
		int setResult = pRequest->setHostPort(newHost, newPort);
		const url_t url  = pRequest->getHeader()->getUrl();
		PAS_DEBUG3("%s ACL applied TR[%d], URL=%s", setMyInfo(), currTransaction->id(), url.toStr());
		PHTR_DEBUG3("%s ACL applied TR[%d], URL=%s", getMyInfo(), currTransaction->id(), url.toStr());
		****/
		proxyHost = newHost;
		proxyPort = newPort;
		return ACL_DNS_APPLIED;
	}

	// proxy ���� �ʿ�
	if(searchResult == ACL_DENY_ACL)
	{
		proxyHost = newHost;
		proxyPort = newPort;
		return AR_CHANGE_PROXY;
	}

	// proxy ���� �ʿ� ����
	return AR_HOLD_PROXY;
}

/**
���� ó��

FILTER_BY_URL -- ������ ��Ʈ�� ��ġ �˻�.   http://localhost:80  �� http://localhost �� ���� �ٸ� ������ ���ֵȴ�.
FILTER_BY_DOMAIN -- http://localhost:80  �� http://localhost �� ���� ���� ������ ���ֵȴ�.
*/
bool	ClientHandler::checkStatFilter(Transaction *tr, char *notimesg )
{
	
	const HTTP::RequestHeader* h = tr->getRequest()->getHeader();
	StatFilterMng *pStatFilterMng = StatFilterMng::instance();

	char *urlValue = 0;

	if (tr->hotNumberConverted)
		urlValue = tr->realUrl;
	else
		urlValue = tr->orgUrl;
		
	if (pStatFilterMng->isBlocked(FILTER_BY_URL,  urlValue, 0, notimesg))
		return true;
		
	if (pStatFilterMng->isBlocked(FILTER_BY_DOMAIN,  (char*)h->getHost().toStr(), h->getPort(), notimesg))
		return true;
		
	if (pStatFilterMng->isBlocked(FILTER_BY_MDN,  tr->phoneNumber,  0, notimesg))
		return true;

	return false;
}


/**
CP ���� �Ŀ�.
*/
int ClientHandler::addHashKey(HTTP::ResponseHeader* pDestHeader, const url_t& requestUrl, const size_t requestSize, int responseBodyLeng)
{
	PAS_TRACE1("ClientHandler::addHashKey >> fd[%d]", get_handle());

	ACE_ASSERT(pDestHeader != NULL);
	ACE_ASSERT(requestSize > 0);

	// hashkey ����
	char hashKey[MaxHashKeySize+4];
	HashKey::getKtfHashKey(hashKey, requestUrl.toStr());
	

	//!!! Contents-Length �� ���� ����Ÿ ������ �ٸ� ��  �ִ�. 
	//!!! ���� ����Ÿ ���� �������� �ۼ��Ͽ��� �Ѵ�.  2006.10.17
	
	int expectedRSSIZE =  responseBodyLeng;

	// �ӽ� KTF_HTTP_KEY ������ ����
	line_t line;
	line.sprintf("%s;RQSIZE=%u;RSSIZE=%d", hashKey, requestSize, expectedRSSIZE);

	// KTF_HTTP_KEY �߰�
	int ret = pDestHeader->addElementAtFront("KTF_HTTP_KEY", line);
	if(ret < 0)
	{
		PAS_ERROR1("ClientHandler::addHashKey >> Http ����� KTF_HTTP_KEY �߰� ����, fd[%d]", get_handle());
		return -1;
	}

	// �ӽ� �۽ſ� http header ����
	HTTP::header_t header;
	ret = pDestHeader->build(&header);
	if(ret < 0)
	{
		PAS_ERROR1("ClientHandler::addHashKey >> �ӽ� Http ��� ������ ���� ����, fd[%d]", get_handle());
		return -1;
	}

	// KTF_HTTP_KEY �߰��� ���� RSSIZE ��ȭ ���� ����	
	for(int loop=0; loop<3; loop++) // while(true) ���� ������. Ȥ�� ���� �����Ǹ� �ȵǴϱ�.
	{
		int diffSize = (header.size() +responseBodyLeng) - expectedRSSIZE;

		// ������ ���ٸ� ����
		if(diffSize == 0)
			break;

		expectedRSSIZE += diffSize;
		line.sprintf("%s;RQSIZE=%u;RSSIZE=%d", hashKey, requestSize, expectedRSSIZE);

		pDestHeader->replaceElement("KTF_HTTP_KEY", line);
		ret = pDestHeader->build(&header);
		if(ret < 0)
		{
			PAS_ERROR1("ClientHandler::addHashKey >> ���� ���� Http ��� ������ ���� ����, fd[%d]", get_handle());
			return -1;
		}
	}
	HashKey::writeLog(session.phoneNumber, session.ipAddr, hashKey, requestUrl.toStr(), requestSize, expectedRSSIZE);

	return 0;
}



CpHandler* ClientHandler::pickCpHandler(const host_t& host, int port)
{
	/*
	_cp.setTraceLog( tracelog );

	if (session.isSSL())
	{
		return &_cp;
	}

	if(Config::instance()->cp.ReuseCpConnection)
	{
		if(_cp.getHost() != host || _cp.getPort() != port)
			_cp.reset(host, port);
	}
	else
	{
		_cp.reset(host, port);
	}


	return &_cp;
	*/

	if(session.isSSL() && _pCP != NULL)
	{
		if(_pCP->getHost() == host || _pCP->getPort() == port)
		{
			return _pCP;
		}
	}

	if(_pCP != NULL)
	{
		delete _pCP;
		_pCP = NULL;
	}

	if(_pCP == NULL)
	{
		// CP �ڵ鷯 ���� & ���
		_pCP = new CpHandler(reactor(), this);
		_pCP->init();
		_pCP->setTraceLog( this->tracelog );

		// ���� �ҽ�
		_pCP->setHost(host);	
		_pCP->setPort(port);

		return _pCP;
	}

	

/*
	CpHandler* pCP = NULL;

	// 2006-12-01 ����â
	// ReuseCpConnection ���׷� ���Ͽ� ReuseCpConnection ��� ����
	//if (session.isSSL() || Config::instance()->cp.ReuseCpConnection)
	if (session.isSSL())
	{
		pCP = cpList.get(host, port);
	}

	if(pCP != NULL)
	{
		PAS_DEBUG3("REUSE CP[%X] %s:%d", pCP, host.toStr(), port);
		PHTR_DEBUG3("REUSE CP[%X] %s:%d", pCP, host.toStr(), port);
		pCP->setTraceLog( this->tracelog );
		return pCP;
	}

	// CP �ڵ鷯 ���� & ���
	pCP = new CpHandler(reactor(), this);
	pCP->init();
	pCP->setTraceLog( this->tracelog );

	#ifdef VERIFY_PAS
	port  = Config::instance()->cp.fakeCpPort;
	
	PAS_INFO1("VERIFY_PAS (COMPILED FOR TEST):  CP= localhost:%d",  port);
	pCP->setHost("127.0.0.1");	
	pCP->setPort(port);	
	#else
	// ���� �ҽ�
	pCP->setHost(host);	
	pCP->setPort(port);
	#endif
	

	cpList.add(pCP);

	PAS_TRACE3("NEW CP[%X] %s:%d", pCP, host.toStr(), port);
	PHTR_DEBUG3("NEW CP[%X] %s:%d", pCP, host.toStr(), port);
	
	return pCP;
*/
}


/**
CP handler�� ���� �����ϰų�, ���� CP handler�� �̿��Ͽ� ���ο� ��û ó���� �����ϵ��� �Ѵ�.
CP ���� ���� ���� ���� ���� ó���� ����Ͽ��� �Ѵ�.
*/
void ClientHandler::requestToCP(Transaction *tr)
{
	ACE_ASSERT(tr != NULL);
	
	preRequestToCP(tr);

	HTTP:: Request *pRequest = tr->getRequest();

	// DNS Query �� �������� ��� ����ó���Ѵ�.
	if ( strcmp(tr->cpIpAddr,"0.0.0.0")==0)
	{
		PAS_INFO("Can't request to CP. Because of DNS query failure.");

		ResponseBuilder::DnsFail( tr->getResponse() );
		tr->setErrorRespCode( RESCODE_CP_TIMEOUT );
		afterCpTransaction(tr);
		return ;
	}

	PHTR_DEBUG3("CP REQ: Host=%s, IP=%s, Port=%d", tr->cpHostName, tr->cpIpAddr, tr->cpPort);
	

#ifdef DEBUG_LOG_HTTP_HEADER
	{
		BigString httpReqHeader(pRequest->getRawHeader()->rd_ptr(), pRequest->getHeadLeng());
		PAS_INFO1("Contents of header which request to CP is\n%s", httpReqHeader.c_str());
	}
#endif
	
	PHTR_HEXDUMP(pRequest->getRawHeader()->rd_ptr(), pRequest->getHeadLeng(), "CP REQ HEAD"); 
	
	CpHandler* pCP = pickCpHandler(tr->cpIpAddr, tr->cpPort);

	logHttpReq("[CP REQ]", tr, false);
	
	int resultRequest = pCP->start(tr);
	if(resultRequest < 0)
	{
		// Connection ���� --> ������ ���� �޽����� �ܸ��� ����.
		PAS_INFO3("Can't connect to CP{%s, %s:%d}", tr->cpHostName, tr->cpIpAddr, tr->cpPort);
		PHTR_INFO3("Can't connect to CP{%s, %s:%d}", tr->cpHostName, tr->cpIpAddr, tr->cpPort);

		ResponseBuilder::CpConnFailed( tr->getResponse() );
		tr->setErrorRespCode( RESCODE_CP_CONN );
		afterCpTransaction(tr);
		return ;
	}

	PAS_TRACE3("CP CONN TRYING: Host=%s, IP=%s, Port=%d", tr->cpHostName, tr->cpIpAddr, tr->cpPort);
	PHTR_DEBUG3("CP CONN TRYING: Host=%s, IP=%s, Port=%d", tr->cpHostName, tr->cpIpAddr, tr->cpPort);
}


/**
CP ���� �Ŀ�. 
����� Bill Info �� CP ������ HTTP header �� �����ؼ� �ֹǷ� �� ����� config �� ON/OFF �����Ͽ��� �Ѵ�.
*/
int ClientHandler::addBillInfo(HTTP::ResponseHeader* pDestHeader)
{
	PAS_TRACE1("ClientHandler::addBillInfo >> fd[%d]", get_handle());

	ACE_ASSERT(pDestHeader != NULL);

	char key[MaxBillInfoKeySize+4];

	if(Util::CreateBillInfoKey(key) < 0)
	{
		PAS_ERROR1("ClientHandler::addBillInfo >> Bill Info Key ���� ����, fd[%d]", get_handle());
		return -1;
	}

	pDestHeader->addElementAtFront("BILL_INFO", key);

	return 0;
}

/**
�ȳ� ������ ���� ��û�� ���ؼ��� BILL_INFO�� KTF_BILL_INFO_PAGE�� �Է��Ͽ� ������ ó���ǵ��� �Ѵ�.
*/
int ClientHandler::addGuidePageBillInfo(HTTP::ResponseHeader* pDestHeader)
{
	PAS_TRACE1("ClientHandler::addGuidePageBillInfo >> fd[%d]", get_handle());

	ACE_ASSERT(pDestHeader != NULL);

	const char* key = "KTF_BILL_INFO_PAGE";

	pDestHeader->addElementAtFront("BILL_INFO", key);

	return 0;
}

void ClientHandler::addProxyInfo(HTTP::RequestHeader* pDestHeader)
{
	PAS_TRACE("addProxyInfo");

	ACE_ASSERT(pDestHeader != NULL);

	const Config* pConfig = Config::instance();

	//MULTI_PROXY: KUN00
	//HTTP_PROXY_INFO:PNAME:t-upas-02;PTIME:20060926001736

	char hostName[32] = "\0";
	gethostname(hostName, sizeof(hostName)-1);

	char curTime[32] = "\0";
	Util2::get_nowtime_str_simple(curTime);

	char proxyInfo[256] = "\0";
	snprintf(proxyInfo, sizeof(proxyInfo), "PNAME:%s;PTIME:%s", hostName, curTime);

	pDestHeader->addElementAtFront("HTTP_PROXY_INFO", proxyInfo);
	pDestHeader->addElementAtFront("MULTI_PROXY", pConfig->process.serverID);
}

/**
�ܸ��� Http Response �޽����� �۽��ϴ� ���̴�.
�������� socket send �� �ƴ϶� sendQueue  �� �ִ� ���̴�.
*/
int	ClientHandler::sendRespToPhoneHeadbody(HTTP::Response* resToPhone)
{
	// �۽�ť�� ��� �Է�
	int allLen = resToPhone->getHeadLeng() + resToPhone->getBodyLeng();
	if(allLen > 0)
	{
		ACE_Message_Block *allmesg = MemoryPoolManager::instance()->alloc(allLen);

		allmesg->copy(resToPhone->getRawHeader()->rd_ptr(), resToPhone->getHeadLeng());
		if (resToPhone->getRawBody())
			allmesg->copy(resToPhone->getRawBody()->rd_ptr(), resToPhone->getBodyLeng());

		int resultEnqueue = enSendQueue(allmesg->rd_ptr(), allmesg->length());

		MemoryPoolManager::instance()->free(allmesg);

		if (resultEnqueue==0)
			sentDirectCnt++;
		else if (resultEnqueue==1)
			sentByQueCnt++;
		
		if(resultEnqueue >= 0)
		{
			PAS_TRACE2("sendRespToPhone_headbody, SEND PHONE RESP - %d bytes, %s", allLen, getMyInfo());
			PHTR_DEBUG2("ClientHandler::sendRespToPhone_headbody, SEND PHONE RESP - %d bytes, %s", allLen, getMyInfo());
		}

		// enQueue fail
		else
		{
			PAS_INFO1("sendRespToPhone_headbody, send fail - fd[%d]", get_handle());
			PHTR_INFO1("ClientHandler::sendRespToPhone_headbody, send fail - fd[%d]", get_handle());
			return -1;
		}
	}

	// no exist send data
	else 
	{
		PAS_INFO2("PHONE RESP size wrong - %d bytes, %s", allLen, getMyInfo());
		PHTR_INFO2("ClientHandler:: PHONE RESP size wrong - %d bytes, %s", allLen, getMyInfo());	
	}

	return 0;
}


/**
�ܸ��� Http Response �޽����� �۽��ϴ� ���̴�.
�������� socket send �� �ƴ϶� sendQueue  �� �ִ� ���̴�.
*/
int	ClientHandler::sendRespToPhone(HTTP::Response* resToPhone)
{
	// �۽�ť�� ��� �Է�
	const int headerLen = resToPhone->getRawHeader()->length();

	if (headerLen  <= 0)
	{
		PAS_ERROR2("ClientHandler:: header length error (%d), fd[%d]", headerLen, get_handle());
		PHTR_DEBUG2("ClientHandler:: header length error (%d), fd[%d]", headerLen, get_handle());
	}
	
	int resultEnqueue = enSendQueue(resToPhone->getRawHeader()->rd_ptr(), resToPhone->getRawHeader()->length());
	
	#ifdef HTTP_DUMP
	filedump->init("PHONE-RESP-HEAD", 4);
	filedump->write(resToPhone->getRawHeader()->rd_ptr(), resToPhone->getRawHeader()->length());
	#endif
	
	if(resultEnqueue < 0)
	{
		PAS_ERROR1("ClientHandler:: ���� ��� �۽� ����, fd[%d]", get_handle());
		PHTR_DEBUG1("ClientHandler:: ���� ��� �۽� ����, fd[%d]", get_handle());
		return -1;
	}
	else
	{
		PAS_DEBUG2("ClientHandler::SEND RESP HEAD - %d bytes, %s", headerLen, getMyInfo());
		PHTR_DEBUG2("ClientHandler::SEND RESP HEAD - %d bytes, %s", headerLen, getMyInfo());
	}

	// �۽�ť�� �ٵ� �Է�
	const int bodyLen = resToPhone->getBodyLeng();

	#ifdef HTTP_DUMP
	filedump->init("PHONE-RESP-BODY", 4);
	if (resToPhone->getRawBody() != NULL)
		filedump->write(resToPhone->getRawBody()->rd_ptr(), bodyLen);
	#endif
	
	if(bodyLen > 0)
	{
		ACE_ASSERT(resToPhone->getRawBody() != NULL);
		
		resultEnqueue = enSendQueue(resToPhone->getRawBody()->rd_ptr(), resToPhone->getRawBody()->length());
		if (resultEnqueue==0)
			sentDirectCnt++;
		else if (resultEnqueue==1)
			sentByQueCnt++;
		
		if(resultEnqueue < 0)
		{
			PAS_ERROR1("ClientHandler:: ���� �ٵ� �۽� ����, fd[%d]", get_handle());
			PHTR_DEBUG1("ClientHandler:: ���� �ٵ� �۽� ����, fd[%d]", get_handle());
			return -1;
		}
		else
		{
			PAS_DEBUG2("ClientHandler::SEND RESP BODY - %d bytes, %s", bodyLen, getMyInfo());
			PHTR_DEBUG2("ClientHandler::SEND RESP BODY - %d bytes, %s", bodyLen, getMyInfo());
		}
	}
	return 0;
}



/**
*/
int	 ClientHandler::getUserInfo(Transaction *tr)
{
	UserInfoMng *userInfoMng = UserInfoMng::instance();
	if (userInfo == NULL && tr->phoneNumber[0] != '\0')
	{
		userInfo = userInfoMng->searchByMdn(tr->phoneNumber, tr->phoneIpAddr);
		if (userInfo==NULL)
		{  // ó�� ������ ����̴�.
			PAS_DEBUG1("Here comes new user. PhoneNumber[%s]",  tr->phoneNumber);
			userInfo = userInfoMng->add(tr->phoneNumber, tr->phoneIpAddr);
			
			if (userInfo==NULL)
				return -1;

			userInfo->onClientConnection();
		}
		else
		{  // �ֱٿ�  ������  ���� �ִ� �����.
			// @todo �ֱ� ���� �ð� Ȯ��.
			// ������ ���� �� �ð� ����� ũ�� Auth,  Santa �����. 
			// @todo browser ���� ����Ǿ����� check
			PAS_TRACE1("UserInfo is already exist. Phone number is %s",  tr->phoneNumber);	
			userInfo->onClientConnection();

			// ù ���ӽÿ��� ���� ����� ������ ������ , ���� ����.
			session.setLastTransactionTime(userInfo->lastRespTime);
		}
	}
	else
	{
		userInfo->updateReqTime();
	}
	
	return 0;

}

/**
*/
int	 ClientHandler::getUserInfo_SSL()
{
	UserInfoMng *userInfoMng = UserInfoMng::instance();
	if (userInfo == NULL)
	{
		userInfo = userInfoMng->searchByAddr(session.getNumber(), session.getIpAddr());
		if (userInfo==NULL)
		{  // ó�� ������ ����̴�.
			PAS_DEBUG1("UserInfoMng:: New SSL User %s",  session.getIpAddr());
			userInfo = userInfoMng->add(session.getNumber(), session.getIpAddr());
			
			if (userInfo==NULL)
				return -1;

			userInfo->onClientConnection();
			
		}
		else
		{  // �ֱٿ�  ������  ���� �ִ� �����.
			// @todo �ֱ� ���� �ð� Ȯ��.
			// ������ ���� �� �ð� ����� ũ�� Auth,  Santa �����. 
			// @todo browser ���� ����Ǿ����� check
			PAS_DEBUG2("UserInfoMng:: Old User (SSL) %s, %s",  userInfo->getPhoneNumber(), session.getIpAddr());

			userInfo->copySession(&session);
			userInfo->onClientConnection();
			
			// ù ���ӽÿ��� ���� ����� ������ ������ , ���� ����.
			session.setLastTransactionTime(userInfo->lastRespTime);
		}
	}
	else
	{
		userInfo->updateReqTime();
	}
	
	return 0;

}


/**
�ܸ��� ��û ������� ���� ���. �� Transaction �� �Ҵ�.
�������� Transaction �� ���۵� ���� �ƴϴ�.. 
*/
int ClientHandler::allocTransaction()
{
	ACE_ASSERT(currTransaction == NULL);
	

	currTransaction = new Transaction();
	currHttpRequest = currTransaction->getRequest();
	currTransaction->setLog(tracelog);

	transactionQueue.push(currTransaction);
	
	numTransactionAlloc++;
	numRequests++;
	
	PAS_TRACE2("Alloc Transaction (%d) [%X]", numTransactionAlloc, currTransaction);

	PAS_TRACE2("%s alloc TR[%d]", setMyInfo(), currTransaction->id());
	PHTR_DEBUG2("%s alloc TR[%d]", getMyInfo(), currTransaction->id());
	return 0;
}

/**
flag_pos == FREE_FRONT, FREE_BACK
tr_to_free == NULL or transaction-to-delete

tr_to_free �� null �� �ƴϸ� ��ġ�ϴ� ��쿡�� free.

�ܸ� requst �޽����� �̻��� ���, �ٷ� freeTransaction() �� �ϴµ�,
front ���� �ϸ� �ȵǰ�, back ���� free �Ͽ��� �Ѵ�.
�ʵ� ���뿡�� �߰��� ���� ������ ����.
2006.12.16 ����.
*/
void ClientHandler::freeTransaction(int flag_pos, Transaction *tr_to_free)
{
	if (transactionQueue.empty())
	{
		PAS_INFO2("freeTransaction() called too many times [%X] [%s]", tr_to_free, setMyInfo());
		PHTR_INFO2("freeTransaction() called too many times [%X] [%s]", tr_to_free, getMyInfo());
		return;
	}
		
	Transaction *todelete = NULL;

	int popResult = 0;
	
	if (flag_pos == FREE_FRONT)
	{
		if (tr_to_free != NULL)
		{
			// tr_to_free �� null �� �ƴϸ� ��ġ�ϴ� ��쿡�� free.
			if (transactionQueue.front()==tr_to_free)
				popResult = transactionQueue.pop_front(todelete);
		}
		else			
			popResult = transactionQueue.pop_front(todelete);
	}
	
	else if (flag_pos == FREE_BACK)
	{
		if (tr_to_free != NULL)
		{
			// tr_to_free �� null �� �ƴϸ� ��ġ�ϴ� ��쿡�� free.
			if (transactionQueue.back()==tr_to_free)
				popResult = transactionQueue.pop_back(todelete);
		}
		else			
			popResult = transactionQueue.pop_back(todelete);
	}
	
	else
	{
		PAS_NOTICE1("ERROR freeTransaction()  flag wrong [%s]", setMyInfo());
		PHTR_NOTICE1("ERROR freeTransaction() flag wrong [%s]", getMyInfo());
		return;
	}
	
	if( popResult == -1 )
	{
		PAS_NOTICE2("ERROR Transaction Q pop() failed, TR [%X] [%s]", tr_to_free, setMyInfo());
		PHTR_NOTICE2("ERROR Transaction Q pop() failed, TR [%X] [%s]", tr_to_free, getMyInfo());
		return;
	}

	if (numTransactionAlloc > 0)
	{
		numTransactionAlloc--;
		numResponses++;
	}

	PAS_TRACE3("ClientHandler::Free Transaction - Deleted=[%X], ToFree=[%X] Current=[%X]", 
		todelete, tr_to_free,  currTransaction);			
	PHTR_DEBUG3("ClientHandler:: Free Transaction - Deleted=[%X], ToFree=[%X] Current=[%X]", 
		todelete, tr_to_free,  currTransaction);	
		
	PAS_TRACE2("ClientHandler::Free Transaction -Qsize=%d TrAlloc=%d", transactionQueue.size(), numTransactionAlloc);
	PHTR_DEBUG2("ClientHandler::Free Transaction -Qsize=%d TrAlloc=%d", transactionQueue.size(), numTransactionAlloc);

	if (tr_to_free != NULL && todelete != tr_to_free)
		PAS_NOTICE1("ClientHandler:: Free Transaction - ERROR todelete != tr_to_free %s", getMyInfo());
		
	if(todelete == currTransaction)
	{
		currTransaction = NULL;
		currHttpRequest = NULL;
	}


	delete todelete;

}



/**
�ܸ��� ��û ���/�ٵ� ��ü ���� ���� ���.
�������� Transaction �� ���۵� ���̴�. Transaction Q �� �߰��Ѵ�.
*/
int ClientHandler::startTransaction(Transaction *tr)
{
	ACE_ASSERT(tr != NULL);

	preStartTransaction(tr);

	HTTP::Request *pRequest = tr->getRequest();
			
	// santa ó���� �Ϸ� �Ǿ��ų�, santa ���� CP�� ����Ǵ� ��쿡
	// �� �ٸ� Request (�ܸ� )�� ó���� �� �ִ� ���°� �ȴ�.
	changeState ( CS_WAIT_REQUEST_HEADER );

	// increase statistic
	pSysStat->clientRequest(1);
	if(pRequest->getContentLength() > 0)
		pSysStat->clientUpload(1);

	if(pRequest->getHeader()->getHost().isEmpty())
	{
		PAS_NOTICE("Host in request header is empty.");
		PAS_NOTICE_DUMP("Request Header", pRequest->getRawHeader()->rd_ptr(), pRequest->getHeadLeng());
		return -1;
	}

	// send mwatch (mIDC system monitoring)
	WatchReporter::IncreaseMsgWatchSvr();

	// pasmonitor (logsvr, KUNLOG) �� 1 �� ���� ȣ��. �����δ� 20�� ���� ����.
	MonitorReporter::increasePasMon();

	session.setThreadId();
	
	// HTTP ��� �Ľ�, ������� tr �� ����
	tr->setTransactionInfo();

	this->tracelog = PhoneTraceMng::instance()->getTraceLog(tr->phoneNumber);
	PAS_TRACE1("tracelog[%X]", tracelog);

	PHTR_HEXDUMP(pRequest->getRawHeader()->rd_ptr(), pRequest->getHeadLeng(), "PHONE REQ HEAD");
	if (pRequest->getRawBody() != NULL) 
	{
		int dump_len = MIN(pRequest->getBodyLeng(), 160);
		PHTR_HEXDUMP(pRequest->getRawBody()->rd_ptr(), dump_len, "PHONE REQ BODY");
	}
			
	// session ���� ����, seqNum ���� ���� �۾� ����.
	session.beginTransaction(tr);
	session.setCPname(tr);

	if (getUserInfo(tr) < 0)
	{
		PAS_ERROR("Memory full: cannot alloc userinfo");
		PHTR_ERROR("Memory full: cannot alloc userinfo");
		return -1;
	}

	
	// ����� ���� ���
	//session.print();
	
	// request�� �ܸ��κ��� ���� ����� ���� ó��
	tr->recvPhoneReq();

	 // Ȥ�� ���� ������ ��쿡�� �ð����� ����� ��µǰ� �ϱ� ���� ���⼭ ����. 
	 // CpHandler ���� ���� ����Ǵ� ��쿡 �ٽ� ������Ʈ �ϹǷ� ���⼭ �ߺ��ؼ� �ҷ��൵ �ȴ�.
	tr->connectCp(); 
	tr->sendCpReq();

	// PAS ���� �α� ����
	paslog->startOfTransaction(userInfo, &session, tr);
	
	if (session.isFirst()) // ù transaction
	{
		// session �� ��ȭ��ȣ  �� ���� ���� ����.
		session.setFirstInfo(tr);

		if (userInfo)
			userInfo->storeSession(&session);
	}
	else
	{
		// �����ȣ�� �ƴϸ鼭,  ������ session �� ����� ��ȭ��ȣ�� �� transaction �� ��ȭ��ȣ�� �ٸ� ��� �α� ���.
		if ( ! session.isSantaNumber &&
			strcmp(session.getNumber(), tr->phoneNumber) != 0)
		{
			PAS_WARNING3("Session[%s], Transaction[%s]. �̻��� �����. ���� �� ��ȭ��ȣ ���� - %s", 
				session.getNumber(), tr->phoneNumber, setMyInfo());
			PHTR_WARN3("Session[%s], Transaction[%s]. �̻��� �����. ���� �� ��ȭ��ȣ ���� - %s",
				session.getNumber(), tr->phoneNumber,  getMyInfo());
		}
	}

	if(Config::instance()->process.browserTypeCheck)
	{
		if(browserTypeCheck(tr) < 0)
			return 0;
	}
	
	if( santaHandler.getState() == SS_NONE) 
	{
		// IMSI �˻�, Santa ����
		// MNC ���� 04, 08 �� �ƴ� ��� (�Ƹ��� 00) �� ���� ��ȣ�뿪���� �˻��ϰ� 
		// Santa ������ �����Ѵ�. -- 2006.10.13

		tr->santaStartTime = ACE_OS::gettimeofday();
		checkSanta(tr);
		tr->santaEndTime = ACE_OS::gettimeofday();

		// �Ʒ��� �α׸� ���� �߰��� ��.
		if(santaHandler.isPassed())
		{
			PAS_DEBUG2("NO SANTA required - %s  TR[%d]", getMyInfo(), tr->id());
			PHTR_DEBUG2("NO SANTA required - %s  TR[%d]", getMyInfo(), tr->id());
		}
	}
		
	// ������ �޾ƾ� �Ѵ�.
	if( santaHandler.getState() == SS_REQUESTED )
	{
		PAS_DEBUG2("SANTA requested - %s  TR[%d]", getMyInfo(), tr->id());
		PHTR_DEBUG2("SANTA requested - %s  TR[%d]", getMyInfo(), tr->id());
		changeState( CS_SANTA_WAITING);
		return 0;
	}

	// Santa ������ �ٷ�  ������  ���: ���� ���� �� �ִ� ��Ȳ�� �ƴϹǷ� ���� �޽��� ���
	else if( santaHandler.getState() == SS_FAILED )
	{
		PAS_DEBUG2("SANTA failed - %s  TR[%d]", getMyInfo(), tr->id());
		PHTR_DEBUG2("SANTA failed - %s  TR[%d]", getMyInfo(), tr->id());
		
		// SANTA ��ȸ ���� ������ ����
		ResponseBuilder::SantaFailed( tr->getResponse() );

		// Over10 �α� ����� ���� ���� �ڵ�
		tr->setErrorRespCode( RESCODE_SANTA );

		afterCpTransaction(tr);

		return 0;
	}
	else if(santaHandler.isPassed())
	{
		// Santa  ������ �ʿ� ���� ���� �Ʒ����� ó���ȴ�.
		// santa ������ ���� ��� �ٷ� midTransaction() ����

		midTransaction(tr);
	}
	

	return 0;
}

GuideCode ClientHandler::getFimmSpecificGuideCode(const url_t& url)
{
	// Fimm ���� �ȳ������� ��û�� ��� ������ ���� URL �� ���ŵȴ�.
	// "http://ktfproxy.magicn.com:9090/?reqURL=fimm.co.kr"

	url_t schemeHostPort("http://");
	schemeHostPort += Config::instance()->network.kunHost;
	if(Config::instance()->network.listenPort != 80)
	{
		schemeHostPort += ":";
		schemeHostPort += Config::instance()->network.listenPort;
	}

	if(url.incaseFind(schemeHostPort + "/?reqURL=") == 0) 
		return GCODE_Fimm;
	else
		return GCODE_Unknown;
}

url_t ClientHandler::getReqURL(const url_t& url)
{
	// Fimm ���� �ȳ������� ��û�� ��� ������ ���� URL �� ���ŵȴ�.
	// "http://ktfkunproxy.magicn.com:9090/?reqURL=fimm.co.kr"

	int pos = url.incaseFind("reqURL=");
	if(pos < 0)
		return url_t();

	return url.substr(pos+7);
}

int ClientHandler::procGuide(Transaction* tr)
{
	PAS_TRACE("Check for guide page.");
	PHTR_DEBUG("Check for guide page.");

	// Sisor���� ��� Ű�� MDN �̹Ƿ�, MDN�� �� �� ���� ��쿡�� 
	// Sisor�� ��� �� �� ����. �׷��Ƿ� �ȳ������� ǥ�� ���θ�
	// Ȯ������ �ʰ�, ������ ǥ������ �ʴ� ������ �Ѵ�.
	if(tr->MDN[0] == '\0')
	{
		PAS_NOTICE("Skip guide checking. Because not found MDN.");
		PHTR_NOTICE("Skip guide checking. Because not found MDN.");
		return 0; // pass
	}

	try
	{
		// �ȳ������� skip �� �����ߴ�.
		if(procSetSkipGuide(tr) == 1)
			return 1; // redirected

		// Fimm Specific �ȳ��������� ǥ������ ����
		if(procFimmSpecificShowGuidePage(tr) == 1)
			return 1; // redirected

		// �ȳ� �������� ǥ���ϵ��� �ܸ�����ڸ� �ȳ��������� redirect ���״�.
		if(procShowGuidePage(tr) == 1)
			return 1; // redirected
	
		url_t kunSchemeHost("http://");
		kunSchemeHost += Config::instance()->network.kunHost;
		if(tr->getRequest()->getHeader()->getHost().incaseFind(kunSchemeHost) == 0)
		{
			//--------
			// ����
			//--------
			// PAS ���� Ư�� �۾��� ��û�ϱ� ���� URL �̶��, ��û�۾��� ó�� �� �׻�
			// �ٸ� URL �� redirect �Ǿ�� �Ѵ�.
			// �� ��û ó�� �۾����� redirect ���� �ʾҴٴ� ����, �۾� ���߿� ������ 
			// �߻������� �ǹ��Ѵ�.
			// ��� �����ϸ� ���ѷ����� �߻��ϹǷ� �۾��� �ߴ��Ѵ�.

			sendHTTPForbidden(tr);
			PAS_INFO1("Access forbidden for %s", tr->getRequest()->getHeader()->getHost().toStr());
			return 1; // stop, because error occur
		}
	}
	catch (Exception e)
	{
		PAS_NOTICE1("Guide page skip. Because of %s", e.toString());
		PHTR_NOTICE1("Guide page skip. Because of %s", e.toString());
	}

	return 0;
}

void ClientHandler::sendHTTPForbidden(Transaction* tr)
{
	ResponseBuilder::Forbidden(tr->getResponse());
	tr->setErrorRespCode(RESCODE_URL_INVALID);

	afterCpTransaction(tr);
}

int ClientHandler::procShowGuidePage(Transaction* tr)
{
	PAS_TRACE("Check for ShowGuidePage");
	PHTR_DEBUG("Check for ShowGuidePage");

	HTTP::value_t strCounter = tr->getRequest()->getHeader()->getElement("COUNTER");
	if(strCounter.isEmpty())
	{
		PAS_INFO("Skip guide checking. Can't find COUNTER field in Http Request Header.");
		PHTR_INFO("Skip guide checking. Can't find COUNTER field in Http Request Header.");
		return 0; // pass
	}
	else
	{
		// ù ��û�� ���� �ȳ������� ǥ�� ���θ� �Ǵ��ϰ�
		// ó���� �ƴ� ���� ������ �ȳ��������� ǥ�� ���� �ʴ´�.
		if(strCounter.toInt() > 1)
		{
			PAS_DEBUG("Skip guide page. Because this is not first request.");
			PHTR_DEBUG("Skip guide page. Because this is not first request.");
			return 0; // pass
		}
	}

	// Sisor���� ��� Ű�� MDN �̹Ƿ�, MDN�� �� �� ���� ��쿡�� 
	// Sisor�� ��� �� �� ����. �׷��Ƿ� �ȳ������� ǥ�� ���θ�
	// Ȯ������ �ʰ�, ������ ǥ������ �ʴ� ������ �Ѵ�.
	if(tr->MDN[0] == '\0')
	{
		PAS_NOTICE("Skip guide checking. Because not found MDN.");
		PHTR_NOTICE("Skip guide checking. Because not found MDN.");
		return 0; // pass
	}

	try
	{
		url_t reqURL = tr->getRequest()->getHeader()->getUrl();
		GuideReadResponse res = getSkipGuide(tr, tr->MDN, reqURL);

		// redirect to guide page
		if(res.skipGuide == false)
		{
			PAS_DEBUG1("Show guide page. MDN[%s]", tr->MDN);
			PHTR_DEBUG1("Show guide page. MDN[%s]", tr->MDN);

			int redirectResult = redirectToGuidePage(tr, res.guideCode);

			// redirect �� ���� �ߴٸ�, �ȳ������� ǥ�ø� �����ϰ� �׳� ��� �����Ѵ�.
			if(redirectResult < 0)
				return 0;

			// redirect �� �����Ͽ����Ƿ�, �߰� ������ �ߴ��Ѵ�.
			else
				return 1; // redirected
		}

		// skip
		else
		{
			PAS_DEBUG("Guide page skip. Because skipGuide is on.");
			PHTR_DEBUG("Guide page skip. Because skipGuide is on.");
			return 0;
		}		
	}
	catch (Exception e)
	{
		// ��ֽÿ��� �ȳ��������� ������ �������� �ʰ�, �׳� ����Ѵ�.

		PAS_NOTICE2("Guide page skip. Because of %s. MDN[%s]", e.toString(), tr->MDN);
		PHTR_NOTICE2("Guide page skip. Because of %s. MDN[%s]", e.toString(), tr->MDN);

		return 0;
	}
}

int ClientHandler::procFimmSpecificShowGuidePage(Transaction* tr)
{
	PAS_TRACE("Check for FimmSpecificShowGuidePage");
	PHTR_DEBUG("Check for FimmSpecificShowGuidePage");

	// Fimm ���� �ȳ������� ��û�� ��� ������ ���� URL �� ���ŵȴ�.
	// "http://ktfkunproxy.magicn.com:9090/?reqURL=fimm.co.kr"

	try
	{
		PAS_TRACE2("ReqHost[%s] KunHost[%s]", 
			tr->getRequest()->getHeader()->getHost().toStr(),
			Config::instance()->network.kunHost.toStr());

		if(tr->getRequest()->getHeader()->getHost() != Config::instance()->network.kunHost)
		{
			PAS_TRACE("Skip guide page. Because Host in header is not kun host");
			PHTR_DEBUG("Skip guide page. Because Host in header is not kun host");
			return 0; // pass
		}

		HTTP::value_t strCounter = tr->getRequest()->getHeader()->getElement("COUNTER");
		if(strCounter.isEmpty())
		{
			PAS_INFO("Skip guide page. Because can't find COUNTER field in Http Request Header.");
			PHTR_INFO("Skip guide page. Because Can't find COUNTER field in Http Request Header.");
			return 0; // pass
		}

		// ù ��û�� ���� �ȳ������� ǥ�� ���θ� �Ǵ��ϰ�
		// ó���� �ƴ� ���� ������ �ȳ��������� ǥ�� ���� �ʴ´�.
		// Fimm ������������ ��� Fimm ������ ���ٰ� PAS �� redirect 
		// �Ǵ� ������ �Ǵٸ� �߰����� ������ ������ ���� COUNTER ���� ó���� 1 �� �ƴϴ�.
		PAS_DEBUG2("COUNTER[%d] in Http Header, FimmSpecificFirstCount[%d] in Config", strCounter.toInt(), Config::instance()->guide.fimmSpecificFirstCounter);
		if(strCounter.toInt() > Config::instance()->guide.fimmSpecificFirstCounter)
			return redirectToReqURL(tr);

		url_t reqURL = tr->getRequest()->getHeader()->getUrl();
		url_t redirectURL = getReqURL(reqURL);

		if (redirectURL.isEmpty() == true)
			return 0; // pass

		GuideReadResponse res = getSkipGuide(tr, tr->MDN, redirectURL);

		// redirect to guide page
		if(res.skipGuide == false)
		{
			PAS_INFO("Show guide page. Because SkipGuide is off.");
			PHTR_INFO("Show guide page. Because SkipGuide is off.");

			redirectToFimmSpecificGuidePage(tr);
			return 1; // redirected
		}

		// skip => ��û�� �������� redirect�� �����ش�.
		else
		{
			PAS_INFO("Guide page skip. Because SkipGuide is on.");
			PHTR_INFO("Guide page skip. Because SkipGuide is on.");

			return redirectToReqURL(tr);
		}		
	}
	catch (Exception e)
	{
		// ��ֽÿ��� �ȳ��������� ������ �������� �ʰ�, �׳� ����Ѵ�.

		PAS_NOTICE1("Skip guide page. Because of %s", e.toString());
		PHTR_NOTICE1("Skip guide page. Because of %s", e.toString());

		return redirectToReqURL(tr);
	}
}

GuideReadResponse ClientHandler::getSkipGuide(Transaction* tr, const MDN& mdn, const url_t& reqURL )
{
	SisorProxy* pSisor = NULL;

	try
	{
		tr->guideStartTime = ACE_OS::gettimeofday();

		// ����
		GuideReadRequest req;
		req.mdn = mdn;
		req.reqURL = reqURL;

		PAS_TRACE3("GetSkipGuide >> Request : MDN[%s] URL[%s] Host[%s]", mdn.toString().toStr(), reqURL.toStr(), req.reqURL.toStr());
		PHTR_DEBUG3("GetSkipGuide >> Request : MDN[%s] URL[%s] Host[%s]", mdn.toString().toStr(), reqURL.toStr(), req.reqURL.toStr());

		pSisor = SisorQueue::instance()->get();
		GuideReadResponse res = pSisor->query(req);
		SisorQueue::instance()->put(pSisor);

		PAS_TRACE3("GetSkipGuide >> Response : MDN[%s], GuideCode[%d], skipGuide[%d]", 
			res.mdn.toString().toStr(), static_cast<int>(res.guideCode), static_cast<int>(res.skipGuide));
		PHTR_DEBUG3("GetSkipGuide >> Response : MDN[%s], GuideCode[%d], skipGuide[%d]", 
			res.mdn.toString().toStr(), static_cast<int>(res.guideCode), static_cast<int>(res.skipGuide));	

		tr->guideEndTime = ACE_OS::gettimeofday();

		writeGuideCommunicationLog(tr->guideEndTime - tr->guideStartTime, res.guideCode);

		return res;
	}
	catch (Exception e)
	{
		if(pSisor != NULL)
			delete pSisor;

		PAS_NOTICE1("Query to SISOR is fail. Because of %s. Force to skip guide page.", e.toString());

		GuideReadResponse res;
		res.guideCode = GCODE_Unknown;
		res.skipGuide = true;
		return res;
	}
}

int ClientHandler::procSetSkipGuide(Transaction* tr)
{
	PAS_TRACE("Check for SetSkipGuide.");

	// Sisor���� ��� Ű�� MDN �̹Ƿ�, MDN�� �� �� ���� ��쿡�� 
	// Sisor�� ��� �� �� ����. �׷��Ƿ� �ȳ������� ǥ�� ���θ�
	// Ȯ������ �ʰ�, ������ ǥ������ �ʴ� ������ �Ѵ�.
	if(tr->MDN[0] == '\0')
	{
		PAS_NOTICE("Skip procSetSkipGuide. Because MDN is empty.");
		PHTR_NOTICE("Skip procSetSkipGuide. Because MDN is empty.");
		return 0; // pass
	}

	url_t reqURL = tr->getRequest()->getHeader()->getUrl();
	GuideCode gcode = getSkipGuideCodeFromURL(reqURL);
	
	if(gcode == GCODE_Unknown)
	{
		PAS_TRACE("Request is not SetSkipGuide.");
		PHTR_DEBUG("Request is not SetSkipGuide.");
		return 0; // pass
	}

	// Sisor���� skip ���� on ��Ű���� �˸���, 
	// �ܸ�����ڸ� �� ������ ù�������� redirect ��Ų��.
	try
	{
		bool resultOfSetSkipGuide = setSkipGuide(tr, tr->MDN, gcode);	
		if(resultOfSetSkipGuide == false)
			ThrowException(ECODE_UNKNOWN);

		redirectToGuideResultPage(tr, gcode);
	}
	catch (Exception e)
	{
		PAS_INFO1("SetSkipGuide fail. Because of %s", e.toString());
		PHTR_INFO1("SetSkipGuide fail. Because of %s", e.toString());

		redirectToReqURL(tr);
	}
	
	return 1; // redirected
}

bool ClientHandler::setSkipGuide(Transaction* tr, const MDN& mdn, const GuideCode& gcode )
{
	// MDN�� ������ �ְų� gcode �� ������ �ִٸ�, �ƹ��͵� ���Ѵ�.
	if(gcode == GCODE_Unknown || mdn.valid() == false)
	{
		PAS_NOTICE2("Can't set skip guide. Because of invalid gcode[%d] or invalid MDN[%s].", gcode, mdn.toString().toStr());
		PHTR_NOTICE2("Can't set skip guide. Because of invalid gcode[%d] or invalid MDN[%s].", gcode, mdn.toString().toStr());
		return false;
	}

	GuideWriteRequest req;
	req.mdn = mdn;
	req.guideCode = gcode;
	req.skipGuide = true;

	PAS_INFO2("Set SkipGuide. MDN[%s] GuideCode[%d]", mdn.toString().toStr(), static_cast<int>(gcode));
	PHTR_INFO2("Set SkipGuide. MDN[%s] GuideCode[%d]", mdn.toString().toStr(), static_cast<int>(gcode));

	GuideWriteResponse res;

	tr->guideStartTime = ACE_OS::gettimeofday();

	SisorProxy* pSisor = NULL;
	
	try
	{
		pSisor = SisorQueue::instance()->get();
		res = pSisor->query(req);
		SisorQueue::instance()->put(pSisor);
	}
	catch (Exception e)
	{
		if(pSisor != NULL)
			delete pSisor;

		PAS_NOTICE1("Guide write fail. Because of %s.", e.toString());

		res.mdn = req.mdn;
		res.resultState = false;
	}

	tr->guideEndTime = ACE_OS::gettimeofday();

	if(res.mdn != req.mdn)
	{
		PAS_NOTICE2("GuideWriteRequest MDN[%s] is miss match with GuideWriteResponse MDN[%s].", 
			req.mdn.toString().c_str(), res.mdn.toString().c_str());
		PHTR_NOTICE2("GuideWriteRequest MDN[%s] is miss match with GuideWriteResponse MDN[%s].", 
			req.mdn.toString().c_str(), res.mdn.toString().c_str());
	}

	if(res.resultState == false)
	{
		PAS_NOTICE("GuideWriteRequest is fail.");
		PHTR_NOTICE("GuideWriteRequest is fail.");
	}

	writeGuideCommunicationLog(tr->guideEndTime - tr->guideStartTime, gcode);

	return res.resultState;
}

GuideCode ClientHandler::getSkipGuideCodeFromURL(const url_t& reqURL)
{
	PAS_TRACE1("ClientHandler::getSkipGuideCodeFromURL >> ReqURL[%s]", reqURL.toStr());
	PHTR_DEBUG1("ClientHandler::getSkipGuideCodeFromURL >> ReqURL[%s]", reqURL.toStr());

	if(reqURL.incaseFind("/?GuideCode=MagicN&SkipGuide=1") > 0)
	{
		return GCODE_MagicN;
	}
	else if(reqURL.incaseFind("/?GuideCode=Fimm&SkipGuide=1") > 0)
	{
		return GCODE_Fimm;
	}
	else if(reqURL.incaseFind("/?GuideCode=MultiPack&SkipGuide=1") > 0)
	{
		return GCODE_MultiPack;
	}

	PAS_TRACE1("Can't recognize GuideCode from Request URL[%s]", reqURL.toStr());

	return GCODE_Unknown;
}

int ClientHandler::redirectToGuidePage(Transaction* tr, const GuideCode gcode)
{
	url_t reqURL = tr->getRequest()->getHeader()->getUrl();
	url_t redirectURL = getGuideURL(gcode, reqURL);
	ResponseBuilder::Redirect(tr->getResponse(), redirectURL);

	afterCpTransaction(tr);

	return 0;
}

int ClientHandler::redirectToFimmSpecificGuidePage(Transaction* tr)
{
	url_t reqURL = tr->getRequest()->getHeader()->getUrl();
	url_t redirectURL = getReqURL(reqURL);
	redirectURL = getGuideURL(GCODE_Fimm, redirectURL);
	ResponseBuilder::Redirect(tr->getResponse(), redirectURL);

	afterCpTransaction(tr);

	return 0;
}

int ClientHandler::redirectToGuideResultPage( Transaction *tr, const GuideCode gcode)
{
	url_t reqURL = tr->getRequest()->getHeader()->getUrl();
	reqURL = getReqURL(reqURL);

	PAS_INFO1("RedirectURL[%s]", getGuideResultURL(gcode, reqURL).toStr());
	ResponseBuilder::Redirect(tr->getResponse(), getGuideResultURL(gcode, reqURL));

	afterCpTransaction(tr);

	return 0;
}

/**
Santa, ó�� ���� ���� ����.
*/
void ClientHandler::midTransaction(Transaction *tr)
{
	ACE_ASSERT(tr != NULL);

	const Config* pConfig = Config::instance();

	// �����ȣ �ܸ���� SANTA ���� ���� ����ȣ�� ����� ������ �Ѵ�.
	// �ڵ��� ��ȣ �ּ��ڸ� �� 10�ڸ� �˻�
	if(session.isSantaNumber)
	{
		tr->setSantaResult(session.phoneNumber, session.IMSI);

		if (session.isFirst()) // ù transaction
			userInfo->set(tr->phoneNumber, tr->phoneIpAddr); // 2006.12.14 
	}

	int checkResult = checkAuth(tr);
	if(checkResult < 0)
	{
		PAS_NOTICE2("AUTH failed TR[%d] %s", tr->id(), getMyInfo());
		PHTR_NOTICE2("AUTH failed TR[%d] %s", tr->id(), getMyInfo());
		// Auth ���� ���н� ���� �޽��� ����
		ResponseBuilder::AuthFailed( tr->getResponse() );
		tr->setErrorRespCode( RESCODE_AUTH ); // Over10 �α� ����� ���� ���� �ڵ�
		afterCpTransaction(tr);
		return;
	}

	PAS_TRACE2("AUTH OK TR[%d] %s", tr->id(), getMyInfo());
	PHTR_DEBUG2("AUTH OK TR[%d] %s", tr->id(), getMyInfo());

	// �� �ѹ� ����
	if(pConfig->hotnumber.enable)
		applyHotNumber(tr);

	// ACL �������� ȣ��Ʈ�� ���.
	tr->setCpConnInfo_first();

	// ACL ����
	if(pConfig->acl.enable)
	{
		if(procACL(tr) < 0)
			return;
	}

	// �ȳ�������
	if(pConfig->guide.enable)
	{
		if(procGuide(tr) == 1)
			return; // �ȳ��������� ���õ� �۾��� �����Ƿ�, ���̻� ������ ���� �ʴ´�.
	}

	// ���� ����
	if(pConfig->service.enable)
	{
		if(procService(tr))
			return;
	}

	char notimesg[512];
	if(checkStatFilter(tr, notimesg))
	{
		PAS_DEBUG1("StatFilter Blocked %s ", session.getNumber());
		PHTR_DEBUG1("StatFilter Blocked %s", session.getNumber());
		
		tr->setErrorRespCode(RESCODE_STATFILTER);
		ResponseBuilder::StatFilterBlocked(tr->getResponse(), notimesg, strlen(notimesg));
		afterCpTransaction(tr);
		return;
	}

	tr->setCpConnInfo_second();

	// ����� �߰� ���� �Է�
	if(additionalInfoToReqHeader(tr) < 0)
	{
		PAS_INFO("Fail additional info insert to header.");
		return;
	}

	// CP���� ������ ��û
	PAS_DEBUG("Request to CPProxy.");
	PHTR_DEBUG("Request to CPProxy.");
	requestToCP(tr);
}

/**
CP ���� ���� ó�� ����. HashKey ���� �߰�. 
CP ��û�� ���� ���� ��쿡�� ���⸦ ��ġ�� �Ѵ�.
*/
void ClientHandler::afterCpTransaction(Transaction *tr)
{
	ACE_ASSERT(tr != NULL);

	preAfterCpTransaction(tr);

	tr->recvCpResp();
	tr->setCpTime(); // CP ���� ���� ���� ���� ��쿡�� �ð��� �����ϱ� ����.
	tr->setDone();

	HTTP::Request* pReq = tr->getRequest();
	HTTP::Response* pRes = tr->getResponse();

	ACE_ASSERT(pReq != NULL);
	ACE_ASSERT(pRes != NULL);

	const HTTP::RequestHeader* h = pReq->getHeader();
	
	// ����� �߰����� ���� ���
	HTTP::ResponseHeader modifyHeader = additionalInfoToResHeader((*pRes->getHeader()), tr->phoneNumber, tr->orgUrl, 
		tr->cpReqBytes, pRes->getContentLength(), h->getHost(), tr->correlationKey);
	pRes->setHeader(modifyHeader);
	
	//PAS_DEBUG_DUMP("PHONE RESP HEAD", resTrResponse->getRawHeader()->rd_ptr(), resTrResponse->getHeadLeng());
	PHTR_HEXDUMP(pRes->getRawHeader()->rd_ptr(), pRes->getHeadLeng(), "PHONE RESP HEAD");
	
	if (pRes->getRawBody()) {
		int dump_len = MIN(pRes->getBodyLeng(), 160);
		//PAS_TRACE_DUMP("PHONE RESP BODY", pRes->getRawBody()->rd_ptr(), dump_len);
		PHTR_HEXDUMP(pRes->getRawBody()->rd_ptr(), dump_len, "PHONE RESP BODY");
	}

	// �ܸ������� ���� ���� 
	finishTransaction(tr);
}


/**
CP�� ������ �ްų� ���� ��Ȳ���� Transaction �� ����Ǵ� ��� ó��.
Transaction �ϷḦ �����Ѵ�. ���������̴� ó��.
Q�� front()�� ��ġ�ϸ� �ٷ� �ܸ��� ����. �ƴϸ� ����. front()�� ó���� �� Q �� ���� �ִ� ���� ������ ���� ó��.
*/
int ClientHandler::finishTransaction(Transaction *tr)
{
	ACE_ASSERT(tr != NULL);

	session.setCPname(tr);

	if (userInfo)
		userInfo->storeSession(&session);
			
	if (transactionQueue.empty())
	{
		PAS_NOTICE1("Transaction finished with NO Q: %s", getMyInfo());
		PHTR_NOTICE1("Transaction finished with NO Q: %s", getMyInfo());

		// let's just send the result to phone
	}
	else
	{
		if (transactionQueue.front() != tr)
		{
			// ���߿� ������ �Ѵ�.
			PAS_DEBUG3("Transaction finish delayed - TR[%d], front TR[%d], %s", 
				tr->id(), transactionQueue.front()->id(), getMyInfo());
			PHTR_DEBUG3("Transaction finish delayed - TR[%d], front TR[%d], %s", 
				tr->id(), transactionQueue.front()->id(), getMyInfo());
			return -1;
		}
	}

	while (true)
	{
		HTTP::Response *response = tr->getResponse();

		PAS_DEBUG2("Transaction finished. TRID[%d] %s", tr->id(), setMyInfo());
		PHTR_DEBUG2("Transaction finished. TRID[%d] %s", tr->id(), getMyInfo());
		
		// response t�� CP �κ��� ����  ����� ���� ó��
		tr->sendPhoneResp();

		// �ܸ��� ���� ������		
		// ��Ʈ������ ���� ���⼭ �۽����� �ʰ�, ���� ��� �ǽð����� �۽��Ѵ�.
		if( !tr->streaming() )
		{
			//sendRespToPhone( response ); // -- head/body �и� ���� ���
			sendRespToPhoneHeadbody(response); // -- head/body ���� ���� ��� -- �ּҷ� �׽�Ʈ���� ���� �߻�.
		}

		// increase statistic
		pSysStat->clientResponse(1);	

		#ifdef HTTP_LOG
		logHttpResp("[PHONE RESP]", tr, false);
		#endif
		
		// Transaction ����� ���� ���� ó��.
		session.endTransaction(tr);

		if (userInfo)
			userInfo->updateRespTime();
			
		// PAS ���� �α� ����
		paslog->endOfTransaction(userInfo, &session,  tr);
		
		// �α� �ۼ� �Ŀ� 
		session.setLastTransactionTime();		

		if (tr->connCloseRequested)
		{
			//!!! opera browser ���ε�ÿ� �ܸ� ��û ����� Connection: close �� ���Ե� ���.
			requestClose(); // �θ� Ŭ���� �Լ� -- PasHandler
			PAS_INFO1("Connection:Close - Request Close  [%s]", tr->phoneNumber);
		}

		// �Ϸ�� transaction �� Q ���� ����.
		freeTransaction(FREE_FRONT, tr);

		// �Ϸ�Ǿ�����  ���������̴� ������ Queue �� ���� �ִ� �������  ó���Ѵ�.
		if (transactionQueue.empty())
			break;
			
		tr = transactionQueue.front();
		if (! tr->isDone())
			break;

		PAS_DEBUG2("Transaction: delayed TR [%d] now finishing, %s", tr->id(), getMyInfo() );
		PHTR_DEBUG2("Transaction: delayed TR [%d] now finishing, %s", tr->id(), getMyInfo() );
	}
	
	return 0;
}


void ClientHandler::startSSL(Transaction *tr)
{
	// SSL �� report �� �߰�.  2006.12.28
	// send mwatch (mIDC system monitoring)
	WatchReporter::IncreaseMsgWatchSvr();

	// pasmonitor (logsvr, KUNLOG) �� 1 �� ���� ȣ��. �����δ� 20�� ���� ����.
	MonitorReporter::increasePasMon();
	
	tr->setCpConnInfo_first();
	
	host_t sslHost(tr->cpHostName);
	int sslPort(tr->cpPort);


	// session, tr  ���� ������ �� ���Ѿ� �Ѵ�. ���� �ʿ�.
	session.setThreadId();
	session.beginSSL();
	
	tr->beginSSL();
	tr->setTransactionInfo();
	tr->recvPhoneReq();
	tr->sendCpReq();
	
	session.setSSLHost(sslHost);
	session.setSSLPort(sslPort);

	// user �������� session ���� ���� �´�.
	getUserInfo_SSL();

	// session �������� tr ���� ���� �´�.
	session.copyTransaction(tr);

	if (tr->phoneNumber[0] == '\0')  // ó������ SSL �� �ö����, ����� ������ UserInfo �� ���� ���. 
	{
		tr->setAnonymous();
	}
	
	PAS_DEBUG3("[%s] SSL CONNECT : %s %d", session.getNumber(), sslHost.toStr(), sslPort);
	PHTR_DEBUG3("[%s] SSL CONNECT : %s %d", session.getNumber(), sslHost.toStr(), sslPort);
	logHttpReq("[SSL CONNECT PHONE REQ]", tr, true);
	
	session.beginTransaction(tr);
	
	
	paslog->startOfTransaction(userInfo, &session, tr);

	tr->setCpConnInfo_second();

	 // Ȥ�� ���� ������ ��쿡�� �ð����� ����� ��µǰ� �ϱ� ���� ���⼭ ����. 
	 // CpHandler ���� ���� ����Ǵ� ��쿡 �ٽ� ������Ʈ �ϹǷ� ���⼭ �ߺ��ؼ� �ҷ��൵ �ȴ�.
	tr->connectCp(); 
	
	host_t sslAddr(tr->cpIpAddr); // IP address �� �����ϸ� �� �ȵǴ� ������ , �������� ���. ( ent.wooribank.com:443)

	requestSSLConnection(tr, sslHost, sslPort);
}

/*----------
[SSL CONNECT PHONE REQ] : Leng=71, Header=71, Body=0
12:06:52 CONNECT ent.wooribank.com:443 HTTP/1.1^M
Host: ent.wooribank.com:443^M
^M

[SSL RESULT PHONE REQ] : Leng=243, Header=243, Body=0
12:06:55 RESULT^M
CPData: cpname=woori;svccode=wooribanking000^M
User-Agent: MobileExplorer/1.2 (Mozilla/1.22; compatible; KUNF12; ^M
HTTP_PHONE_NUMBER: 8201073989200^M
HTTP_PHONE_SYSTEM_PARAMETER: BASE_ID:326, NID:36, SID:2189, BASE_LAT:0, BASE_LONG:0^M
^M

--------*/

/**
*/
void ClientHandler::finishSSL(Transaction *tr,  bool resultRecved /* = false */)
{
	// SSL ����
	//storeSSLTransaction(sslResult);
	if (resultRecved)
	{
		PAS_DEBUG1("ClientHandler::finishSSL() - SSL RESULT : %s", setMyInfo());
		PHTR_DEBUG1("ClientHandler::finishSSL() - SSL RESULT : %s", getMyInfo());
		
		tr->beginSSL();
		tr->recvPhoneReq();
		tr->setTransactionInfo();
		session.setFirstInfo(tr);
		session.setCPname(tr);
		session.copyTransaction(tr);
	}
	else
	{
		PAS_DEBUG1("ClientHandler::finishSSL() - SSL FINISH : %s", setMyInfo());
		PHTR_DEBUG1("ClientHandler::finishSSL() - SSL FINISH : %s", getMyInfo());
		
		session.copyTransaction(tr);
	}

	if (userInfo) //2006-12-13
		userInfo->storeSession(&session);
		
	if (tr->phoneNumber[0] == '\0')  // ó������ SSL �� �ö����, ����� ������ UserInfo �� ���� ���. 
	{
		tr->setAnonymous();
	}
	
	
	logHttpReq("[SSL RESULT PHONE REQ]", tr, true);

	tr->sendPhoneResp();
	tr->setCpTime(); // CP ���� ���� ���� ���� ��쿡�� �ð��� �����ϱ� ����.
	tr->setDone();
	tr->endSSL();
	snprintf(tr->realUrl, MAX_URL_LEN, "%s:%d", session.sslHost.toStr(), session.sslPort);
	session.endTransaction(tr);
	session.endSSL();


	if (userInfo)
		userInfo->updateRespTime();

	paslog->openFiles();
	paslog->writeSslPasResponse(userInfo, &session, tr, resultRecved);
	paslog->writeSslPasStat(userInfo, &session, tr);
	paslog->writeSslPasIdr(userInfo, &session, tr);

	if (userInfo)
		userInfo->updateReqTime();
		
	// �α� �ۼ� �Ŀ� 	
	session.setLastTransactionTime();
	session.clearSSL();
	
	
}

void ClientHandler::storeSSLTransaction(const HTTP::KTFSSLResult& /* sslResult */)
{
	// @todo : Http ��� ���� alloc �� Ʈ����� ������ ó���� ����
}



void ClientHandler::requestSSLConnection(Transaction * /* tr */, const host_t& host, const int port)
{
	CpHandler* cp = pickCpHandler(host, port);
	
	// SSL ���� ��û
	if(!cp->isConnected())
	{
		int conn_res = cp->connectSSL(host, port);
		if (conn_res < 0)
		{
			PAS_DEBUG3("SSL Client: CONNECT Failed instantly : %s %d [%s]", host.toStr(), port, session.getNumber());
			PHTR_DEBUG3("SSL Client: CONNECT Failed instantly : %s %d [%s]", host.toStr(), port, session.getNumber());
			responseSSLConnectionFail();
		}
		else
		{
			 /// SSL ������ ���� ���̸� ���� ��� 
			changeState (CS_SSL_CONNECTING );
			PAS_DEBUG3("SSL Client: CONNECTING : %s %d ; %s", host.toStr(), port, session.getNumber());
			PHTR_DEBUG3("SSL Client: CONNECTING : %s %d ; %s", host.toStr(), port,  session.getNumber());
		}
	}

	// �̹� CP�� ����Ǿ� ����
	else
	{
		PAS_DEBUG3("SSL Client: Already CONNECTED : %s %d ; %s", host.toStr(), port, session.getNumber());
		PHTR_DEBUG3("SSL Client: Already CONNECTED : %s %d ; %s", host.toStr(), port,  session.getNumber());
		// SSL��ſ� CP ������ �Ϸ� ������ Client���� �˸�
		responseSSLConnectionSuccess();
	}
}

void ClientHandler::responseSSLConnectionSuccess()
{
	
	Transaction *tr = NULL;
	
	if (! transactionQueue.empty())
	{
		tr = transactionQueue.front();
	}
	else
	{
	// @todo
		tr = currTransaction;
	}

	HTTP::Response *pResponse = tr->getResponse();
		
	ResponseBuilder::CpSSLConnSuccessed(pResponse);
	
	PAS_DEBUG1("SSL Client: sending CONNECT SUCC to phone; %s",  setMyInfo());
	PHTR_DEBUG1("SSL Client: sending CONNECT SUCC to phone; %s",  getMyInfo());

	tr->recvCpResp(); 
	tr->sendPhoneResp(); 
	tr->setCpTime(); 
	
	enSendQueue(pResponse->getRawHeader()->rd_ptr(), pResponse->getRawHeader()->length());
	if (pResponse->getRawBody() != NULL)
		enSendQueue(pResponse->getRawBody()->rd_ptr(), pResponse->getRawBody()->length());
		
	paslog->endOfTransaction(userInfo, &session, tr);
	session.setLastTransactionTime();
	tr->clearSizeAndTime();
	
}

void ClientHandler::responseSSLConnectionFail()
{
	Transaction *tr = NULL;
	
	if (! transactionQueue.empty())
		tr = transactionQueue.front();
	else
		tr = currTransaction;
	

	HTTP::Response *pResponse = tr->getResponse();
		
	ResponseBuilder::CpSSLConnFailed(pResponse);
	

	PAS_DEBUG1("SSL Client: sending CONNECT FAIL to phone; %s",  setMyInfo());
	PHTR_DEBUG1("SSL Client: sending CONNECT FAIL to phone; %s",  getMyInfo());

	tr->recvCpResp(); 
	tr->sendPhoneResp(); 
	tr->setCpTime(); 
	
	enSendQueue(pResponse->getRawHeader()->rd_ptr(), pResponse->getRawHeader()->length());
	if (pResponse->getRawBody() != NULL)
		enSendQueue(pResponse->getRawBody()->rd_ptr(), pResponse->getRawBody()->length());
	
	paslog->endOfTransaction(userInfo, &session, tr);
	session.setLastTransactionTime();
	tr->clearSizeAndTime();
}





char* ClientHandler::setMyInfo()
{
	snprintf(myinfo, MYID_LEN, "PhoneNumber[%s] Phone[%s:%d] Sock[%d] Seq[%d]",
		session.phoneNumber, session.ipAddr, session.port, session.sockfd, session.getSeqNum());
	return myinfo;
}

#ifdef HTTP_LOG
void ClientHandler::logHttpReq(const char *mesg, Transaction *tr, bool printBody)
#else
void ClientHandler::logHttpReq(const char* /* mesg */, Transaction *tr, bool /* printBody */)
#endif
{
	if (tr==NULL) return;

	HTTP::Request* pRequest = tr->getRequest();
	const ACE_Message_Block*header = pRequest->getRawHeader();

	if (header==NULL)
	{
		PAS_NOTICE1("ODD happend - HTTP::Request==NULL - %s", getMyInfo());
		return;
	}
	
	char	headerBuff[1024*2];
	int	len = MIN(sizeof(headerBuff)-1, header->length());
	memcpy(headerBuff, header->rd_ptr(), len);
	headerBuff[len] = 0;

	#ifdef HTTP_LOG
	httplog->logprint(LVL_INFO, "=== %s : Leng=%d, Header=%d, Body=%d\n", 
			mesg,  pRequest->getHeadLeng() + pRequest->getBodyLeng(), pRequest->getHeadLeng(), pRequest->getBodyLeng());
	httplog->logprint(LVL_INFO, "%s", headerBuff);
	httplog->logprint(LVL_INFO, "---\n" );
	
	const ACE_Message_Block* body = pRequest->getRawBody();
	if (body && printBody && body->length())
	{
		len = MIN(body->length(), 256);
		httplog->hexdump(LVL_INFO,  body->rd_ptr(), len,  "Body ");
	}
	#endif
}

/**
*/
#ifdef HTTP_LOG
void ClientHandler::logHttpResp(const char *mesg, Transaction *tr, bool printBody)
#else
void ClientHandler::logHttpResp(const char * /* mesg */, Transaction *tr, bool /* printBody */)
#endif
{
	if (tr==NULL) return;
	HTTP::Response* resp = tr->getResponse();
	const ACE_Message_Block*header = resp->getRawHeader();
	
	if (header==NULL)
	{
		PAS_NOTICE1("ODD happend - HTTP::Response==NULL - %s", getMyInfo());
		return;
	}
	
	char	headerBuff[1024*2];
	int	len = MIN(sizeof(headerBuff)-1, header->length());
	memcpy(headerBuff, header->rd_ptr(), len);
	headerBuff[len] = 0;

	#ifdef HTTP_LOG
	httplog->logprint(LVL_INFO, "=== %s : Leng=%d, Header=%d, Body=%d\n", 
			mesg,  resp->getHeadLeng() + resp->getBodyLeng(), resp->getHeadLeng(), resp->getBodyLeng());
			
	httplog->logprint(LVL_INFO, "%s", headerBuff);
	httplog->logprint(LVL_INFO, "---\n" );

	const ACE_Message_Block*body = resp->getRawBody();
	if (body && printBody && resp->getBodyLeng())
	{
		len = MIN(body->length(), 32);
		httplog->hexdump(LVL_INFO,  body->rd_ptr(), len,  "Body ");
	}
	#endif
}


/**
���� ó�� �߿� �ִ� transaction �� �ܸ� request header �� ���� �Ѵ�.
�ܸ��κ��� �̻��� ��Ŷ�� ������ ���, �� ������ � ��Ŷ�� �����Ͽ����� ���� �����̴�.
2006.12.16
*/
void ClientHandler::printRecentTransaction()
{
	if (transactionQueue.empty())
		return;
	
	Transaction *recentTr = transactionQueue.back();
	HTTP:: Request *pRequest = recentTr->getRequest();
	PAS_INFO1("ClientHandler:: print the recent HTTP header from phone - %s", getMyInfo());
	PAS_INFO_DUMP("Recent PHONE HEAD", pRequest->getRawHeader()->rd_ptr(), pRequest->getHeadLeng());	
}


void ClientHandler::updateSessionInfo()
{
	bool emptyPhoneNumber = (session.phoneNumber[0] == '\0');
	if(emptyPhoneNumber)
	{
		PAS_DEBUG("Session update skip. Because phoneNumber is empty.");
		return;
	}

	if(strlen(session.phoneNumber) < 10)
	{
		PAS_INFO1("Session update skip. Because phoneNumber[%s] is small.", session.phoneNumber);
		return;
	}

	if(memcmp(session.phoneNumber, "010", 3) != 0)
	{
		PAS_INFO1("Session update skip. Because phoneNumber[%s] is not started with \"010\".", session.phoneNumber);
		return;
	}

	SisorProxy* pSisor = NULL;

	try
	{
		SessionWriteRequest req;
		req.mdn = session.phoneNumber;
		req.connCount = 1;
		req.lastAddr = session.ipAddr;
		req.lastAuthTime = session.pasauthTime;
		req.lastClose = time(NULL);
		req.lastConn = session.startSec;
		req.lastSantaTime = session.santaTime;
		req.tranCount = session.respNums;

		pSisor = SisorQueue::instance()->get();
		pSisor->query(req);
		SisorQueue::instance()->put(pSisor);

		PAS_DEBUG3("Session updated. MDN[%s] ConnCount[%d] TranCount[%d]",
			req.mdn.toString().toStr(), req.connCount, req.tranCount);
	}
	catch (Exception e)
	{
		if(pSisor != NULL)
			delete pSisor;

		PAS_NOTICE1("Session update fail. Because of %s", e.toString());
	}
}

url_t ClientHandler::addQueryToURL(const url_t& srcURL, const char* key, const char* value)
{
	url_t result;

	// exist query
	if(srcURL.find('?') >= 0)
	{
		result.sprintf("%s&%s=%s", srcURL.toStr(), key, value);
	}
	else
	{
		// end of host or directory
		if(srcURL[srcURL.size()-1] == '/')
		{
			result.sprintf("%s?%s=%s", srcURL.toStr(), key, value);
		}

		else
		{
			int schemePos = srcURL.incaseFind("http://");
			int schemeNextPos = (schemePos >= 0) ? (schemePos + 7) : 0;
			
			// end of file
			if(srcURL.find('/', schemeNextPos) >= 0)
			{
				result.sprintf("%s?%s=%s", srcURL.toStr(), key, value);
			}

			// end of host
			else
			{
				result.sprintf("%s/?%s=%s", srcURL.toStr(), key, value);
			}
		}
	}

	return result;
}

url_t ClientHandler::getGuideURL( const GuideCode gcode, const url_t& reqURL )
{
	url_t redirectURL;

	const GuideConfig& guideConf = Config::instance()->guide;
	switch(gcode)
	{
	case GCODE_MagicN:
		redirectURL = guideConf.guideUrlMagicn;
		break;

	case GCODE_Fimm:
		redirectURL = guideConf.guideUrlFimm;
		break;

	case GCODE_MultiPack:
		redirectURL = guideConf.guideUrlMultipack;
		break;

	default:
		redirectURL = guideConf.guideUrlMagicn;
		break;
	}

	if(redirectURL.isEmpty())
		return url_t();

	return addQueryToURL(redirectURL, "reqURL", reqURL);
}

url_t ClientHandler::getGuideResultURL( const GuideCode gcode, const url_t& reqURL )
{
	url_t resultURL;
	const GuideConfig& guideConf = Config::instance()->guide;

	switch(gcode)
	{
	case GCODE_MagicN:
		resultURL= guideConf.guideResultUrlMagicn;
		break;

	case GCODE_Fimm:
		resultURL= guideConf.guideResultUrlFimm;
		break;

	case GCODE_MultiPack:
		resultURL = guideConf.guideResultUrlMultipack;
		break;

	default:
		PAS_INFO1("Unknown gcode[%d]", gcode);
		resultURL = guideConf.guideResultUrlMagicn;
		break;
	}

	return addQueryToURL(resultURL, "reqURL", reqURL);
}

int ClientHandler::redirectToReqURL( Transaction* tr )
{
	url_t reqURL = tr->getRequest()->getHeader()->getUrl();
	url_t redirectURL = getReqURL(reqURL);
	if(redirectURL.isEmpty())
		return 0; // no redirected

	// Pas �� ���� redirection �ƴٴ� ���� �˸��� ���� URL �ڿ� �߰� ������ ���δ�.
	// add slash to trail

	PAS_INFO1("Redirect to reqURL[%s]", redirectURL.toStr());
	PHTR_INFO1("Redirect to reqURL[%s]", redirectURL.toStr());

	// redirect
	ResponseBuilder::Redirect(tr->getResponse(), redirectURL);

	afterCpTransaction(tr);	

	return 1; // redirected
}

bool ClientHandler::procService( Transaction* tr )
{
	PAS_TRACE("Check for service.");

	url_t reqURL = tr->getRequest()->getHeader()->getUrl();
	int pos = reqURL.find("service.");

	if(pos < 0)
		return false;

	url_t serviceStr = reqURL.substr(pos);
	urls_t parts = explode(serviceStr, '.');

	if(parts.size() != 3)
		return false;

	if(parts[0] != "service" || parts[2] != "html")
		return false;

	int productCode = parts[1].toInt();

	ServiceReadRequest req;
	req.mdn = tr->MDN;
	req.productCode = productCode;
	
	SisorProxy* pSisorProxy = NULL;

	bool blockProduct;

	try
	{
		pSisorProxy = SisorQueue::instance()->get();
		ServiceReadResponse res = pSisorProxy->query(req);				
		SisorQueue::instance()->put(pSisorProxy);

		blockProduct = res.blockProduct;
		if(blockProduct)
		{
			PAS_DEBUG("Service was blocked.");
		}
	}
	catch (Exception e)
	{
		PAS_NOTICE1("Query to sisor fail. Because of %s. Force to block.", e.toString());

		blockProduct = true;
		delete pSisorProxy;
	}

	if(blockProduct)
	{
		// redirect
		ResponseBuilder::Redirect(tr->getResponse(), "http:://172.23.35.87/~ssehoony/service/blocked.html");

		tr->setErrorRespCode(RESCODE_STATFILTER);
		afterCpTransaction(tr);	
		return true;
	}

	return false;
}

bool ClientHandler::addCorrelationKey( HTTP::ResponseHeader& destHeader, const CorrelationKey& key )
{
	PAS_TRACE("Add correlation key to response header");
	PAS_TRACE1("Correlation key is %s", key.toString().toStr());

	HTTP::value_t bInfo = destHeader.getElement("BILL_INFO");
	bInfo += ";";
	bInfo += key.toString();
	destHeader.replaceElement("BILL_INFO", bInfo);

	return true;
}

bool ClientHandler::addCorrelationKey( HTTP::RequestHeader& destHeader, const CorrelationKey& key )
{
	PAS_TRACE("Add correlation key to request header");
	PAS_TRACE1("Correlation key is %s", key.toString().c_str());

	destHeader.addElement("CORRELATION_KEY", key.toString());

	return true;
}

void ClientHandler::onReceivedHeaderFromCP(CpHandler* pCP, CpResponseData* pData)
{
	//ACE_ASSERT(pCP == &_cp);
	ACE_ASSERT(pData != NULL);

	const Transaction* tr = pData->getTransaction();
	ACE_ASSERT(tr != NULL);

	if(!tr->streaming())
		return;

	_sentBodySizeByStream = 0;

	HTTP::ResponseHeader header;

	header.parse(pData->getData(), pData->getDataSize());

	// ����� �߰����� ���� ���
	HTTP::ResponseHeader modifyHeader = additionalInfoToResHeader(header, tr->phoneNumber, tr->orgUrl, 
		tr->cpReqBytes, header.getContentLength(), tr->getRequest()->getHeader()->getHost(), tr->correlationKey);
	
	HTTP::header_t rawHeader;
	modifyHeader.build(&rawHeader);
	
	enSendQueue(rawHeader.toStr(), rawHeader.size());

	if (userInfo)
		userInfo->updateRespTime();	
}

void ClientHandler::onReceivedPartialBodyFromCP(CpHandler* pCP, CpResponseData* pData)
{
	//ACE_ASSERT(pCP == &_cp);
	ACE_ASSERT(pData != NULL);

	const Transaction* tr = pData->getTransaction();
	ACE_ASSERT(tr != NULL);

	if(!tr->streaming())
		return;

	_sentBodySizeByStream += pData->getDataSize();

	int res = enSendQueue(pData->getData(), pData->getDataSize());
	
	// ���� ��� �۽����� ���ߴٸ�
	// handset���� �۽��� ���� �� ������ cp�� ������ ��� ��Ų��.
	if(res == 1)
		pCP->removeEvent(ACE_Event_Handler::READ_MASK);

	if (userInfo)
		userInfo->updateRespTime();
}

void ClientHandler::onSendQueueEmpty()
{
	// streaming�� ���� client�� send queue�� �����Ͱ� ���� ���
	// CP�� ���� �̺�Ʈ MASK�� �����Ѱ� �ȴ�.
	// �׷��Ƿ� client�� send queue�� empty�� �Ǹ�, CP�� �����͸� ��� ���� �� �� �ֵ���
	// READ_MASK�� ����ؾ� �Ѵ�.
	//_cp.addEvent(ACE_Event_Handler::READ_MASK);
	if(_pCP != NULL)
		_pCP->addEvent(ACE_Event_Handler::READ_MASK);
}

HTTP::ResponseHeader ClientHandler::additionalInfoToResHeader( const HTTP::ResponseHeader& header, const char* phoneNumber, 
	const url_t& orignalRequestURL, const int requestDataSize, const int responseBodySize, const host_t& requestHost, 
	const CorrelationKey& correlationKey )
{
	HTTP::ResponseHeader modifyHeader = header;

	// billinfo �� ON �̸鼭 ��ȭ��ȣ�� billinfoTarget �� �����ϴ� ��쿡 ����.
	Config *conf = Config::instance();
	if (conf->process.billinfo 
		&&	(conf->process.billinfoTarget.c_str()[0] == '*'  || Util::mdnStartswith(phoneNumber, conf->process.billinfoTarget.c_str() )))
	{
		if(addBillInfo(&modifyHeader) < 0)
		{
			PAS_ERROR1("�Ϲ� ������ BillInfo �߰� ����, fd[%d]", get_handle());
			PHTR_DEBUG("�Ϲ� ������ BillInfo �߰� ����");
		}
	}

	// �ȳ� ������ BILL_INFO ����
	if (!strcasecmp(conf->network.kunHost, requestHost))
	{
		if (addGuidePageBillInfo(&modifyHeader) < 0)
		{
			PAS_ERROR1("�ȳ� ������ BillInfo �߰� ����, fd[%d]", get_handle());
			PHTR_DEBUG("�ȳ� ������ BillInfo �߰� ����");
		}
	}

	// correlation key
	//addCorrelationKey(modifyHeader, correlationKey);

	// hash key
	// @ ���� :  hash key �� ���Ҷ��� ���� URL ���� �������� ���Ͽ��� �Ѵ�. 
	// ACL, Hotnumber ���� ��� URL�� ����ϴ� ���� �ƴϴ�.
	if(addHashKey(&modifyHeader, orignalRequestURL, requestDataSize, responseBodySize) < 0)
	{
		PAS_ERROR1("HashKey �߰� ����, fd[%d]", get_handle());
		PHTR_DEBUG("HashKey �߰� ����");
	}

	return modifyHeader;
}

void ClientHandler::writeGuideCommunicationLog( const ACE_Time_Value& duration, const GuideCode& gcode ) const
{
	MyLog log;
	filename_t filename = Config::instance()->getLogPrefix();
	filename += "guide";
	log.openWithYear("./", filename);

	// MDN Duration GCode MSModel Browser URL
	log.logprint(LVL_INFO, "%s %d.%06d %d %s %s %s\n", 
		session.phoneNumber, duration.sec(), duration.usec(), 
		static_cast<int>(gcode), session.msModel, session.browser, 
		session.lastRequestURL.c_str());
	log.close();
}

void ClientHandler::writeAbnormalCloseLog()
{
	if(transactionQueue.size() > 0 || sendQueue.size() > 0)
	{
		if(transactionQueue.size() > 0)
			PAS_INFO("Abnormal close. Transaction queue is not empty.");
		
		if(sendQueue.size() > 0)
			PAS_INFO("Abnormal close. Send queue is not empty.");

		MyLog log;
		filename_t filename = Config::instance()->getLogPrefix();
		filename += "abnormalclose";
		log.openWithYear("./", filename);

		// MDN msModel Browser URL
		log.logprint(LVL_INFO, "%-11s %03d/%03d %-11s %-11s %s\n", 
			session.phoneNumber, session.getRespNum(), session.getSeqNum(),
			session.msModel, session.browser, session.lastRequestURL.c_str());

		log.close();
	}
}


int ClientHandler::additionalInfoToReqHeader( Transaction * tr )
{
	HTTP::Request* pRequest = tr->getRequest();
	HTTP::RequestHeader modifyHeader = (*pRequest->getHeader());

	// Proxy Info �߰�
	addProxyInfo( &modifyHeader );

	// Correlation key �߰�
	//addCorrelationKey(modifyHeader, tr->correlationKey);

	// ���ο� ��� ����
	HTTP::header_t newRawHeader;
	if(modifyHeader.build(&newRawHeader) < 0)
	{
		PAS_ERROR1("additionalInfoToReqHeader >> HTTP header building fail. fd[%d]", get_handle());
		PHTR_ERROR("additionalInfoToReqHeader >> HTTP header building fail.");
		return -1;
	}

	// ���ο� ��� ����
	if(pRequest->setHeader(newRawHeader) < 0)
	{
		PAS_ERROR1("additionalInfoToReqHeader >> HTTP header setting fail. fd[%d]", get_handle());
		PHTR_ERROR("additionalInfoToReqHeader >> HTTP header setting fail.");

		PAS_ERROR1("newRawHeader =>\n%s", newRawHeader.c_str());
		PHTR_ERROR1("newRawHeader =>\n%s", newRawHeader.c_str());

		PAS_ERROR_DUMP("newRawHeader tail", newRawHeader.c_str() + newRawHeader.size() - std::min(newRawHeader.size(), 128U), std::min(newRawHeader.size(), 128U));
		return -1;
	}

	return 0;
}

