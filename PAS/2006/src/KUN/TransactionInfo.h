#ifndef TRANSACTIONINFO_H
#define	TRANSACTIONINFO_H



#include <list>

#include "Common.h"

#include "HttpRequest.h"
#include "HttpResponse.h"

#include "MyLog.h"
#include "ActiveObjectChecker.h"
#include "CorrelationKey.h"



#define RESCODE_OK 200

#define RESCODE_URL_INVALID 400
#define RESCODE_SANTA 421
#define RESCODE_AUTH 422

#define RESCODE_CP_CONN 431
#define RESCODE_CP_TIMEOUT 408

#define RESCODE_STATFILTER 299
#define RESCODE_MULTIPROXY 399
#define RESCODE_WRONG_PROXY 488
#define RESCODE_WRONG_BROWSER 489


#define	HEADERNAME_LEN 32

enum SpeedUpTagStatus
{
	SUTS_NEED_NOT = 0,
	SUTS_BIG_CONTENT = 3,
	SUTS_POOL_REMOVE_PHONE = 11,
	SUTS_POOL_INSERT_PHONE = 12,
	SUTS_NON_POOL_REMOVE_VENDER = 13,
	SUTS_NON_POOL_INSERT_VENDER = 14
};



/// Transaction ���� ����
/**
@author DMS

�ϳ��� Transaction�� ���� ������ ��� �����ϰ� �ִ� Ŭ����
Transaction�̶� ����, ��û/���� �� �ѽ��� Transaction�̶� ����.

�ѹ��� Transaction�� ó���ϱ���� ���� ������ �ʿ��ϸ�
�װ͵��� ����/������ �����ϰ� �ϱ� ���ؼ� ���� Ŭ�����̴�.

�Ʒ��� Transaction���� ����/���� �ϴ� �������̴�.

[���� ����]
Sequence Number
Request
Response
Thread ID (�� transaction �� ó���ϴ� thread �� ��ȣ)

[�ܸ����� ���� ����]
USER-AGENT
PHONE_NUMBER
MNC
MDN
MIN
IMSI
ChInfo
BaseID
MsModel
OrgURL
RealURL
COUNTER
CKEY
KTF-INIT-PATH

[CP ���� ����]
CP Name
SVC Code
HashKey
BillInfo

[�޽��� ��/���� ����]
Phone IP
Phone Port
CP Host Name
CP IP
CP Port
Transaction ���� �ð�
Transaction ���� �ð�
Phone Request Bytes
Phone Response Bytes
Phone Response Code

Phone Start Time
Phone End Tiem
Phone Response Time

CP�� ������ �ð�
CP Request Bytes
CP Response Bytes
CP Response Code

CP Start Time
CP End Tiem
CP Response Time

SSL ���� �ð�
SSL ���� �ð�
SSL Request Bytes
SSL Response Bytes
*/

class Transaction : public ActiveObjectChecker
{
public:
	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------
	Transaction();
	Transaction(const Transaction &old);
	virtual ~Transaction();

	bool operator < (const Transaction& right);
	bool operator ==  (const Transaction& right);

	void clear();
	void clearSizeAndTime();
	
	void setLog(MyLog *log);

	/// set id : Transaction id (seqNum)�� �����ϸ鼭  Request/Response �޽����� seqNum �� ���� �����Ѵ�.
	void id(int _id);

	/// get id
	int	id();

	HTTP::Request *getRequest();
	const HTTP::Request *getRequest() const;
	HTTP::Response *getResponse();
	const HTTP::Response *getResponse() const;
	
	void setErrorRespCode(int code);
	int	getErrorRespCode();
	void recvPhoneReq();
	void sendPhoneResp();
	void connectCp();
	void sendCpReq();
	void recvCpResp();
	void setCpTime();

	static void parseCpName(HTTP::HeaderElement::value_t &cpdata, const char *delimiter, char *_cpName, char *_svcCode);
	
