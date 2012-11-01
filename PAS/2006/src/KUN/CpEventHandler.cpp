#include "CpEventHandler.h"
#include <ace/Reactor.h>
#include "Common.h"
#include "Mutex.h"
#include "HttpResponseHeader.h"
#include "HttpRequest.h"
#include "Config.h"
#include "HttpCodes.h"
#include "ResponseBuilder.h"
#include <stdio.h>
#include <strings.h>

unsigned int CpHandler::sentDirectCnt = 0;
unsigned int CpHandler::sentByQueCnt = 0;

CpHandler::CpHandler(ACE_Reactor* pReactor, PasHandler* pRequester_)
: PasHandler(pReactor, HT_CpHandler)
{
	myinfo[0] = '\0';

	closeFuncExecuted = false;	
	this->pRequester = pRequester_;
	isSSL = false;

	#ifdef HTTP_DUMP
	filedump = new FileDump();
	#endif

	state = CPS_NONE;
	lastCpRecvSec = 0;
	_port = 0;
}

// construct ���Ŀ� �����ϱ� ����.
void CpHandler::init()
{
	ACE_ASSERT(reactor()==pRequester->reactor());

	// PasEventHandler ���� ó���ϵ��� ����. 2007.1.12
	int BufferSize = Config::instance()->process.RecvBufferBytes;
	if (BufferSize > 4 * 1024)
		recvBuffer.size(BufferSize); 

	setMyInfo();
		
	PAS_TRACE2("CP Init:BufferSize[%d] %s", BufferSize, getMyInfo());
	PHTR_DEBUG2("CP Init:BufferSize[%d] %s", BufferSize, getMyInfo());
}

CpHandler::~CpHandler(void)
{
	if(isConnected())
		pSysStat->cpCloseByHost(1);

	close();

	#ifdef HTTP_DUMP
	if( filedump )
		delete filedump;
	#endif
}


void CpHandler::close()
{
	if (lastCpRecvSec != 0)
	{
		PAS_DEBUG1("CP Complete odd body. %s", getMyInfo());
		PHTR_INFO1("CP Complete odd body. %s", getMyInfo());
		changeState ( CPS_RECEIVED_RESPONSE );
		onCompletedReceiveResponse();
	}

	deleteSendQueue();
	
	if (closeFuncExecuted)
	{
		PAS_TRACE1("Already closed. %s", getMyInfo());	
		return;
	}

	closeFuncExecuted = true;
	
	PAS_TRACE1("CP closed. %s", getMyInfo());
	PHTR_DEBUG1("CP closed. %s", getMyInfo());

	if(!requestedQueue.empty())
	{
		PAS_INFO2("CP close(). STILL %d requests left - %s", requestedQueue.size(), getMyInfo());
		PHTR_INFO2("CP close(). STILL %d requests left - %s", requestedQueue.size(), getMyInfo());
		
		if (!pRequester->isConnected())
		{
		// �̹� �ܸ��� close �� ���.
			PAS_INFO2("CP finish [%s]: phone is closed already. [%s]", getMyInfo(), pRequester->getMyInfo());
			PHTR_INFO2("CP finish [%s]: phone is closed already. [%s]", getMyInfo(), pRequester->getMyInfo());
		}
		else
		{
			PAS_INFO3("CP finish [%s] Finishing %d jobs. [%s]", getMyInfo(), requestedQueue.size(), pRequester->getMyInfo());
			PHTR_INFO3("CP finish [%s] Finishing %d jobs. [%s]", getMyInfo(), requestedQueue.size(), pRequester->getMyInfo());
			finishAll(RESCODE_CP_TIMEOUT);
		}
	}

	PasHandler::close();
	
	setJobDone();
}

void CpHandler::changeState(CPState _state)
{
	PAS_TRACE2("CpHandler::changeState: %d --> %d", state,  _state);
	PHTR_DEBUG2("CpHandler::changeState: %d --> %d", state,  _state);
	state = _state;
}

/**
PAS - CP ���� ������ ��  transaction�� ���ø� �ǹ��Ѵ�.
���ð� ���������� ���� ��� return -1. 
transaction �� �� ���õ� ��쿡�� finish() �� transaction�ϷḦ ó���Ͽ��� �Ѵ�.

@return -1: connect �õ��� ������ ���. (���� CP�� �����ϱ⵵ ���� ������ ����̴�. ���� socket ���� ���?
 CP�� ���� ���д� ���Ŀ� event�� �� �� �ִ�.
*/
int CpHandler::start(Transaction *pTransaction)
{
	setMyInfo();

	PAS_TRACE1("CpHandler:: start() %s", getMyInfo());
	PHTR_DEBUG1("CpHandler:: start() %s", getMyInfo());

	startTimeTick(Config::instance()->cp.timeoutCheckInterval);  // Timer ���� 
	
	// CP WEB �� ����
	if( !isConnected() )
	{
		if (isConnecting())
		{
			PAS_INFO2("Connect requested while connecting %s:%d", _host.toStr(), _port);
			PHTR_INFO2("CpHandler:: start() - connect requested while connecting %s:%d", _host.toStr(), _port);
		}
		else if( connect(_host, _port) == -1 )
		{
			// 2007.1.4 don't close()
			//close();
			return -1;
		}

		changeState (CPS_CONNECTING);
		
		pSysStat->cpConnectionRequest(1);

		startConnectTimer(); // ���� �õ� ���̹Ƿ� Timer ����

		// onConnect() ���� ó��. sendToCp() ȣ��.
		requestedQueue.push(pTransaction);
		setMyInfo();
	}

	// CP WEB �� �̹� ���ӵ� ����
	else
	{
		pTransaction->connectCp();
		int sendres = sendToCp(pTransaction);
		PAS_DEBUG2("Already connected. sendres[%d] %s", sendres, getMyInfo());
		PHTR_DEBUG2("CpHandler:: already connected. sendres[%d] %s", sendres, getMyInfo());
		if (sendres >= 0)
			requestedQueue.push(pTransaction);
	}

	return 0;
}


