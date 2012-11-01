#ifndef __CLIENT_EVENT_HANDLER_H__
#define __CLIENT_EVENT_HANDLER_H__

#include <ace/Reactor.h>
#include <ace/Message_Block.h>

#include "Common.h"
#include "ReactorPool.h"
#include "UserInfo.h"
#include "Session.h"
#include "TransactionInfo.h"
#include "PasEventHandler.h"
#include "HttpRequestHeader.h"
#include "HttpRequest.h"
#include "WorkInfo.h"
#include "CpEventHandler.h"
#include "AuthHandler.h"
#include "CpList.h"
#include "SantaResponse.h"
#include "SantaEventHandler.h"
#include "UserInfoMng.h"
#include "PasDataLog.h"
#include "ResponseBuilder.h"
#include "HttpKTFSSLResult.h"
#include "FileDump.h"
#include "GuideCode.h"
#include "GuideReadResponse.h"
#include "CorrelationKey.h"

/**
@brief PASGW�� �ܸ� ���� ����� ����

 * �ܸ���� ���������� ����ϰ�, CpHandler�� ���� CP WEB�� ���������� ����ϰ�,
 * AuthAgent�� ���� PasAuth�� ���������� ����ϰ�, SantaHandler�� ���� Santa�� ���������� ����Ѵ�.
 *
 * �ܸ���� ���� ��û�� �޾� PasAuth�� Santa�� ���� ������ �ϰ�, CpHandler�� ���� CP WEB���� �����͸� ��û�Ͽ�
 * CP WEB���� ���� ������ �����͸� �ܸ���� Relay �Ѵ�.
 *
 * �� ��ü�� �����ֱ�� �ܸ��Ⱑ PASGW�� ������ �ϰ� �Ǹ� PasAcceptEventHandler�� handle_input���� �������ϰ�,
 * �ܸ��Ⱑ ������ �����ϸ� ���Ÿ� �Ѵ�.
 * �ܸ��Ⱑ �ɵ������� ���� ���Ḧ ���� �ʰ�, ��ð� idle ���·� ������ ClientHandler�� ��ü���� Timeout �̺�Ʈ�� ����
 * �ܸ������ ������ ���� �����Ѵ�.
 *
 * ��ü ���Ÿ� �õ��� �� CpHandler, AuthEventHandler, SantaHandler�� ����Ǿ� �ִ��� Ȯ���ϰ� ���� ����� ����
 * �Ѱ� �̻� �����Ѵٸ� ������ ��� ����Ǳ� ��ٸ� ��, ��� ������ ������� �� ClientHandler �� ������ WorkQueue�� �ڽ��� �����ϵ��� �ϴ� �۾��� ����Ѵ�.
 * ����, ����� �ܺ� ��ü(CpHandler, AuthEventHandler, SantaHandler)�� ���ٸ� ��� ClientHandler ��ü�� �����Ѵ�.
 */

#define	FREE_FRONT	(1)
#define	FREE_BACK	(2)