	void setCpAddr();
	void setCpConnInfo_first();
	void setCpConnInfo_apply_ACL(const char *NameOrIp, int port);
	void setCpConnInfo_second();
	void parsePhoneNumberInfo();
	void setTransactionInfo();
	void setSantaResult(const char *santaMDN, const char *santaIMSI);
	void setSantaResult_New(const char *santaMDN, const char *santaIMSI);
	void setPhoneNumber(const char *tmp_phoneNumber);
	void setAnonymous();
	
	bool isDone();
	void setDone();
	
	void beginSSL();
	void endSSL();
	
	bool isSSL();

	void onSslDataRecv(int recvBytes);
	void onSslDataSend(int sendBytes);

	void parseCpName_New(const char *fullSourceStr);
	int	validatePhoneNumber(const char *http_phoneNumber, char *good_phoneNumber);
	void logStrangeNumber(const char *orgPhonenumber);
	
	/// ��Ʈ���� ��� ���� Ȯ��
	/**
	 * @return ��Ʈ���� ��� ����
	 * @exception no throw
	 *
	 * @date 2007/03/26
	 * @author Sehoon Yang
	 *
	 * ��Ʈ�����̶� CP�� ���� ���� ���� ������ PAS�� �������� �ʰ�, ���� ��� �ڵ������� �۽��ϴ� ����̴�.
	 * �������̳� ��뷮 ������ ���� ����ȭ�� �ϱ� �����̴�.
	 **/
	bool streaming() const;

	/// ��Ʈ���� ��� ���� ����
	/**
	 * @param stream ��Ʈ���� ��� ����
	 * @exception no throw
	 *
	 * @date 2007/03/26
	 * @author Sehoon Yang
	 **/
	void streaming(bool stream);

	/// ����Ʈ�� �ױ׸� �����ؾ� �ϴ°�?
	/**
	 * CP�� ���� ���� ���� �����Ϳ� ���ǵ�� �ױ׸� �����ؾ� �Ѵٸ� true �� �����ϰ�,
	 * �ƴ϶�� false �� �����Ѵ�.
	 *
	 * ���� �Ǵ� ������ Phone �� IP�� Lucent �뿪�� ��� SPEEDUP_REMOVE_PHONE �� �ƴҰ�� �߰��ϸ�,
	 * Lucent �뿪�� �ƴ� ��� SPEEDUP_REMOVE_VENDOR �� �ƴ� ��� �߰��Ѵ�.
	 *
	 * @return true �̸� �����ؾ��ϰ�, false �̸� �������� �ʴ´�.
	 *
	 * @exception No throw 
	 *
	 * @date 2007/06/01 
	 * @author SeHoon Yang
	 **/
	bool isNeedInsertSpeedupTag() const;

	/// ����Ʈ�� �ױ׸� �����ؾ� �ϴ°�?
	/**
	* CP�� ���� ���� ���� �����Ϳ� ���ǵ�� �ױ׸� �����ؾ� �Ѵٸ� true �� �����ϰ�,
	* �ƴ϶�� false �� �����Ѵ�.
	*
	* ���� �Ǵ� ������ Phone �� IP�� Lucent �뿪�� ��� SPEEDUP_REMOVE_PHONE �̸� �����ϰ�,
	* Lucent �뿪�� �ƴ� ��� SPEEDUP_REMOVE_VENDOR �̸� �����Ѵ�.
	*
	* @return true �̸� �����ؾ��ϰ�, false �̸� �������� �ʴ´�.
	*
	* @exception No throw 
	*
	* @date 2007/06/01 
	* @author SeHoon Yang
	**/
	bool isNeedRemoveSpeedupTag() const;

	SpeedUpTagStatus getSpeedupStatus() const;

	

	//-------------------------------------------------------
	// ��� ����
	//-------------------------------------------------------

	int	threadId; // �� transaction �� ó���ϴ� thread �� ��ȣ.
	bool is3G;

	bool connCloseRequested;  // �ܸ� ��û ����� Connection: close �� ���Ե� ���.

	bool hotNumberConverted;
	int	sockfd;
	int	errorRespCode;