void CpHandler::onConnect()
{
	PAS_TRACE1("Connected. %s", getMyInfo());
	PHTR_DEBUG1("CpHandler:: connected %s", getMyInfo());

	PasHandler::onConnect();
	
	changeState (CPS_WAIT_RESPONSE_HEADER);

	closeFuncExecuted = false;

	int connFlag = 1;	
	pRequester->onCommand(CID_CP_Connected, this, &connFlag); // ���� �Ϸ� ������ ClientHandler �˸�, 1�� ������ �ǹ�
	pSysStat->cpConnectionEstablished(1); // increase statistic

	if (!requestedQueue.empty())
	{
		Transaction *pTr = requestedQueue.front();
		pTr->connectCp();
		int sendres = sendToCp(pTr);
		PAS_TRACE2("Sent on connect. SendRes[%d] %s", sendres, getMyInfo());
		PHTR_DEBUG2("CpHandler:: sent on connect. SendRes[%d] %s", sendres, getMyInfo());
	}
	
}

/**
sendToCp() �߰�: Connect�� send�� �и��ϴ� ��. �������� start() �� ���ԵǾ���.
*/
int CpHandler::sendToCp(Transaction *pTransaction)
{
	startReceiveTimer();

	HTTP::Request* pNewRequest = pTransaction->getRequest();
		
	ACE_ASSERT(pNewRequest != NULL);
	ACE_ASSERT(pNewRequest->getRawHeader() != NULL);

	// increase statistic
	pSysStat->cpRequest(1);

	// ��� �۽�
	const ACE_Message_Block* pSendData = pNewRequest->getRawHeader();
	int resultEnqueue = enSendQueue(pSendData->rd_ptr(), pSendData->length());
		
	#ifdef HTTP_DUMP
	filedump->init("CP-REQU-ALL", 2);
	filedump->write(pSendData->rd_ptr(), pSendData->length());
	#endif
	
	if(resultEnqueue < 0)
	{
		PAS_INFO1("CP enSendQueue failed. %s", getMyInfo());
		PHTR_INFO1("CP enSendQueue failed. %s", getMyInfo());

		return -1;
	}

	// body �۽�
	const size_t bodyLen = pNewRequest->getBodyLeng();
	if(bodyLen > 0)
	{
		ACE_ASSERT(pNewRequest->getRawBody() != NULL);
		// increase statistic
		pSysStat->cpUpload(1);

		pSendData = pNewRequest->getRawBody();
		resultEnqueue = enSendQueue(pSendData->rd_ptr(), pSendData->length());

		if (resultEnqueue==0)
			sentDirectCnt++;
		else if (resultEnqueue==1)
			sentByQueCnt++;
			
		if(resultEnqueue < 0)
		{
			PAS_NOTICE1("CP enSendQueue failed [%s]", getMyInfo());
			PHTR_NOTICE1("CP enSendQueue failed [%s]", getMyInfo());

			return -1;
		}
	}

	// request�� CP �� ���� ����� ���� ó��
	pTransaction->sendCpReq();
	_receivedBodySize = 0;

	return 0;
}

int	CpHandler::finishAll(int errorCode)
{
	int	count = 0;
	while (! requestedQueue.empty())
	{
		count++;
		PAS_INFO2("CpHandler::finishAll() - Count=%d, Code=%d", count, errorCode);
		PHTR_INFO2("CpHandler::finishAll() - Count=%d, Code=%d", count, errorCode);
		finish(errorCode);	
	}
	return 0;
}

/*
�ϳ��� transaction �� �Ϸ�Ǿ��ٰ� �Ǵܵ� �� ȣ��.
transaction �� �� ���õ� ��쿡�� finish() �� transaction�ϷḦ ó���Ͽ��� �Ѵ�.
@param errorCode  default���� RESCODE_OK. RESCODE_OK�� ���������� �ǹ�. timeout �ó�  �����ÿ� errorCode�� �����Ͽ� ���ο��� ȣ��.
*/
int	CpHandler::finish(int errorCode)
{
	Transaction* finishTransaction;

	if ( !pRequester->isConnected() )
	{
		// �̹� �ܸ��� close �� ���.
		PAS_INFO1("Phone is closed already. %s", getMyInfo());
		PHTR_INFO1("Phone is closed already. %s", getMyInfo());
		return -1;
	}
	
	if( requestedQueue.empty() || requestedQueue.pop_front(finishTransaction) < 0 )
	{
		PAS_NOTICE1("%s CP finish: ��û���� ���� ����� ����.", getMyInfo());
		PHTR_NOTICE1("%s CP finish: ��û���� ���� ����� ����.", getMyInfo());
		return  -1;
	}

	// increase statistic
	pSysStat->cpResponse(1);

	PAS_TRACE2("errorCode[%d] %s", errorCode, getMyInfo());
	PHTR_DEBUG2("errorCode[%d] %s", errorCode, getMyInfo());

	// response �� CP �κ��� ����  ����� ���� ó��
	
	if (errorCode == RESCODE_OK)
	{
		if (currHttpResponse != finishTransaction->getResponse())
		{
			// ���� ��Ȳ. ���������̴׿��� ���� ���� �� �� �ִ�.   recv���� mutex �ϱ� ������ �߻� �Ұ��� ��Ȳ.
			currHttpResponse = NULL;
			PAS_NOTICE1("%s CP finish: currHttpResponse is wrong.", getMyInfo());
			PHTR_NOTICE1("%s CP finish: currHttpResponse is wrong.", getMyInfo());
			return -1;
		}
		else
		{
			//������ Request/Response ���� �����ϴ� ���� Transaction �ϳ��� ������.
			PAS_TRACE1("CP finish: notify to Client. %s", getMyInfo());
			PHTR_DEBUG1("CP finish: notify to Client. %s", getMyInfo());
			pRequester->onCommand(CID_CP_Completed, this, (void*)finishTransaction);
			currHttpResponse = NULL;
		}

	}
	else
	{
		if (errorCode==RESCODE_CP_CONN)
			ResponseBuilder::CpConnFailed( finishTransaction->getResponse() );
		else if (errorCode==RESCODE_CP_TIMEOUT)
			ResponseBuilder::CpTimeout( finishTransaction->getResponse() );

		_receivedBodySize = finishTransaction->getResponse()->getBodyLeng();
			
		finishTransaction->setErrorRespCode( errorCode );

		// ClientHandler ���� ��� �뺸.
		pRequester->onCommand(CID_CP_Completed, this, (void*)finishTransaction);
		currHttpResponse = NULL;
	}

	//!!! CP ���� connection �� ���� ���� �ʴ´�. cp reuse �� ���� �ʱ� ����.  2006.12.08
	if(isConnected())
		pSysStat->cpCloseByHost(1);
	this->close();
	return 0;
}