class ClientHandler :
	public PasHandler
{
	//-------------------------------------------------------
	// ��� Ÿ��
	//-------------------------------------------------------
protected:
	enum AclResult
	{
		AR_HOLD_PROXY,			///< PROXY ���� ���ʿ�
		AR_CHANGE_PROXY,		///< PROXY ���� �ʿ�
		ACL_DNS_APPLIED
	};

private:
	enum ClientState
	{
		CS_WAIT_REQUEST_HEADER,	///< Client�� ���� �������� ��û����� ��ٸ��� ����
		CS_WAIT_REQUEST_BODY,	///< Client�� ���� ��û��� ���� �� ��û�ٵ� ��ٸ��� ����
		CS_RECEIVED_REQUEST,	///< Client�� ���� ��û���/�ٵ� ��� ���� �Ϸ��� ����
		CS_CLOSING,				///< ClientHandler ���� �õ� ����
		CS_SSL_CONNECTING,		/// SSL ������ ���� ���̸� ���� ��� 
		CS_SANTA_WAITING,		/// SANTA ������ ��ٸ��� ��.
		CS_SSL
	};

public:
	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------
	ClientHandler(ReactorInfo* rInfo);
	virtual ~ClientHandler(void);
	void init(int newsockfd);

	//-------------------------------------------------------
	// ��� ����
	//-------------------------------------------------------
	static unsigned int numTransactionAlloc;
	static unsigned int numRequests;
	static unsigned int numClientHandlers;
	static unsigned int numResponses;

	static unsigned int sentDirectCnt;
	static unsigned int sentByQueCnt;

protected:
	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------

	/// ACL ������ ��ȸ�Ͽ� ������� �Ѵ�.
	/**
	* ACL ����� ��å�� ���� ��û�ؾ� �� CP�� Host/Port ���� ��ȯ�Ѵ�.
	* Client ��û�� ���� Proxy���� ó���ؾ� �� ���� AR_HOLD_PROXY�� �����ϰ�,
	* �ٸ� Proxy���� ó���ؾ� �� ��� AR_CHANGE_PROXY�� �����Ѵ�.
	*
	* @note AR_CHANGE_PROXY�� ���� �޾Ҵٸ�, proxyHost�� proxyPort �� ����� ������ Client�� �������ϵ��� �����Ѵ�.
	*       AR_HOLD_PROXY�� ���� �޾Ҵٸ�, proxyHost/proxyPort�� ������� �ʾƾ� �Ѵ�.
	*
	* @return ���� Proxy�������� ��û�� ó���ؾ� �� ��� AR_HOLD_PROXY�� �����ϰ�,
	*         �ٸ� Proxy���� ��û�� ó���ؾ� �� ��� AR_CHANGE_PROXY�� �����Ѵ�.
	**/
	AclResult applyACL(HTTP::Request* pRequest, host_t& proxyHost, int& proxyPort);
	
	void afterCpTransaction(Transaction *tr);

	//-------------------------------------------------------
	// ��� ����
	//-------------------------------------------------------
	Session	session;
	UserInfo* userInfo;


private:
	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------

	virtual void preStartTransaction(Transaction* tr) {}
	virtual void preAfterCpTransaction(Transaction* tr) {}

	/// CP���� ���������� ��û�ϱ� ������ �̺�Ʈ
	virtual void preRequestToCP(Transaction *tr) {}
	virtual int browserTypeCheck( Transaction * tr ) = 0;

	/// send �߿� connection close �� ������ ��� ȣ���
	virtual void onCloseByPeer();
	virtual void onRecvFail();
	virtual void onSendFail();

	/// �۽� �����Ͱ� ��� �۽ŵǾ��� ���
	virtual void onSendQueueEmpty();

	/// timer�� ������� �� ȣ�� �Ǵ� �Լ�
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act=0);

	/// timer�� ������� �� ȣ�� �Ǵ� �Լ�
	virtual int handle_exception(ACE_HANDLE fd);

	/// Client�� ����� �������� ���� �����Ͱ� ���ŵ��� ��
	virtual void onReceived();

	/// CP�� Auth�� Santa�� ���� �̺�Ʈ�� �߻����� �� ȣ���ϴ� �Լ�
	virtual void onCommand(CommandID cid, PasHandler* pEH, void* arg1 = NULL, void* arg2 = NULL);

	virtual void close();

	void startSession();
	void finishSession();

	int	consumeRecvBuffer();

	bool isHttpHeaderCandidate(char *buff, int size);
		
	/// ��û ������ �ٵ� ��� ���� �ƴ��� Ȯ��
	/**
	 * ��û ����� content-length�� ���� ���ŵ� �ٵ��� ����� ���Ѵ�.
	 *
	 * @return ��� ���ŵƴٸ� true, �ƴ϶�� false
	 **/
	bool isCompletedReceiveRequestBody();

	/// Client�� ���� ��û ������ ����� ���� �ɶ�
	int onReceivedRequestHeader();

	/// Client�� ���� ��û ������ �ٵ� ���� �ɶ�
	int onReceivedRequestBody();

	/// SSL ������ ������ ����;
	int onReceivedSSLData(Transaction *tr);

	/// Client�� ���� ��û ������ ���� ���� ��
	int onCompletedReceiveRequest();

	/// CP�� ��û�� ���������� ������ �Ϸ� ���� ��
	void onCompletedCPResponse(CpHandler* pCP,  Transaction *resTransaction);

	/// CP WEB���� ������ �������� ��
	void onCompletedCPConnection(CpHandler* pCP, int isSucc);

	/// CP�� ���� Header�� �������� ��
	/**
	* @exception no throw
	*
	* @date 2007/03/26
	* @author Sehoon Yang
	**/
	void onReceivedHeaderFromCP(CpHandler* pCP, CpResponseData* pData);

	/// CP�� ���� body �������� �Ϻθ� �������� ��
	/**
	 * @exception no throw
	 *
	 * @date 2007/03/26
	 * @author Sehoon Yang
	 **/
	void onReceivedPartialBodyFromCP(CpHandler* pCP, CpResponseData* pData);
	

	/// CP�� ��Ʈ��ũ ������ �õ��Ѵ�.
	/**
	 * ��û�� CP�� host/port�� �̹� ���ӵ� CpEH�� CpList�� �����Ѵٸ�, �ش� CpEH�� �����Ѵ�.
	 * CpList�� �������� �ʴ´ٸ�, CpEH�� Heap �� �����ϰ� ���� ���� ��û�� �ϰ�, CpList�� ������ CpEH�� ����� �� CpEH�� �����Ѵ�.
	 * CpHandler �� ���ϵƴٰ� �ؼ� ������ �Ϸ������ �ǹ��ϴ� ���� �ƴϴ�.
	 *
	 * @param host ���� �õ� �� host
	 * @param port ���� �õ� �� port
	 * @return CpHandler
	 **/
	CpHandler* pickCpHandler(const host_t& host, int port);


	/// PasAuth�� ��Ʈ��ũ ������ �õ��Ѵ�.
	/**
	 * Heap �� AuthEventHandler�� ������ �� ��������� pAuthEventHandler�� Heap�ּҸ� ����ϰ�, ���� ������ ��û�Ѵ�.
	 * 0 �� ���ϵƴٰ� �ؼ� ������ �Ϸ������ �ǹ��ϴ� ���� �ƴϴ�.
	 *
	 * @param host ���� �õ� �� host
	 * @param port ���� �õ� �� port
	 * @return ���� �õ��� ���������� ����ƴٸ� 0, �����ߴٸ� -1
	 **/
	int requestAuth(Transaction *tr, const host_t& host, int port);

	/// Santa�� ��Ʈ��ũ ������ �õ��Ѵ�.
	/**
	 * Heap �� SantaHandler�� ������ �� ��������� pSantaHandler�� Heap�ּҸ� ����ϰ�, ���� ������ ��û�Ѵ�.
	 * 0 �� ���ϵƴٰ� �ؼ� ������ �Ϸ������ �ǹ��ϴ� ���� �ƴϴ�.
	 *
	 * @param host ���� �õ� �� host
	 * @param port ���� �õ� �� port
	 * @return ���� �õ��� ���������� ����ƴٸ� 0, �����ߴٸ� -1
	 **/

	/// ��������� hashkey�� �߰��Ѵ�.
	int addHashKey(HTTP::ResponseHeader* pDestHeader, const url_t& requestUrl, const size_t requestSize, int responseBodyLeng);

	/// ��������� bill info �� �߰��Ѵ�.
	int addBillInfo(HTTP::ResponseHeader* pDestHeader);
	int addGuidePageBillInfo(HTTP::ResponseHeader* pDestHeader);

	/// ��������� Proxy Info �� �߰��Ѵ�.
	void addProxyInfo(HTTP::RequestHeader* pDestHeader);

	/// CP���� ���������� ��û�Ѵ�.
	/**
	 * ��������� pHttpRequest�� �̿��� CP���� �������� ��û�� �Ѵ�.
	 **/
	void requestToCP(Transaction *tr);

	/// Hot Number ������ ��ȸ�Ͽ� ������� �Ѵ�.
	bool applyHotNumber(Transaction *tr);

	/// CP�� ��û�� ���������� ������ �Ϸ� ���� ��
	int onCommandCPCompleted(CpHandler* pCP,  Transaction *resTransaction);

	/// CP ������ �Ϸ� ���� ��
	int onCommandCPConnected(CpHandler* pCP, int isSucc);

	/// CP�� ���� ������ ���� ���� ��
	int onCommandCPClosed(CpHandler* pCP);

	/// CP WEB���κ��� SSL DATA�� �������� ��
	int onCommandCPReceivedSSLData(CpHandler* pCP, const char* buf, size_t bufSize);

	/// Santa�� ��û�� ������ ���� �Ϸ� ���� ��
	int onCommandSantaCompleted(SantaHandler* pSanta, const SANTA::Response* pResponse);

	/// Santa�� ���� ������ ���� ���� ��
	int onCommandSantaClosed(SantaHandler* pSanta);

	/// Santa Timeout
	int onCommandSantaTimeOut(SantaHandler* pSanta);

	int checkSanta(Transaction *tr);
	
	/// Client(�ܸ���)�� ���񽺸� ���������� �̿��� �� �ִ��� Ȯ��
	/**
	 * �����͸� ��û�� Client�� ���������� ���񽺸� �̿��ص� �Ǵ� ��������� �ƴ����� PasAuth�� Santa�� ���� ������ �Ѵ�.
	 *
	 * @return ���񽺸� �̿��ص� �Ǵ� ��� 0�� �����ϰ�, �׷��� ���� ��� -1�� �����Ѵ�.
	 */
	int checkAuth(Transaction *tr);

	// AuthAgent �� ���� request �� �ۼ��Ѵ�.
	void makeRequestAuth(AUTH::RequestBody &body, const int seq, const char* pMin, const unsigned int ip, const int port, const int startConn, const int newBrowser, int g3GCode, const char* pMdn, const char* pMsModel);

	bool checkStatFilter(Transaction *tr, char *notimesg );

	int	getUserInfo(Transaction *tr);
	int	getUserInfo_SSL();

	int allocTransaction();

	void freeTransaction(int flag_pos, Transaction *tr_to_free);

	void freeTransactionFromQ();

	void midTransaction(Transaction *tr);

	int additionalInfoToReqHeader( Transaction * tr );

	virtual int procACL(Transaction* tr) = 0;

	HTTP::ResponseHeader additionalInfoToResHeader(const HTTP::ResponseHeader& header, const char* phoneNumber, const url_t& orignalRequestURL, 
		const int requestDataSize, const int responseBodySize, const host_t& requestHost, const CorrelationKey& correlationKey);
	
	int startTransaction(Transaction *startedTransaction);

	int finishTransaction(Transaction *finishedTransaction);

	int	sendRespToPhoneHeadbody(HTTP::Response* resToPhone);
	int sendRespToPhone(HTTP::Response* resToPhone);

	virtual	char* setMyInfo();

	// CP ���� SSL ���� ��û
	void requestSSLConnection(Transaction *tr, const host_t& host, const int port);
	
	// Client ���� SSL ������ ���������� �˸�
	void responseSSLConnectionSuccess();

	// Client ���� SSL ������ ���������� �˸�
	void responseSSLConnectionFail();

	// SSL Ʈ����� ���� ���
	void storeSSLTransaction(const HTTP::KTFSSLResult& sslResult);

	void logHttpReq(const char *mesg, Transaction *tr, bool printBody = false);
	void logHttpResp(const char *mesg, Transaction *tr, bool printBody = false);

	void startSSL(Transaction *tr);
	void finishSSL(Transaction *tr, bool resultRecved = false);

	void changeState(ClientState _state);	

	void printRecentTransaction();
	
	/// Guide ������ ǥ�� ó���� �Ѵ�.
	/**
	 * ���� �� ó�� ��û�� �ƴ� ��� sisor �� ��� ��, �ȳ������� ǥ�� ���θ� �����Ѵ�.
	 * �ȳ��������� ǥ���ؾ� �� ��쿡��, �ܸ��⿡ �ȳ��������� �̵��ϵ��� redirect ���� ������
	 * �۽��ϰ� -1�� �����ϸ�, �ȳ��������� ǥ�� �� �ʿ� ���� ��� �ƹ��͵� ���� �ʰ� 0�� �����Ѵ�.
	 * 
	 * redirect �� �������� ���� �Ķ���ͷ� �Ѿ�� tr�� ���������� �������ϹǷ�, �ܺο��� tr ��
	 * ������ �ؼ��� �ȵȴ�.
	 * 
	 * @param tr ����� Ʈ����� ����
	 * @return 0 : �ȳ��������� ǥ���� �ʿ䰡 ���� ��� �����ؾ� �� �� 0�� �����Ѵ�.
	 * @return 1 : �ȳ��������� redirect�� �߰ų�, ������ �߻��ؼ� �۾��� �ߴ��ؾ� �� �� 1�� �����Ѵ�.
	 *
	 * @date 2007/02/23
	 * @author Sehoon Yang
	 **/
	int procGuide(Transaction* tr);

	/// �ȳ������� ǥ�� ó�� (��� ���񽺿�)
	/**
	 * �ȳ��������� ǥ���ؾ� ���� ���ƾ� ������ �Ǵ��Ͽ�
	 * ǥ���� �ʿ䰡 ������, �ܸ�����ڸ� �ȳ��������� redirect ��Ų��.
	 * 
	 * @return �ȳ��������� redirect �ƴٸ� 1, �ƴϸ� 0
	 *
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	int procShowGuidePage(Transaction* tr);

	/// �ȳ������� ǥ�� ó�� (Fimm ���� ����)
	/**
	* Fimm�� ����ϴ� �ܸ����� ���� �ܸ����, ��Ÿ�� �ܸ���� ����� �޶�
	* ������ ó���� �ʿ��ϴ�.
	*
	* �ȳ��������� ǥ���ؾ� ���� ���ƾ� ������ �Ǵ��Ͽ� ǥ���� �ʿ䰡 �ִٸ�,
	* �ܸ�����ڸ� �ȳ��������� redirect ��Ų��.
	* �ȳ��������� ǥ���� �ʿ䰡 ���ٸ�, ��û URL ���� reqURL �ʵ尪�� �̿��ؼ�
	* �ش� URL�� redirect ��Ų��.
	* 
	* @return �ȳ��������� ������ URL�� redirect �ƴٸ� 1, �ƴϸ� 0
	*
	* @date 2007/02/25
	* @author Sehoon Yang
	**/
	int procFimmSpecificShowGuidePage(Transaction* tr);

	/// ��û URL �� �ڿ� Query �� �־��� reqURL �ּҷ� redirect ��Ų��.
	/**
	 * ��û URL �� "http://ftpkunproxy.magicn.com:9090/?reqURL=http://www.magicn.com" �� ����
	 * reqURL �̶�� ���� ���� �� �ش� URL �� redirect ��Ų��.
	 * 
	 * @return redirect ������ 1, �ƴϸ� 0
	 *
	 * @date 2007/03/02
	 * @author Sehoon Yang
	 **/
	int redirectToReqURL( Transaction* tr );

	/// �ȳ��������� ��� Skip �ϵ��� ���� ���� ó��
	/**
	 * �ȳ��������� ������ ��� Skip �ϵ��� ����ڰ� ��û�ߴ����� �Ǵ��Ͽ�
	 * ������ ��� Skip �ϵ��� �����ߴٸ�, �� ����� sisor�� �˸���, �ܸ� ����ڴ�
	 * �ش� ������ ���� �������� redirect ��Ų��.
	 * 
	 * @return Skip �ϵ��� �����ϰ� redirect ���״ٸ� 1, �ƴϸ� 0
	 *
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	int procSetSkipGuide(Transaction* tr);

	/// �ش� URL�� Fimm ���� �ȳ������� ǥ�� URL ���� �Ǵ��Ѵ�.
	/**
	 * Fimm ���񽺸� �̿��ϴ� �ܸ��� ��, Ư��(����) �ܸ����� ��� �ȳ������� 
	 * ǥ�� �����Ǵ��� ������ �ϴ� �ܸ��� ����� �ٸ���.
	 *
	 * Fimm ���� �ȳ������� ��û�� ��� ������ ���� URL �� ���ŵȴ�.
	 * "http://ktfkunproxy.magicn.com:9090/?reqURL=fimm.co.kr"
	 * 
	 * @param url �Ǵ� ������ �Ǵ� URL
	 * @return Fimm ������ ��� GCODE_Fimm�� �����ϰ�, �� �� ���� URL�� ��� GCODE_Unknown�� �����Ѵ�.
	 *
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	GuideCode getFimmSpecificGuideCode(const url_t& url);

	/// ��û URL �� �ڿ� Query �� �־��� reqURL �ּҸ� �о� �´�.
	/**
	* ��û URL �� "http://ktfkunproxy.magicn.com:9090/?reqURL=http://www.magicn.com" �� ����
	* reqURL �̶�� ���� ���� �� reqURL ���� �о� �´�
	* 
	* @return reqURL ������ �� ���� ���õǰ�, ������ empty �̴�.
	*
	* @date 2007/03/02
	* @author Sehoon Yang
	**/
	url_t getReqURL(const url_t& url);
	
	/// �ȳ� �������� redirect
	/**
	 * �Է¹��� GuideCode �� �ش��ϴ� �ȳ��������� �ܸ��� redirect �ϵ��� 
	 * �ϴ� �޽����� �۽��Ѵ�.
	 * 
	 * @param tr Transaction
	 * @param gcode redirect �� �ȳ�������
	 * @return redirect�� ���� ��� 0, redirect�� ���� ���� ��� -1
	 *
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	int redirectToGuidePage(Transaction *tr, const GuideCode gcode);
	
	/// Fimm ���� �ȳ� �������� redirect
	/**
	 * Fimm�� ���� �ȳ� �������� ������ �ִ� ���� �ƴϰ�,
	 * Fimm ���� ���� �ܸ��� ����, redirect �ּ� �ڿ� "?FromPAS=Y" ��� �߰�
	 * Query �� �����δ�.
	 * 
	 * @param tr Transaction
	 * @return redirect�� ���� ��� 0, redirect�� ���� ���� ��� -1
	 *
	 * @date 2007/02/28
	 * @author Sehoon Yang
	 **/
	int redirectToFimmSpecificGuidePage(Transaction* tr);

	/// ���񽺺� SkipGuide ���� ��� �������� redirect
	/**
	* �Է¹��� GuideCode �� �ش��ϴ� SkipGuide ���� ��� �������� �ܸ���
	* redirect ��Ų��.
	* 
	* @param tr Transaction
	* @param gcode �����ڵ�
	* @return �׻� 0
	*
	* @date 2007/02/25
	* @author Sehoon Yang
	**/
	int redirectToGuideResultPage(Transaction *tr, const GuideCode gcode);

	/// ���񽺺� SkipGuide ���� ��� ������ URL ������
	/**
	 * @param gcode �����ڵ�
	 * @param reqURL ����ڰ� ��û�� URL
	 * @return redirect �ؾ��� URL
	 *
	 * @date 2007/02/28
	 * @author Sehoon Yang
	 * 
	 * @see redirectToGuideResultPage
	 **/
	url_t getGuideResultURL( const GuideCode gcode, const url_t& reqURL );

	/// �ȳ������� ǥ�� ���θ� �˾ƿ´�.
	/**
	 * Sisor ���� ���Ǹ� �ؼ�, �ȳ��信���� ǥ�� ���θ� �˾ƿ´�.
	 *
	 * Exception : Sisor�� ��Ű����� ������ ���� ��� exception�� �߻��Ѵ�.
	 * 
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	GuideReadResponse getSkipGuide(Transaction* tr, const MDN& mdn, const url_t& reqURL);

	/// �ȳ��������� ��� Skip �ϵ��� �����Ѵ�.
	/**
	 * �ȳ��������� ������ ��� Skip �ϵ��� Sisor���� �˸���.
	 *
	 * Exception : Sisor�� ��Ű����� ������ ���� ��� exception�� �߻��Ѵ�.
	 *
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	bool setSkipGuide(Transaction* tr, const MDN& mdn, const GuideCode& gcode);

	/// ��û URL�� ���� � ���񽺿� ���� Skip Flag ���� ��û���� �Ǵ��Ѵ�.
	/**
	 * @param reqURL ��û URL
	 * @return Ư�� ���񽺿� ���� Skip ���� ��û�� ��� �ش� ���� �ڵ带 ��ȯ�Ѵ�.
	 * @return Skip ���� ��û�� �ƴϰų�, �� �� ���� ��û�� ��� GCODE_Unknown �� ��ȯ�Ѵ�.
	 *
	 * @date 2007/02/25
	 * @author Sehoon Yang
	 **/
	GuideCode getSkipGuideCodeFromURL(const url_t& reqURL);

	/// ��û�� �������� Forbidden ���� �ܸ�����ڿ��� �۽��Ѵ�.
	/**
	 * ��û�� ������ Forbidden ���� �˸��� http response�� �ܸ� ����ڿ��� �۽��Ѵ�.
	 * 
	 * @date 2007/02/26
	 * @author Sehoon Yang
	 **/
	void sendHTTPForbidden(Transaction* tr);

	/// ���������� update �Ѵ�.
	/**
	 * ���������� Sisor ���� update �ϵ���, ���������� Sisor�� �۽��Ѵ�.
	 * ���������� ��ȭ������ Sisor���� �˸���, Sisor �� ���������Ϳ� ������ ��Ų��.
	 * 
	 * @date 2007/02/26
	 * @author Sehoon Yang
	 **/
	void updateSessionInfo();

	/// redirect �ؾ��� �ȳ������� URL �� ��� �´�.
	/**
	* �ȳ��������� redirect �� �� URL �ڿ� ����ڰ� ���� ������ �ߴ� URL��
	* �߰��� ��� �Ѵ�. 
	* ��, �ȳ��������� http://magicn.com/guide.asp �̰�, ����ڰ� ��û�� ��������
	* http://magicn.com/onlyadult.asp ��� �ϸ�ٸ�,
	* http://magicn.com/guide.asp?reqURL=http://magicn.com/onlyadult.asp �� ���� 
	* URL�� �����ؾ� �Ѵ�.
	* 
	* @param gcode �����ڵ�
	* @param reqURL ����ڰ� ��û�� URL
	* @return �ȳ������� URL
	*
	* @date 2007/02/28
	* @author Sehoon Yang
	**/
	url_t getGuideURL( const GuideCode gcode, const url_t& reqURL );

	/// URL �� query �� �߰��Ѵ�.
	/**
	 * URL := http://host/path?query
	 * 
	 * @param srcURL ���� URL
	 * @param key �߰��� query �� key
	 * @param value �߰��� key �� value
	 * @return query �� �߰��� URL
	 *
	 * @date 2007/02/28
	 * @author Sehoon Yang
	 **/
	url_t addQueryToURL(const url_t& srcURL, const char* key, const char* value);
	
	bool procService(Transaction* tr);

	bool addCorrelationKey(HTTP::RequestHeader& destHeader, const CorrelationKey& key);
	bool addCorrelationKey(HTTP::ResponseHeader& destHeader, const CorrelationKey& key);

	/// �ȳ������� ���� �α� ���
	/**
	 * @exception No throw 
	 *
	 * @date 2007/06/05 
	 * @author SeHoon Yang
	 **/
	void writeGuideCommunicationLog(const ACE_Time_Value& duration, const GuideCode& gcode) const;

	/// ������ ���� �α� ���
	/**
	 * @exception No throw 
	 *
	 * @date 2007/06/05 
	 * @author SeHoon Yang
	 **/
	void writeAbnormalCloseLog();
	
	

	//-------------------------------------------------------
	// ��� ����
	//-------------------------------------------------------
	
	bool everRecvPacket;
	
	ReactorInfo* myReactor;
	
	// currTransaction �� transactionQueue �� ��� �����Ƿ� ������ ������ �ʿ� ����.
	Transaction* currTransaction;

	// currHttpRequest�� transaction�� ��� �����Ƿ� ������ ������ �ʿ� ����.
	HTTP::Request* currHttpRequest;	///< Client�� ���� ������ Request ����
	
	TransactionQueue transactionQueue;

	ClientState state;
	//CpList cpList;					///< ClientHandler:1 <---> N:CPEventHandler �� ����� CP ����Ʈ
	//CpHandler _cp;
	CpHandler* _pCP;

	SantaHandler santaHandler;
	AuthHandler authHandler;

	PasDataLog* paslog;
	bool isTrace; // phoneTrace �� ������ ��ȭ��ȣ�̸�  true 
	MyLog* httplog;
	FileDump* filedump;
	bool closeFuncExecuted;
	int _sentBodySizeByStream; // ��Ʈ���� ������� ������ body ������
};

#endif