	char userAgentLine[LEN_USER_AGENT+1];
	char phoneNumber[LEN_PHONE_NUM+1];
	char MNC[LEN_MNC+1];
	char MDN[LEN_MDN+1];
	char MINnumber[LEN_MDN+1];
	char IMSI[LEN_IMSI+1];
	char chInfo[LEN_CH_INFO+1];
	char baseId[LEN_BASE_ID+1];
	char msModel[LEN_MS_MODEL+1];
	char browser[LEN_BROWSER+1];
	char orgUrl[MAX_URL_LEN+1];
	char realUrl[MAX_URL_LEN+1];
	char counterStr[LEN_COUNTER+1];
	int	counter;
	char cKey[LEN_CKEY+1];
	char ktfInitPath[LEN_KTF_INIT_PATH+1];

	// CP response �޽������� ��� ����
	char cpName[LEN_CP_NAME+1];
	char svcCode[LEN_SVC_CODE+1];

	char hashKey[LEN_HASH_KEY+1];
	char billInfo[LEN_BILL_INFO+1];

	// �޽��� �ۼ��� ���� ����
	char phoneIpAddr[LEN_IPADDR+1];		// �ܸ����� IP �ּ� ���ڿ�(123.123.123.123)
	unsigned int nIPAddr;				// �ܸ����� IP �ּ� unsigned int ��
	int	phonePort;
	char cpHostName[LEN_HOST_NAME+1];
	char cpIpAddr[LEN_IPADDR+1];
	int	cpPort;

	unsigned int trStartSec; // sec from epoch. time() ���� ���ϴ� ��.
	int	phoneReqBytes;
	int	phoneRespBytes;
	int	phoneRespCode;

	time_t phoneStartSec;
	time_t phoneEndSec;		
	int	phoneRespSec;

	time_t callstarttime; // OLD PAS �ҽ��� user ������ �̸� �״��. CP connect �ð�.
	
	int	cpReqBytes;
	int	cpRespBytes;
	int	cpRespCode;

	// santa time
	ACE_Time_Value santaStartTime;
	ACE_Time_Value santaEndTime;

	// guide time
	ACE_Time_Value guideStartTime;
	ACE_Time_Value guideEndTime;

	// cp time
	ACE_Time_Value cpStartTime;
	ACE_Time_Value cpEndTime;
	ACE_Time_Value cpRespTime;
	double cpRespMicrosec;

	// total(phone) time
	ACE_Time_Value phoneStartTime;
	ACE_Time_Value phoneEndTime;
	
	time_t cpStartSec;
	time_t cpEndSec;
	int	cpRespSec;
	
	int	sslReqBytes;
	int	sslRespBytes;

	time_t sslStartTm;
	time_t sslEndTm;

	char headerNameForPhoneNum[HEADERNAME_LEN+1]; // 2006.12.8

	CorrelationKey correlationKey;

private:

	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------

	/// ����Ʈ�� �ױ� ó�� ����
	/**
	* ����Ʈ�� �ױ׸� �����ϰų� ������ �ؾ� �Ѵٸ� true �� �����ϰ�, �ƴϸ� false �� �����Ѵ�.
	*
	* �Ǵ� ������, ���� �������� PAS�� MEó���� PAS���� ���ο� Config �� �������� CP�� ���� ���� ����
	* ������ ���İ� ������, ���� �ڵ�, �׸��� Phone ���� ������ Ȯ���ؼ� �����ȴ�.
	*
	* @return true �̸� ó���ϰ�, false �̸� ó������ �ʴ´�.
	*
	* @exception No throw 
	*
	* @date 2007/06/01 
	* @author SeHoon Yang
	**/
	bool isNeedHandleSpeedupTag() const;

	bool isSpeedupRemovePhone() const;
	bool isLucentPool() const;
	bool isSpeedupVender() const;
	bool isSpeedupRemoveVender() const;
	

	//-------------------------------------------------------
	// ��� ����
	//-------------------------------------------------------
	bool jobDone;
	int seqNum;
	bool _isSSL;
	bool _streaming; ///< ��Ʈ���� ��� ����

	HTTP::Request request;
	HTTP::Response response;
	bool aclApplied;
	bool hotNumberApplied;

	MyLog *tracelog;
};



//typedef MutexQueue<Transaction*> TransactionQueue;
typedef NullmutexQueue<Transaction*> TransactionQueue;

#endif