/**
SSL CP ������ ���� ����. isSSL ���� true�� �����Ѵ�.
connection fail (timeout) �ÿ� SSL �̸� �ٸ��� ó���ϰ� �ϱ� ���� ���̴�.\
SSL ó���� transaction ���� ���� ó���ϹǷ� �ٸ��� ó���Ͽ��� �Ѵ�.
*/
int CpHandler::connectSSL(const host_t &host, int port)
{
	startTimeTick(Config::instance()->cp.timeoutCheckInterval);  // Timer ���� 
	
	this->setHost(host);
	this->setPort(port);

	// SSL CP �� ����
	if( !isConnected() )
	{
		if (isConnecting())
			return 0;
			
		if( connect(host, port) == -1 )
		{
			if(isConnected())
				pSysStat->cpCloseByHost(1);
			close();
			return -1;
		}

		changeState (CPS_CONNECTING);
		startConnectTimer(); // ���� �õ� ���̹Ƿ� Timer ����
		
		isSSL = true;

	}
	return 0;
}

int CpHandler::sendSSLData(char* buf, size_t bufSize)
{
	if (get_handle() < 0)
		return -1;

	changeState( CPS_SSL );
	enSendQueue(buf, bufSize);
	#ifdef HTTP_DUMP
	filedump->init("CP-REQ-SSL", 2);
	filedump->write(buf, bufSize);
	#endif
	return 0;
}

int CpHandler::finishSSL(int error_code)
{
	if (error_code ==  RESCODE_CP_CONN)
	{
		// '���� ���� ����'�� ClientHandler���� �˸�, 0 ��  ���и� �ǹ�
		int connFlag = 0;
		pRequester->onCommand(CID_CP_Connected, this, &connFlag);		
	}
	else if(error_code == RESCODE_CP_TIMEOUT)
	{

	}

	isSSL = false;
	return 0;
}

void CpHandler::onReceived()
{
	if (get_handle() < 0)
	{
		PAS_NOTICE1("CpHandler::onReceived - SOCK closed - %s", getMyInfo());
		if(isConnected())
			pSysStat->cpCloseByHost(1);
		close();
		return;
	}

	// re-start timer ( == init timer)
	startReceiveTimer();

	connectedFlag = true;
	
	PAS_TRACE2("CP recv %d bytes %s", recvBuffer.length(), getMyInfo());
	ACE_ASSERT(recvBuffer.length() > 0);
	//PAS_DEBUG_DUMP("recvBuffer", recvBuffer.rd_ptr(), recvBuffer.length());

	// �̺�Ʈ �Լ� ������ state �� ���� �ȴ�.
	while(true)
	{
		// �۾� �� ���� ������
		const size_t oldRecvBufLength = recvBuffer.length();

		// ���¿� ���� �̺�Ʈ �Լ� ȣ��
		switch(state)
		{
		case CPS_WAIT_RESPONSE_HEADER:
			#ifdef HTTP_DUMP
			filedump->init("CP-RESP-HEAD", 3);
			filedump->before(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			onReceivedResponseHeader();

			#ifdef HTTP_DUMP
			filedump->after(recvBuffer.length());
			#endif
			break;

		case CPS_WAIT_RESPONSE_BODY:
			#ifdef HTTP_DUMP
			filedump->init("CP-RESP-BODY", 3);
			filedump->before(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			onReceivedResponseBody();

			#ifdef HTTP_DUMP
			filedump->after(recvBuffer.length());
			#endif
			break;

		case CPS_WAIT_RESPONSE_CHUNK_SIZE:
			#ifdef HTTP_DUMP
			filedump->init("CP-RESP-CHUNK", 3);
			filedump->before(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			onReceivedResponseChunkSize();

			#ifdef HTTP_DUMP
			filedump->after(recvBuffer.length());
			#endif
			
			break;

		case CPS_WAIT_RESPONSE_CHUNK_DATA:
			#ifdef HTTP_DUMP
			filedump->before(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			onReceivedResponseChunkData();

			#ifdef HTTP_DUMP
			filedump->after(recvBuffer.length());
			#endif
			
			break;

		case CPS_SSL:
			#ifdef HTTP_DUMP
			filedump->init("CP-RESP-SSL", 3);
			filedump->write(recvBuffer.rd_ptr(), recvBuffer.length());
			#endif
			
			onReceivedSSLData();

			break;

		default:
			ACE_ASSERT(false);
		}

		// Header/Body ���� �Ϸ� �ƴٸ�
		if(state == CPS_RECEIVED_RESPONSE)
		{
			onCompletedReceiveResponse();
		}

		// �۾� �� ���� ������
		const size_t newRecvBufLength = recvBuffer.length();

		// ���̻� ó���� �����Ͱ� ���ٸ�
		bool noDataProcessed = (oldRecvBufLength == newRecvBufLength);
		if(newRecvBufLength == 0 || noDataProcessed)
			break;
	}

	// ���� ���� ����
	recvBuffer.crunch();

	// ��� �߻�
	bool recvBufferFull = (recvBuffer.space() == 0);
	if(recvBufferFull)
	{
		// ���� �ʱ�ȭ
		recvBuffer.reset();
		changeState (CPS_WAIT_RESPONSE_HEADER );
	}
}



void CpHandler::onReceivedSSLData()
{
	// re-start timer ( == init timer)
	startReceiveTimer();
	
	size_t bufSize = recvBuffer.length();
	pRequester->onCommand(CID_CP_SSLData, this, recvBuffer.rd_ptr(), (void*)&bufSize);
	recvBuffer.reset();
}

/// recv �߿� connection close �� ������ ��� ȣ���
void CpHandler::onRecvFail()
{
	PAS_DEBUG1("Connection closed by CP WEB. %s", getMyInfo());

	if(isConnected())
		pSysStat->cpCloseByHost(1);
	close();
}

/// send �߿� connection close �� ������ ��� ȣ���
void CpHandler::onSendFail()
{
	// 2006.12.19
	// send �� ������ ��� socket close() ����.
	PAS_NOTICE1("Send fail. %s", getMyInfo());
	// 2007.1.4 don't close()
	//close();
}

/**

@todo �Ľ� ������ ��� �ܸ��� ������ ���� ����. ����� abort ó��.
*/
void CpHandler::onReceivedResponseHeader()
{
	ACE_ASSERT(state == CPS_WAIT_RESPONSE_HEADER);

	// ���ŵ� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return;

	if (requestedQueue.empty())
	{
		// CP�� ��û�� �������� �ʾҴµ�  ������ �� ��� �̰ų�, ���α׷� ���� . ^^
		// advance read pointer -- �� ó���� �� ó�� ������ �̵�.
		PAS_INFO1("CpHandler - Recv unexpected http mesg. %s", getMyInfo());
		PHTR_INFO1("CpHandler - Recv unexpected http mesg. %s", getMyInfo());
		PAS_NOTICE_DUMP("Unexpected response header from CP", recvBuffer.rd_ptr(), recvBuffer.length());

		recvBuffer.rd_ptr(recvBuffer.length());
		return;
	}
	
	Transaction *currTransaction = requestedQueue.front();
	ACE_ASSERT(currTransaction != NULL);

	// currHttpResponse�� �����ʹ� request �۽Žÿ� �����ϸ� �ȵǰ�, response ���� �ÿ�  �����ϴ� ���� �´�. 
	// ���������̴��� ����ϱ� �����̴�.
	currHttpResponse = currTransaction->getResponse();

	if (currHttpResponse == NULL)
	{
		PAS_NOTICE("CpHandler::onReceivedResponseHeader >> currHttpResponse is NULL.");
		return;
	}
	
	// ���� �������� http header ����
	HTTP::ResponseHeader resHeader;
	HTTP::header_t header;

	// raw buffer���� http header �κб����� �߶� FastString (header_t) �� �����Ѵ�.
	int ret = resHeader.getHeader(&header, recvBuffer.rd_ptr(), recvBuffer.length());
	if(ret < 0)
	{
		PAS_DEBUG2("CpHandler::waiting more packet to complete Http Header. curr leng=%d. %s", recvBuffer.length(), getMyInfo());
		PHTR_DEBUG2("CpHandler::waiting more packet to complete Http Header. curr leng=%d. %s", recvBuffer.length(), getMyInfo());
		PAS_DEBUG_DUMP("CpHandler::waiting more packet", recvBuffer.rd_ptr(), recvBuffer.length());
		return;
	}

	// advance read pointer
	ACE_ASSERT(header.size() <= recvBuffer.length());
	recvBuffer.rd_ptr(header.size());

	int resultSetHeader = currHttpResponse->setHeader(header);
	if(resultSetHeader < 0)
	{
		PAS_NOTICE1("CpHandler:: Strange Header from CP -%s", getMyInfo());
		PHTR_NOTICE1("CpHandler:: Strange Header from CP -%s", getMyInfo());
		PAS_NOTICE_DUMP("Strange Header from CP", recvBuffer.rd_ptr(), recvBuffer.length());
		return;
	}

	// content-length ?
	bool hasContentData = currHttpResponse->getContentLength() > 0;
	bool hasChunkData = currHttpResponse->hasChunked();
	
	// content-length �������� ó��.
	// content-length �� chunked �� ���ÿ� �����ϴ��� content-length �������� ó��.
	// ����� Transfer-Encoding: chunked �� �����Ͽ� ����. --- old PAS ó�� ����.
	if(hasContentData)
	{
		PAS_DEBUG("Response type is Content-Length.");

		const int loggingContentSize = 1024*1024;
		if(loggingContentSize <= currHttpResponse->getContentLength())
		{
			PAS_INFO3("Big content found. Content size is %d bytes. URL is [%s] %s",
				currHttpResponse->getContentLength(), currTransaction->getRequest()->getHeader()->getUrl().toStr(), setMyInfo());
		}


		// Config ������ Ȯ���Ͽ� ��Ʈ���� ��� ���θ� �����Ѵ�.
		if(Config::instance()->network.streaming)
		{
			if(Config::instance()->network.streamingMinSize <= currHttpResponse->getContentLength())
				currTransaction->streaming(true);
			else
				currTransaction->streaming(false);
		}
		else
			currTransaction->streaming(false);

		pSysStat->cpNormal(1);

		if(currTransaction->streaming())
			pSysStat->cpStream(1);

		changeState ( CPS_WAIT_RESPONSE_BODY );
	}
	else if(hasChunkData)
	{
		PAS_DEBUG("Response type is Chunked.");

		// ûũ �����ʹ� �ؽ�Ű ���������� ���� ��Ʈ������ ����� �� ����.
		currTransaction->streaming(false);
		pSysStat->cpChunked(1);

		changeState ( CPS_WAIT_RESPONSE_CHUNK_SIZE );
	}	
	else
	{
		PAS_DEBUG("Response type is Non Content-Length.");

		// content-length�� ���ٸ� �ؽ�Ű ���������� ���� ��Ʈ������ ����� �� ����.
		currTransaction->streaming(false);
		pSysStat->cpOdd(1);

		const HTTP::ResponseHeader *currHeader = currHttpResponse->getHeader();
		PAS_TRACE2("ContentLengthEmpty[%d] StatusCode[%d]",
			currHeader->getElement("Content-Length").isEmpty() ,
			currHeader->getStatusCode());

		// !! ���� ��Ȳ content-length ���� Chunked �� �ƴ� ���. �׷��鼭 200 �� ���.
		if (currHeader->getElement("Content-Length").isEmpty() && currHeader->getStatusCode() == 200)
		{
			if (recvBuffer.length() > 0)  
				// ������ �ٵ� �ִ� ����̸� �ٵ� �����ϵ��� �Ѵ�.		
				changeState ( CPS_WAIT_RESPONSE_BODY );
			else  
				// ������ �ٵ� �ٷ� ������ �ѹ��� socket read �ؼ� �ٵ� �����ϵ��� �Ѵ�.		
				changeState ( CPS_WAIT_RESPONSE_BODY );

			lastCpRecvSec = 0;
		}

		// ����� ���� �ϴ� ����̴�.
		else
		{
			changeState ( CPS_RECEIVED_RESPONSE );		
		}		
	}
	
	PAS_TRACE2("CP recv header. %s\n%s", getMyInfo(), header.toStr());
	PHTR_DEBUG2("CP recv header. %s\n%s", getMyInfo(), header.toStr());

	// client handler ������ currTransaction �� streaming ���� �̿��ϹǷ�, streaming ���� ��Ȳ�� �°� ������ �Ŀ�
	// onCommand �� ȣ���ؾ� �ϹǷ� �� �ڵ带 ������� �̵� ��Ű�� �ȵȴ�.
	CpResponseData dataInfo(header.front(), header.size(), currTransaction);
	pRequester->onCommand(CID_CP_Received_Header, this, &dataInfo);
}

bool CpHandler::isCompletedReceiveResponseBody()
{
	if (currHttpResponse == NULL)
		return false;

	ACE_ASSERT(currHttpResponse->valid());

	// 2007.1.3 ������ ����Ʈ�� �� ���Ƶ� OK ó��.
	// onReceivedResponseBody() ���� appendSize �� minus ��� �߻��Ͽ� ������.
	return (static_cast<int>(currHttpResponse->getContentLength()) <= _receivedBodySize);
}

/**
!! ���� ��Ȳ content-length ���� Chunked �� �ƴ� ��츦 ó���ϱ� ����.

handle_timeout() ���� �� �̻� response �޽����� ���� ������ �Ǵ��ϰ�,
�޽��� ������ �Ϸ� ó���ϵ��� �Ǿ� �ִ�.
*/
void CpHandler::onOddResponseBody()
{
	// lastCpRecvSec != 0 �̸�  CP ������ ���� ������ �ǹ��Ѵ�. !! 
	lastCpRecvSec = time(NULL);
	
	_receivedBodySize += recvBuffer.length();

	const int loggingContentSize = 1024*1024;
	if((_receivedBodySize - recvBuffer.length()) < loggingContentSize && loggingContentSize <= _receivedBodySize)
	{
		ACE_ASSERT( !requestedQueue.empty() );
		Transaction* tr = requestedQueue.front();

		PAS_NOTICE3("Big content found. URL[%s] is over %d bytes. Transfer method is Non-Content-Length. %s", 
			tr->getRequest()->getHeader()->getUrl().toStr(), loggingContentSize, setMyInfo());
	}
	
	if(!requestedQueue.front()->streaming())
	{
		int resultAppend = currHttpResponse->appendBody(recvBuffer.rd_ptr(), recvBuffer.length());	
		if(resultAppend < 0)
		{
			PAS_NOTICE2("onOddResponseBody >> Append error, appendSize[%d], %s", recvBuffer.length(), getMyInfo());
			PHTR_NOTICE2("CpHandler::onOddResponseBody >> Append error, appendSize[%d], %s", recvBuffer.length(), getMyInfo());
			// �̷� ���� ��쿡�� ���� ��ŭ �ܸ��� ��������.
		}
	}

	PAS_TRACE3("Received odd body from CP. body %d/%d bytes. %s", recvBuffer.length(), _receivedBodySize, getMyInfo());
	PHTR_DEBUG3("Received odd body from CP. body %d/%d bytes. %s", recvBuffer.length(), _receivedBodySize, getMyInfo());

	recvBuffer.reset();
}

void CpHandler::onReceivedResponseBody()
{
	PAS_TRACE1("CpHandler::onReceivedResponseBody >> fd[%d]", get_handle());
	PHTR_DEBUG("CpHandler::onReceivedResponseBody");

	ACE_ASSERT(state == CPS_WAIT_RESPONSE_BODY);
	ACE_ASSERT(currHttpResponse != NULL);
	ACE_ASSERT(currHttpResponse->valid());

	// ���ŵ� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return;

	if (currHttpResponse->getContentLength() == 0)
	{
		// content-length ���� Chunked �� �ƴ� ���
		onOddResponseBody();
	}
	else
	{
		onNonOddResponseBody();
	}
}

void CpHandler::onReceivedResponseChunkSize()
{
	PAS_TRACE1("CpHandler::onReceivedResponseChunkSize >> fd[%d]", get_handle());
	PHTR_DEBUG("CpHandler::onReceivedResponseChunkSize");

	ACE_ASSERT(state == CPS_WAIT_RESPONSE_CHUNK_SIZE);
	ACE_ASSERT(currHttpResponse != NULL);
	ACE_ASSERT(currHttpResponse->valid());

	// ���ŵ� �߰� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return;

	const char* pos = (char*)memchr(recvBuffer.rd_ptr(), '\n', recvBuffer.length());

	// ���� �̿Ϸ�
	if(pos == NULL)
		return;

	const char* startPos = recvBuffer.rd_ptr();

	size_t sizeOfChunkSizeStringWithTrailer = pos - startPos + 1;

	// \r\n�� ��� pos �� \r ��ġ�� ����
	if(pos - startPos > 0)
	{
		if(*(pos-1) == '\r')
			--pos;
	}

	//PAS_DEBUG_DUMP("chunk data", recvBuffer.rd_ptr(), recvBuffer.length());

	HTTP::chunkSize_t chunkSizeStr(startPos, pos - startPos);
	recvBuffer.rd_ptr(sizeOfChunkSizeStringWithTrailer);

	//PAS_DEBUG_DUMP("chunk data", recvBuffer.rd_ptr(), recvBuffer.length());

	int chunkSize = strtol(chunkSizeStr, (char **)NULL, 16);

	PAS_TRACE2("onReceivedResponseChunkSize >> chunkSizeStr[%s] chunkSize[%d]", chunkSizeStr.toStr(), chunkSize);
	PHTR_DEBUG2("CpHandler::onReceivedResponseChunkSize >> chunkSizeStr[%s] chunkSize[%d]", chunkSizeStr.toStr(), chunkSize);

	if(chunkSize < 0)
	{
		PAS_ERROR1("CpHandler::onReceivedResponseChunkSize >> Chunk Size ��ȯ ����, fd[%d]", get_handle());
		PHTR_DEBUG("CpHandler::onReceivedResponseChunkSize >> Chunk Size ��ȯ ����");
		return;
	}

	currHttpResponse->setChunkSize(chunkSize);
	currHttpResponse->setRecevicedChunkSize(0);

	if(chunkSize > 0)
		changeState ( CPS_WAIT_RESPONSE_CHUNK_DATA );
	else {
		/*
		2006.09.24 - handol
		chunkSize �� 0 �� ���� �������� �ִ� ��찡 �ִ�.
		�� ��쿡�� \r\n �� �о� �־�� �Ѵ�.
		*/
		if (recvBuffer.length() >= 2) {
			PAS_TRACE1("Before absorbing last chunk trailer. Remain buffer size is %d", recvBuffer.length());
			const char *endline = recvBuffer.rd_ptr();
			if (endline[0]=='\n') 
				recvBuffer.rd_ptr(1);
			if (endline[0]=='\r' && endline[1]=='\n') 
				recvBuffer.rd_ptr(2);
			PAS_TRACE1("After absorbing last chunk trailer. Remain buffer size is %d", recvBuffer.length());
		}

		PAS_DEBUG("Chuncked data receive completed.");
			
		changeState ( CPS_RECEIVED_RESPONSE );
	}
}

void CpHandler::onReceivedResponseChunkData()
{
	PAS_TRACE1("onReceivedResponseChunkData >> fd[%d]", get_handle());
	PHTR_DEBUG("CpHandler::onReceivedResponseChunkData");

	ACE_ASSERT(state == CPS_WAIT_RESPONSE_CHUNK_DATA);
	ACE_ASSERT(currHttpResponse != NULL);
	ACE_ASSERT(currHttpResponse->valid());

	// ���ŵ� �߰� �����Ͱ� ���ٸ� skip
	if(recvBuffer.length() == 0)
		return;

	size_t remainSize = currHttpResponse->getChunkSize() - currHttpResponse->getReceivedChunkSize();

	// �� �����ؾ��� chunk �����Ͱ� �ִٸ�
	if(remainSize > 0)
	{
		size_t appendSize = std::min(recvBuffer.length(), remainSize);
		int resultAppend = currHttpResponse->appendBody(recvBuffer.rd_ptr(), appendSize);
		if(resultAppend < 0)
		{
			PAS_ERROR1("CpHandler::onReceivedResponseChunkData >> ������ �ٿ� �ֱ� ����, appendSize[%d]", appendSize);
			PHTR_ERROR1("CpHandler::onReceivedResponseChunkData >> ������ �ٿ� �ֱ� ����, appendSize[%d]", appendSize);
		}

		_receivedBodySize += appendSize;

		currHttpResponse->setRecevicedChunkSize(currHttpResponse->getReceivedChunkSize() + appendSize);

		ACE_ASSERT(appendSize <= recvBuffer.length());
		recvBuffer.rd_ptr(appendSize);

		const int loggingContentSize = 1024*1024;
		if((currHttpResponse->getBodyLeng() - appendSize) < loggingContentSize && loggingContentSize <= _receivedBodySize)
		{
			ACE_ASSERT( !requestedQueue.empty() );
			Transaction* tr = requestedQueue.front();

			PAS_NOTICE3("Big content found. URL[%s] is over %d bytes. Transfer method is chunked data. %s", 
				tr->getRequest()->getHeader()->getUrl().toStr(), loggingContentSize, setMyInfo());
		}
	}

	//PAS_DEBUG_DUMP("chunk data", recvBuffer.rd_ptr(), recvBuffer.length());

	remainSize = currHttpResponse->getChunkSize() - currHttpResponse->getReceivedChunkSize();
	bool completeChunkDataReceive = (remainSize == 0);

	if(completeChunkDataReceive)
	{
		// chunk ������ ���Ŀ� ������� "\r\n" skip �ϱ�
		if(recvBuffer.length() >= 1)
		{
			if(*recvBuffer.rd_ptr() == '\n')
			{
				recvBuffer.rd_ptr(1);
				changeState ( CPS_WAIT_RESPONSE_CHUNK_SIZE );
			}
			else if(recvBuffer.length() >= 2)
			{
				if(*recvBuffer.rd_ptr() == '\r' && *(recvBuffer.rd_ptr()+1) == '\n')
				{
					recvBuffer.rd_ptr(2);
					changeState ( CPS_WAIT_RESPONSE_CHUNK_SIZE );
				}
				else
				{
					// chunk data�� �����ٸ�, �� "\r\n"�� �־�� �Ѵ�.
					// ���� ��ŭ�� ���� ������.
					// !!! 2006.10.20
					changeState ( CPS_RECEIVED_RESPONSE );
					if (requestedQueue.size() <= 1)
					{
						// ���ۿ� ���� ������ �о� ������.
						PAS_INFO2("CpHandler::onReceivedResponseChunkData() - CONSUME garbage after chunk data. [%d] bytes - %s",
							recvBuffer.length(), getMyInfo());
						recvBuffer.rd_ptr(recvBuffer.length());
					}
				}
			}
		}
	}
}

/**
Contents-Length ���� �� ���� �� ��� ���� ó��.
*/
int CpHandler::handleOddLengthResponse()
{
	if (requestedQueue.size() > 1) // pipelining �� ���� ���� ó������ �ʴ´�.
		return 0;

	if (recvBuffer.length() == 0)
		return 0;
	
	PAS_INFO1("CpHandler::handleOddLengthResponse - Receive more than Contents-Length. Recv size is [%d]", recvBuffer.length());
	PHTR_INFO1("CpHandler::handleOddLengthResponse - Receive more than Contents-Length. Recv size is [%d]", recvBuffer.length());

	if(requestedQueue.front()->streaming())
	{
		// ���� �ִ� �����͸� �׳� ����
		recvBuffer.reset();
	}
	else
	{
		// ���� �ִ� �����͸� responseBody�� ����
		int orgBodyLeng = currHttpResponse->getBodyLeng();
		int resultAppend = currHttpResponse->appendBody(recvBuffer.rd_ptr(), recvBuffer.length());
		if(resultAppend < 0)
		{
			PAS_ERROR1("CpHandler::handleOddLengthResponse >> ������ �ٿ� �ֱ� ����, appendSize[%d]", recvBuffer.length());
			PHTR_ERROR1("CpHandler::handleOddLengthResponse >> ������ �ٿ� �ֱ� ����, appendSize[%d]", recvBuffer.length());
		}

		_receivedBodySize += recvBuffer.length();

		// advance read pointer
		recvBuffer.rd_ptr(recvBuffer.length());

		PAS_INFO3("CpHandler::handleOddLengthResponse - LENG: org=%d contents=%d new=%d",
			orgBodyLeng, currHttpResponse->getContentLength(), currHttpResponse->getBodyLeng());
		PHTR_INFO3("CpHandler::handleOddLengthResponse - LENG: org=%d contents=%d new=%d",
			orgBodyLeng, currHttpResponse->getContentLength(), currHttpResponse->getBodyLeng());
	}

	return recvBuffer.length();
}

/**
 onReceived() ���� CP ������ ���� �Ϸ��� ��� ȣ���Ѵ�. CP ������ chunked �� ���� �ƴ� ��� ��� ó���Ѵ�.
*/
int CpHandler::onCompletedReceiveResponse()
{
	ACE_ASSERT(pRequester != NULL);
	ACE_ASSERT(currHttpResponse != NULL);
	ACE_ASSERT(currHttpResponse->valid());

	lastCpRecvSec = 0;	
	
	if(currHttpResponse->getBodyBufferExpandCount() > 5)
	{
		PAS_INFO4("Body buffer expanded %d times. contents[%d] body[%d] %s",
			currHttpResponse->getBodyBufferExpandCount(), currHttpResponse->getContentLength(),
			currHttpResponse->getBodyLeng(), getMyInfo());
		PHTR_INFO4("Body buffer expanded %d times. contents[%d] body[%d] %s",
			currHttpResponse->getBodyBufferExpandCount(), currHttpResponse->getContentLength(),
			currHttpResponse->getBodyLeng(), getMyInfo());
	}
	
	changeState ( CPS_WAIT_RESPONSE_HEADER );

	int	http_res_code = currHttpResponse->getHeader()->getStatusCode();
	if (http_res_code == HTTP_CODE_100_CONTINUE || http_res_code == HTTP_CODE_101_SWITCH)
	{
		// �̷��� ������ HTTP response�� �ܸ��� �������� �ʰ�, PAS ���������� ó���ϰ� 
		// �� ���� Response �� ��ٷ��� �Ѵ�.

		PAS_DEBUG1("%s CP HTTP CONTINUE recved", getMyInfo());
		PHTR_DEBUG1("%s CP HTTP CONTINUE recved", getMyInfo());

		// ���ο� ����Ÿ�� �޾ƾ� �ϹǷ� Ŭ���� �� ����. -- 2006.10.17
		currHttpResponse->clear();
		return 0;
	}

	// Chunked �̸鼭 ���ÿ� Contents-Length �� �ִ� ���, Contents-Length ���� �� ���� �� ��� ���� ���� ó��.
	handleOddLengthResponse();
	
	// ��Ʈ������ �ƴ� ���� ���� �����͸� �����Ѵ�.
	// create Content-Length
	ACE_ASSERT( !requestedQueue.empty() );
	if( !requestedQueue.front()->streaming() )
	{
		// chunk to Content-Length
		if( currHttpResponse->hasChunked() )
		{
			if ( !currHttpResponse->getHeader()->getElement("Content-Length").isEmpty() )
			{	
				// Chunked �̸鼭 Content-Length �� �ִ� ��� -- web server�� �� �̻��� ������� ^^
				// CGI ���α׷��� �̻��ϰ� �ۼ��ϸ� �߻� ����.
				PAS_NOTICE2("CP %s:%d replied odd Response. Content-Length in Chunked", _host.toStr(), _port);
				PHTR_NOTICE2("CP %s:%d replied odd Response. Content-Length in Chunked", _host.toStr(), _port);
			}
			
			reformatHeader(currHttpResponse);
		}

		// create Content-Length for old web server
		else if ( currHttpResponse->getContentLength() == 0 )
		{
			reformatHeader(currHttpResponse);
		}
	}

	PAS_DEBUG3("Received Response from CP. HeadSize[%d] ContentLength[%d] Body[%d]", 
			currHttpResponse->getHeadLeng(), currHttpResponse->getContentLength(), _receivedBodySize);

	// !!! finish normally
	finish();
	
	return 0;
}

/**
Content-Length ����� ������Ʈ �Ѵ�.
�� �Լ��� Chunked �� ��쿡  ȣ��ȴ�. - "Transfer-Encoding" �� �����ϰ�, Content-Length �� �߰�.
Chunked �ƴϸ鼭 Content-Length �� ���� ��쿡�� ȣ��ȴ�. �̻��� ������.  - Content-Length �� 0 ���� �����Ѵ�.
2006-11-28
*/
int	CpHandler::reformatHeader(HTTP::Response* resultHttpResponse)
{

	HTTP::ResponseHeader resHeader(*resultHttpResponse->getHeader());

	if (! resHeader.getElement("Transfer-Encoding").isEmpty())
	{
		if (resHeader.delElement("Transfer-Encoding"))
			PAS_INFO1("CpHandler::reformatHeader() - delElement(\"Transfer-Encoding\") FAIL - %s",
				getMyInfo());
	}

	
	if(resHeader.getElement("Content-Length").isEmpty())
	{
		// ���� ���̽�. �翬�� "Content-Length" ����� �Ѵ�.
		resHeader.addElement("Content-Length", resultHttpResponse->getBodyLeng());
	}
	else
	{
		// Chunked �̸鼭 Content-Length �� �ִ� ��� -- web server�� �� �̻��� ������� ^^
		//resHeader.replaceElement("Content-Length", resultHttpResponse->getBodyLeng());

		//replace ���� �ʰ� ��� �߰�.
		resHeader.addElement("Content-Length", resultHttpResponse->getBodyLeng());
	}
	
	

	HTTP::header_t newRawHeader;
	int ret = resHeader.build(&newRawHeader);
	if(ret < 0)
	{
		PAS_ERROR1("CpHandler::onCompletedReceiveResponse >> ��� ���� ����, fd[%d]", get_handle());
		PHTR_ERROR("CpHandler::onCompletedReceiveResponse >> ��� ���� ����");
		return -1;
	}

	ret = currHttpResponse->setHeader(newRawHeader);
	if(ret < 0)
	{
		PAS_ERROR1("CpHandler::onCompletedReceiveResponse >> ��� ���� ����, fd[%d]", get_handle());
		PHTR_ERROR("CpHandler::onCompletedReceiveResponse >> ��� ���� ����");
		return -1;
	}

	PAS_TRACE1("CpHandler::reformatHeader\n%s", newRawHeader.toStr());

	return 0;
}

host_t CpHandler::getHost()
{
	return _host;
}

int CpHandler::getPort()
{
	return _port;
}

int CpHandler::setHost(host_t host)
{
	ACE_ASSERT(host.size() > 0);

	_host = host;
	return 0;
}

int CpHandler::setPort(const int port)
{
	// ��Ʈ�� 80 ~ 65535 ������ �Ѿ�� �⺻��Ʈ(80)�� ����
	if(80 > port || port > 0xFFFF)
		_port = 80;

	else
		_port = port;

	return 0;
}

int CpHandler::handle_timeout(const ACE_Time_Value &current_time, const void* /* act */)
{
	PAS_TRACE1("CpHandler::handle_timeout >> fd[%d]", get_handle());
	PHTR_INFO("CpHandler::handle_timeout");

	const CPConfig& cpConfig = Config::instance()->cp;
	ACE_Time_Value connTimeout(cpConfig.connectionTimeout);
	ACE_Time_Value recvTimeout(cpConfig.receiveTimeout);

	if (lastCpRecvSec != 0)
	{
		time_t now = time(NULL);
	 	int diff = now - lastCpRecvSec;
	 	if (diff >= 2)
		{
	 		PAS_DEBUG3("CP COMPLETE ODD BODY [%s:%d] %s", _host.toStr(), _port, getMyInfo());
	 		PHTR_INFO3("CP COMPLETE ODD BODY [%s:%d] %s", _host.toStr(), _port, getMyInfo());
	 		changeState ( CPS_RECEIVED_RESPONSE );
	 		onCompletedReceiveResponse();
	 		return 0;
	 	}
	}
	
	// �ð��� ����Ǹ� ����
	if(isConnectTimeOut(current_time, connTimeout))
	{
		PAS_INFO1("CP connect timeout %s", getMyInfo());
		PHTR_INFO1("CP connect timeout %s", getMyInfo());

		if (isSSL)
			finishSSL(RESCODE_CP_CONN);
		else
			finishAll(RESCODE_CP_CONN);

		if(isConnected())
			pSysStat->cpCloseByHost(1);
		close();

	}
	else if(isIdle(current_time, recvTimeout))
	{
		if (lastCpRecvSec != 0)
		{
	 		PAS_INFO1("CP COMPLETE ODD BODY %s", getMyInfo());
			PHTR_INFO1("CP COMPLETE ODD BODY %s", getMyInfo());
	 		changeState ( CPS_RECEIVED_RESPONSE );
	 		onCompletedReceiveResponse();
	 		return 0;
	 	}

		PAS_INFO2("CP recv timeout. Q=%d %s",  requestedQueue.size(), getMyInfo());
		PHTR_INFO2("CP recv timeout. Q=%d %s", requestedQueue.size(), getMyInfo());

		
		// timeout ���� ClientHandler �� �˷��־�� �Ѵ�. 
		if (isSSL) 
		{
			finishSSL(RESCODE_CP_TIMEOUT);
		}
		else 
		{
			// �̹� ó���Ǿ��µ� ��� timeout �߻��ΰ�?  -- �̻� ��Ȳ
			if (requestedQueue.empty())
			{
				PAS_INFO1("CP recv timeout with Nothing in Q %s", getMyInfo());
				PHTR_INFO1("CP recv timeout with Nothing in Q %s", getMyInfo());
			}
			else
			{
				finishAll(RESCODE_CP_TIMEOUT);
			}			
		}

		if(isConnected())
			pSysStat->cpCloseByHost(1);
		close();
	}
	
	return 0;
}

bool CpHandler::isRemovable()
{
	if( requestedQueue.size() > 0 )
		jobDone = false;

	else
		jobDone = true;

	return jobDone;
}

char* CpHandler::setMyInfo()
{
	Transaction *tr = NULL;
	if (requestedQueue.size() > 0)
		tr = requestedQueue.front();
		
	if (tr)
		snprintf(myinfo, MYID_LEN, "MDN[%s] CPHost[%s:%d] Sock[%d]", 
			tr->phoneNumber,  _host.toStr(), _port, sock.get_handle());
	else
		snprintf(myinfo, MYID_LEN, "CPHost[%s:%d] Sock[%d]", 
			_host.toStr(), _port, sock.get_handle());

	return myinfo;
}

bool CpHandler::isConnecting()
{
	return (state == CPS_CONNECTING);
}

void CpHandler::onNonOddResponseBody()
{
	if(recvBuffer.length() == 0)
		return;

	// content length	
	const int remainSize = currHttpResponse->getContentLength() - _receivedBodySize;

	// 2007.1.3 ������ ����Ʈ�� contentLength ���� �� ���Ƶ� OK ó��.
	// onReceivedResponseBody() ���� appendSize �� minus ��� �߻��Ͽ� ������.
	if (remainSize < 0)
	{
		PAS_NOTICE3("CpHandler: remainSize[%d] recv[%d] %s", remainSize, recvBuffer.length(), getMyInfo());
		PHTR_NOTICE3("CpHandler: remainSize[%d] recv[%d] %s", remainSize, recvBuffer.length(), getMyInfo());

		// 2007.1.3 �̷� ���� ��쿡�� ���� ��ŭ �ܸ��� ��������.
		changeState ( CPS_RECEIVED_RESPONSE );
		return;
	}

	const int partOfBodySize = std::min(remainSize, (int)recvBuffer.length());

	ACE_ASSERT(0 < partOfBodySize && partOfBodySize <= recvBuffer.length());
	onReceivedPartOfBody(recvBuffer.rd_ptr(), partOfBodySize);

	if(!requestedQueue.front()->streaming())
	{
		// ������ �����͸� responseBody�� ����
		int resultAppend = currHttpResponse->appendBody(recvBuffer.rd_ptr(), partOfBodySize);
		if(resultAppend < 0)
		{
			PAS_NOTICE2("CpHandler::onReceivedResponseBody >> Append error, appendSize[%d] %s", partOfBodySize, getMyInfo());
			PHTR_NOTICE2("CpHandler::onReceivedResponseBody >> Append error, appendSize[%d] %s", partOfBodySize, getMyInfo());
			// �̷� ���� ��쿡�� ���� ��ŭ �ܸ��� ��������.
		}
	}

	// advance read pointer
	recvBuffer.rd_ptr(partOfBodySize);

	// �ٵ� ���� �Ϸ�?
	if(isCompletedReceiveResponseBody())
	{
		PAS_TRACE2("Body receive complete. Body is %d bytes. ContentLength is %d bytes.",
			_receivedBodySize, currHttpResponse->getContentLength());
		PHTR_DEBUG2("CP recv body %d bytes. %s", _receivedBodySize, getMyInfo());

		changeState ( CPS_RECEIVED_RESPONSE );
	}
	else
	{
		PAS_TRACE("Body receive incomplete.");
	}
}

void CpHandler::onReceivedPartOfBody( const char* srcBuf, const int srcSize )
{
	_receivedBodySize += srcSize;

	if (!requestedQueue.empty())
	{
		CpResponseData dataInfo(srcBuf, srcSize, requestedQueue.front());
		pRequester->onCommand(CID_CP_Received_PartOfBody, this, &dataInfo);
	}
}

void CpHandler::reset( const host_t& host, const int port )
{
	if(isConnected())
		pSysStat->cpCloseByHost(1);

	close();
	stopTimeTick();
	stopConnectTimer();

	setHost(host);
	setPort(port);
}

int CpHandler::getReceiveBodySize()
{
	return _receivedBodySize;
}

void CpHandler::onCloseByPeer()
{
	pSysStat->cpCloseByPeer(1);
	close();
}

