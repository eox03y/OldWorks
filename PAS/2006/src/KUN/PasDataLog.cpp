#include "PasDataLog.h"
#include "UserInfoMng.h"

char	PasDataLog::pasIpAddr[32];

PasDataLog *PasDataLog::oneInstance =NULL;

PasDataLog *PasDataLog::instance()
{
	if (oneInstance == NULL)
	{
		oneInstance = new PasDataLog();
		oneInstance->openFiles();
	}
	return oneInstance;
}

PasDataLog::PasDataLog()
{
	// time
	lastSec = lastMinute = lastHour = lastDay = 0;
}

PasDataLog::~PasDataLog()
{
}
	
/**
lineTime == YYYYMMDDhhmm ������ �ð��� ���ڿ�. �α� ������ �̸� ������ �� ��.

fnameTime == YYYY/MM/DD hh:mm:ss ������ �ð��� ���ڿ�. �α� ������ �� ������ �� �κп� �� ��.
*/
void PasDataLog::openFiles(bool reopenFlag /*= false */)
{
	time_t	t_val;
	time(&t_val);

	if (t_val == lastSec) return;
	lastSec = t_val;
	localtime_r(&t_val, &t);
	snprintf(lineTime, sizeof(lineTime)-1,  "%04d/%02d/%02d %02d:%02d:%02d",
		t.tm_year+1900, t.tm_mon+1,
		t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

	// 1�� ���� ���� ����
	if (reopenFlag || lastDay != t.tm_mday || myFilePas.empty() || myFilePasStat.empty())
	{
		openPas();
		openPasStat();
		lastDay = t.tm_mday;
	}

	// 1�ð� ���� ���� ����
	if (reopenFlag || lastHour != t.tm_hour || myFileOver10.empty() || myFileDuration.empty())
	{
		openOver10();
		openDuration();
		lastHour = t.tm_hour;
	}

	// 1�� ���� ���� ����
	if (reopenFlag || lastMinute != t.tm_min || myFilePasIdr.empty() || myFilePasIss.empty())
	{
		openPasIdr();
		openPasIss();
		lastMinute = t.tm_min;
	}
}

/**

flush�� �󸶳� ���� �Ұ��ΰ�/

1. �Ź� ?  -- �ʹ� ����
2. 1�� ���� ?
3. over10 �� �ǽð� flush �ʿ�.
*/

void PasDataLog::startOfTransaction(UserInfo *user, Session *sess, Transaction *tr)
{
	#ifdef BENCH_TEST
	return;
	#endif
	
	openFiles();
	if (sess->isSSL())
		writeSslPasRequest(user, sess, tr);
	else
		writePasRequest(user, sess, tr);
}

void PasDataLog::endOfTransaction(UserInfo *user, Session *sess, Transaction *tr)
{
	#ifdef BENCH_TEST
	return;
	#endif
	
	openFiles();

	// ���� �α� ���
	if (sess->isSSL())
		writeSslConnectResult(user, sess, tr);
	else
		writePasResponse(user, sess, tr);

	// ���α�
	writePasStat(user, sess, tr);

	// ���ݷα�
	writePasIdr(user, sess, tr);

	// �����α�
	writeOver10(user, sess, tr);

	// �������α�
	writePasIss(user, sess, tr);

	// �ð��α�
	writeDuration(user, sess, tr);
}

/** ���� �α� - ./k_pas.MMDD.log - 1 �� ���� */
void PasDataLog::openPas()
{
	char fname[64];

	snprintf(fname, sizeof(fname)-1, "%spas.%02d%02d.log", Config::instance()->getLogPrefix().c_str(), t.tm_mon+1, t.tm_mday);
	
	// default open parameter is O_CREAT|O_APPEND|OWRONLY
	int ret = myFilePas.open( fname );
	if( ret == -1 )
		printErr( fname, errno );
}

/** ��� �α� - ./k_passtat.MMDD.log  - 1 �� ���� */
void PasDataLog::openPasStat()
{
	char fname[64];

	snprintf(fname, sizeof(fname)-1, "%spasstat.%02d%02d.log", Config::instance()->getLogPrefix().c_str(),  t.tm_mon+1, t.tm_mday);

	int ret = myFilePasStat.open( fname );
	if( ret == -1 )
		printErr(fname,  errno );
}

void PasDataLog::printErr(char *fname, int err)
{
	char *sysErrMsg = NULL;
	sysErrMsg = strerror( err );

	PAS_WARNING2("file open error - %s: %s", fname, sysErrMsg);
}

/** ���� ���� �α� - bill/k_n_pasidr.YYYYMMDDhhmm.log  - 10 �� ���� */
void PasDataLog::openPasIdr()
{
	char	fname[64];
	char	timeval[32];
	int	minute=0;

	minute = ((int)(t.tm_min / 10)) * 10;
	snprintf(timeval, sizeof(timeval)-1,  "%04d%02d%02d%02d%02d",
		t.tm_year+1900, t.tm_mon+1,
		t.tm_mday, t.tm_hour, minute);

	SmallString prefix = Config::instance()->getLogPrefix();
	
	// ���ݷα״� ME�� ��� prefix �� "a_" �̴�.
	// ME ������ ���ݷα׸� ������ �ٸ� �α��� prefix �� ������, ���ݷα׸� �������δ�.
	// (�̷� ���ܴ� ���� ¯����. >_<)
	if(Config::instance()->process.serverID.incaseFind("ME") >= 0)
		prefix = "a_";

	snprintf(fname, sizeof(fname)-1, "bill/%sn_pasidr.%s.log", prefix.c_str(), timeval);
	int ret = myFilePasIdr.open( fname );
	if( ret == -1 )
		printErr(fname,  errno );
}

/** ����(over10) �α� - bill/k_over10.YYYYMMDDhh.log  - 1 �ð� ���� */
void PasDataLog::openOver10()
{
	char	fname[64];
	char	timeval[32];

	snprintf(timeval, sizeof(timeval)-1, "%04d%02d%02d%02d",
		t.tm_year+1900, t.tm_mon+1,
		t.tm_mday, t.tm_hour);
	snprintf(fname, sizeof(fname)-1,  "bill/%sover10.%s.log", Config::instance()->getLogPrefix().c_str(), timeval);
	int ret = myFileOver10.open( fname );
	if( ret == -1 )
		printErr(fname,  errno );
}

/** ���� ��� �α� - cmslog/k_pasiss.YYYYMMDDhhmm.log  - 5 �� ���� */
void PasDataLog::openPasIss()
{
	char	fname[64];
	char	timeval[32];
	int	minute=0;

	minute = ((int)(t.tm_min / 5)) * 5;
	snprintf(timeval, sizeof(timeval)-1, "%04d%02d%02d%02d%02d",
		t.tm_year+1900, t.tm_mon+1,
		t.tm_mday, t.tm_hour, minute);
	snprintf(fname, sizeof(fname)-1,  "cmslog/%spasiss.%s.log", Config::instance()->getLogPrefix().c_str(), timeval);

	int ret = myFilePasIss.open( fname );
	if( ret == -1 )
		printErr(fname,  errno );
}

/**
SSL CONNECT ���Ž� �α�
*/
void	PasDataLog::writeSslPasRequest(UserInfo* /* user */, Session *sess, Transaction *tr)
{
	const HTTP::RequestHeader *reqHeader = tr->getRequest()->getHeader();
	const ProcessConfig& procConf = Config::instance()->process;

	myFilePas.print( "%s [SSL] %s --> %d %s %s %s, %d bytes, %s/%d ssock:%d, %s, (CKEY:%s)\n",
		lineTime,
		sess->phoneNumber,
		UserInfoMng::instance()->getCount(),
		reqHeader->getMethodStr().toStr(), reqHeader->getOrgUrl().toStr(), reqHeader->getVersion().toStr(),  // HTTP Req
		tr->phoneReqBytes,
		sess->ipAddr, sess->port, sess->sockfd, procConf.serverID.toStr(),
		tr->cKey
		);
}

/**
SSL CONNECT ���� �۽Ž� �α�
*/
void	PasDataLog::writeSslConnectResult(UserInfo* /* user */, Session *sess, Transaction *tr)
{
	const HTTP::ResponseHeader *respHeader = tr->getResponse()->getHeader();
	const ProcessConfig& procConf = Config::instance()->process;
	
	myFilePas.print( "%s [SSL] %s <-- %d %s %d %s, %d bytes, %s/%d ssock:%d, %s, (CKEY:%s)\n",
		lineTime,
		sess->phoneNumber,
		UserInfoMng::instance()->getCount(),
		respHeader->getVersion().toStr(), respHeader->getStatusCode(), respHeader->getStatusString().toStr(),
		tr->getResponse()->getHeadLeng(),
		sess->ipAddr, sess->port, sess->sockfd, procConf.serverID.toStr(),
		(tr) ?  tr->cKey : "0"
		);
}

/** ���� �α� - ./k_pas.MMDD.log - 1 �� ���� */
void	PasDataLog::writeSslPasResponse(UserInfo* /* user */, Session *sess, Transaction *tr, bool resultRecved /* = false */)
{
	int reqBytes = 0;
	char method[32];
	
	if (resultRecved) 
	{
		const HTTP::RequestHeader *reqHeader = tr->getRequest()->getHeader();
		
		reqBytes = tr->phoneReqBytes;
		STRNCPY(method, reqHeader->getMethodStr(), 16);
	}
	else
	{
		strcpy(method, "FINISH");
	}
	
	//int res =
	myFilePas.print( "%s [SSL] %s --> %d %s  (size:%d bytes, thr:%d, sock:%d) (cpname=%s;svccode=%s)\n",
		lineTime,
		sess->phoneNumber,
		UserInfoMng::instance()->getCount(),
		method,
		reqBytes,
		sess->threadId,
		sess->sockfd,
		sess->cpName,
		sess->svcCode);
}

/** ���� �α� - ./k_pas.MMDD.log - 1 �� ���� */
void	PasDataLog::writeSslDataReq(UserInfo* /* user */, Session *sess, int currBytes)
{
	//int res =
	myFilePas.print( "%s [SSL] %s --> %s:%d  (size:%d bytes, total:%d bytes,  %s:%d, thr:%d, sock:%d) (cpname=%s;svccode=%s)\n",
		lineTime,
		sess->phoneNumber,
		sess->sslHost.toStr(),
		sess->sslPort,
		currBytes,
		sess->sslReqBytes,
		sess->sslHost.toStr(),
		sess->sslPort,
		sess->threadId,
		sess->sockfd,
		sess->cpName,
		sess->svcCode);
}

/** ���� �α� - ./k_pas.MMDD.log - 1 �� ���� */
void	PasDataLog::writeSslDataResp(UserInfo* /* user */, Session *sess, int currBytes)
{
	//int res =
	myFilePas.print( "%s [SSL] %s <-- %s:%d  (size:%d bytes, total:%d bytes,  thr:%d, sock:%d) (cpname=%s;svccode=%s)\n",
		lineTime,
		sess->phoneNumber,
		sess->sslHost.toStr(),
		sess->sslPort,
		currBytes,
		sess->sslRespBytes,
		sess->threadId,
		sess->sockfd,
		sess->cpName,
		sess->svcCode);
}


/** ���� �α� - ./k_pas.MMDD.log - 1 �� ���� */
void	PasDataLog::writePasRequest(UserInfo* /* user */, Session* /* sess */, Transaction *tr)
{
	const HTTP::RequestHeader *reqHeader = tr->getRequest()->getHeader();
	const ProcessConfig& procConf = Config::instance()->process;

	//int res =
	myFilePas.print( "%s %s --> %d %s %s %s, %d bytes, %s/%d ssock:%d, %s, (CKEY:%s)\n",
		lineTime,
		tr->phoneNumber,
		UserInfoMng::instance()->getCount(),
		reqHeader->getMethodStr().toStr(), reqHeader->getOrgUrl().toStr(), reqHeader->getVersion().toStr(),  // HTTP Req
		tr->phoneReqBytes,
		tr->phoneIpAddr, tr->phonePort, tr->sockfd, procConf.serverID.toStr(),
		tr->cKey
		);
}

/** ���� �α� - ./k_pas.MMDD.log - 1 �� ���� */
void	PasDataLog::writePasResponse(UserInfo *user, Session *sess, Transaction *tr)
{
	const HTTP::ResponseHeader *respHeader = tr->getResponse()->getHeader();

	int reqCount = 0;
	if (user)
		reqCount = user->numRequests;
	else
		reqCount = sess->getSeqNum();

	// 2007-04-03 �� ���� ���� �ۼ��ϱ�
	char	initPath[128];
	if( tr->counter == 1 )
	{
		// InitPath �� �� ù request ���� ������ req ����� ���Եȴ�.
		snprintf(initPath, sizeof(initPath), "(KTF-INIT-PATH:%s) ", tr->ktfInitPath );
	}
	else
	{
		initPath[0] = 0;
	}
	
	//int res =
	myFilePas.print( "%s %s <-- %d %s %d %s %s (size:%d bytes, thr:%d, sock:%d) (cpname=%s;svccode=%s)(RequestCount:%d) (RequestMIN:%s) (RequestMNC:%s) (ChannelInfo:%s) (Counter:%s) %s (CKEY:%s)\n",
		lineTime,
		tr->phoneNumber,
		UserInfoMng::instance()->getCount(),
		respHeader->getVersion().toStr(), respHeader->getStatusCode(), respHeader->getStatusString().toStr(), 
		tr->userAgentLine,
		tr->phoneRespBytes,
		tr->threadId,
		tr->sockfd,
		tr->cpName,
		tr->svcCode,
		reqCount, 
		tr->MINnumber,
		tr->MNC,
		tr->chInfo,
		tr->counterStr,
		initPath,
		tr->cKey);

}

/** ��� �α� - ./k_passtat.MMDD.log  - 1 �� ���� */
void	PasDataLog::writeSslPasStat(UserInfo* /* user */, Session *sess, Transaction *tr)
{
	// ȣ ó�� ���� �ð� �������� ���� -- 2006.10.24 -- �絿�� ����� ���� ����. - OLD PAS �� �ٸ�.
	char timestr[32];
	struct tm dateTM;
	localtime_r(&sess->sslEndTm, &dateTM);
	strftime( timestr, 30, "%Y/%m/%d %H:%M:%S", &dateTM);
	
	// 2007-04-03 �� ������ �� ���� ������ ����.
	char	cpnameNsvccode[128];

	if( sess->cpName[0] != '\0' )
	{
		snprintf(cpnameNsvccode, sizeof(cpnameNsvccode),  "(cpname=%s;svccode=%s)", sess->cpName, sess->svcCode);
	}

	else
	{
		snprintf(cpnameNsvccode, sizeof(cpnameNsvccode),  "(N/A)");
	}
	
	myFilePasStat.print( "%s %-11s %-15s %0.4f %4d %5d %5d %s %s %s 200 %s:%d\n",
		timestr,
		sess->phoneNumber, 
		sess->IMSI,  
		tr->cpRespMicrosec,  // CP req ~ resp �ҿ� �ð�.
		//sess->sslEndTm - sess->sslStartTm,
		tr->phoneEndSec - sess->getLastTransactionTime(),  // ���� transaction ���� ���� 
		sess->sslReqBytes, sess->sslRespBytes, 
		//sess->msModel, sess->browser
		tr->msModel, tr->browser,
		cpnameNsvccode,
		sess->sslHost.toStr(), sess->sslPort
		);
		
}


/** ��� �α� - ./k_passtat.MMDD.log  - 1 �� ���� */
void	PasDataLog::writePasStat(UserInfo* /* user */, Session *sess, Transaction *tr)
{
/*
���� �Ͻ�	������ ��¥/�ð�	2005-06-09 15:55
��ȭ��ȣ(MIN)	016xxxx, 018xxxx	162011138
IMSI��ȣ	�ܸ� �ĺ��� ���� IMSI ��ȣ	4.5E+14   -----> 3G �̸� "WCDMA"
ó�� �ð�	CP Web ���� ���� ó�� �ð� + PAS ���� ó�� �ð�	0.104
����ð�	�ܸ��� ���� ���� �ð�	4
��û Size	�ܸ��κ��� ��û ���� Size(parsed by pasgw)	407
���� Size	������ ������ ������	2400
�ܸ��� ����	�ܸ���𵨸�/�ܸ��� ��������	SPHX5900 MSMB13
CPName/ServiceCode	CP Code / Service Code	(cpname=ktf;svccode=ktfimg000)
HTTP ����	HTTP ���� �ڵ�	200
��û URL	��û�� URL	http://www.magicn.com/main.asp

Ex) 2005/06/09 15:55:11 01692087244 450001692087244 0.0533 5  407  2400 SPHX7700 MSMB13 (cpname=daum;svccode=daumportal000) 200 http://m.daum.net/
*/
	const HTTP::ResponseHeader *respHeader = tr->getResponse()->getHeader();

	char timestr[32];
	struct tm dateTM;
	localtime_r(&tr->phoneEndSec, &dateTM);
	// ȣ ó�� ���� �ð� �������� ���� -- 2006.10.24 -- �絿�� ����� ���� ����. - OLD PAS �� �ٸ�.
	strftime( timestr, 30, "%Y/%m/%d %H:%M:%S", &dateTM); 

	char imsi[LEN_IMSI+1];
	if (strncmp(tr->IMSI, "3G", 2)==0)
		strcpy(imsi, "WCDMA");
	else
		STRCPY(imsi, tr->IMSI, LEN_IMSI);

	// 2007-04-03 �� ������ �� ���� ������ ����.
	char	cpnameNsvccode[128];
	
	if( tr->cpName[0]  &&  tr->svcCode[0])
	{
		snprintf(cpnameNsvccode, sizeof(cpnameNsvccode),  "(cpname=%s;svccode=%s)", tr->cpName, tr->svcCode);
	}
	else if( tr->cpName[0] )
	{
		snprintf(cpnameNsvccode, sizeof(cpnameNsvccode),  "(cpname=%s)", tr->cpName);
	}
	else if( tr->svcCode[0])
	{
		snprintf(cpnameNsvccode, sizeof(cpnameNsvccode),  "(svccode=%s)", tr->svcCode);
	}
	else
	{
		snprintf(cpnameNsvccode, sizeof(cpnameNsvccode),  "(N/A)");
	}

	myFilePasStat.print( "%s %-11s %-15s %0.4f %4d %5d %5d %s %s %s %d %s\n",
		timestr,
		tr->phoneNumber, 
		imsi,  
		tr->cpRespMicrosec,  // CP req ~ resp �ҿ� �ð�.
		tr->phoneEndSec - sess->getLastTransactionTime(),  // ���� transaction ���� ���� 
		tr->phoneReqBytes, tr->phoneRespBytes, 
		tr->msModel, tr->browser,
		cpnameNsvccode,
		respHeader->getStatusCode(), tr->realUrl
		);

}



/** ���� ���� �α� - bill/k_n_pasidr.YYYYMMDDhhmm.log  - 10 �� ���� */
void	PasDataLog::writeSslPasIdr(UserInfo* /* user */, Session *sess, Transaction *tr)
{
/*
1	Record_length	���ڵ� ��ü����	Alpha Numeric	1	5
2	Service_type	�������� �ڵ�	Alpha Numeric	6	2	����('A'), ����('F'), �⺻���� 'A'
3	Calling_MDN	�������ȭ��ȣ(MDN)	Alpha Numeric	8	12
4	Connect_start_time	����� ���ӿ�û �ð�	Alpha Numeric	20	15
5	Page_duration	�������� �̵��ð�	Alpha Numeric	35	10
6	Packet_count	������Ʈ	Alpha Numeric	45	10
7		���ð�	Alpha Numeric	55	15
8	Call_phone_type	�ܸ����	Alpha Numeric	70	16
9	Call_browser_version	�ܸ��� ������ ����	Alpha Numeric	86	16
10	�ܸ� IP	������ IP	Alpha Numeric	102	16
11	Response_code	HTTP ����ڵ�	Alpha Numeric	118	7
12	G/W IP	PAS G/W L4 IP	Alpha Numeric	125	16
13	Call_start_time	Callstarttime	Alpha Numeric	141	15
14	Cp_IP	CP ���� IP	Alpha Numeric	156	16
15	Cp_port	CP ���� Port	Alpha Numeric	172	7
16	Cp_name	CP Name	Alpha Numeric	179	21
17	service_code	Service Code	Alpha Numeric	200	21	Header�� Setting�Ǵ� Service Code
18	URL	���� URL	Alpha Numeric	221	variable	��� url
19	Line Feed		Alpha Numeric	variable	1	ASCII Code 10 ( 0x0a )

*/
	char trEndTime[32];
	char callstart[32];
	char response_time[32];
	time_t last_tr_time = sess->getLastTransactionTime();
	struct tm dateTM;

	// ȣ ó�� ���� �ð� �������� ���� -- 2006.10.24 -- �絿�� ����� ���� ����. - OLD PAS �� �ٸ�.
	localtime_r(&tr->phoneEndSec, &dateTM);
	strftime( trEndTime, 30, "%Y%m%d%H%M%S", &dateTM);

	localtime_r(&tr->callstarttime, &dateTM);
	strftime( callstart, 30, "%Y%m%d%H%M%S", &dateTM); // CP connect ���� �ð�

	localtime_r(&tr->cpEndSec, &dateTM);
	strftime( response_time, 30, "%Y%m%d%H%M%S", &dateTM);

	// ���� �ҽ����� CP connect ������ �ð���.
	
	//int res =
	myFilePasIdr.print( "%04d A %-11s %14s %09d %09d %-14s %-15s %-15s %-15s %-6d %-15s %-14s %-15s %-6d %-20s %-20s %s\n",
		125+9+68+strlen(tr->realUrl),
		tr->phoneNumber, 			// 3	Calling_MDN	�������ȭ��ȣ(MDN)	Alpha Numeric	8	12
		
		trEndTime,					 // 4	Connect_start_time	����� ���ӿ�û �ð�	Alpha Numeric	20	15
		
		// ���� transaction �� ���� transaction �Ϸ� �ð����� ���� 
		(int)(tr->phoneEndSec -last_tr_time) ,	// 5	Page_duration	�������� �̵��ð�	Alpha Numeric	35	10

		sess->sslReqBytes + sess->sslRespBytes, 		// 6	Packet_count	������Ʈ	Alpha Numeric	45	10
		// CP ���� �ð� -- ���� �ҽ����� CP ��û �ð��� ���� �Ǿ� �ִ�.  (������ ����)
		callstart,					//7	���  �ð� Alpha Numeric	55	15
		
		tr->msModel,			//8	Call_phone_type	�ܸ����	Alpha Numeric	70	16
		tr->browser,			//9	Call_browser_version	�ܸ��� ������ ����	Alpha Numeric	86	16
		tr->phoneIpAddr,		//10	�ܸ� IP	������ IP	Alpha Numeric	102	16
		tr->errorRespCode,		//11	Response_code	HTTP ����ڵ�	Alpha Numeric	118	7
		pasIpAddr,			//12	G/W IP	PAS G/W L4 IP	Alpha Numeric	125	16
		
		callstart,			//13	Call_start_time	Callstarttime	Alpha Numeric	141	15
		tr->cpIpAddr,	//14	Cp_IP	CP ���� IP	Alpha Numeric	156	16
		tr->cpPort,	//15	Cp_port	CP ���� Port	Alpha Numeric	172	7
		tr->cpName,	//16	Cp_name	CP Name	Alpha Numeric	179	21
		tr->svcCode,	//17	service_code	Service Code	Alpha Numeric	200	21	Header�� Setting�Ǵ� Service Code
		tr->realUrl	//18	URL	���� URL	Alpha Numeric	221	variable	��� url
	);	 
}


/** ���� ���� �α� - bill/k_n_pasidr.YYYYMMDDhhmm.log  - 10 �� ���� */
void	PasDataLog::writePasIdr(UserInfo* /* user */, Session* sess, Transaction *tr)
{
/*
1	Record_length	���ڵ� ��ü����	Alpha Numeric	1	5
2	Service_type	�������� �ڵ�	Alpha Numeric	6	2	����('A'), ����('F'), �⺻���� 'A'
3	Calling_MDN	�������ȭ��ȣ(MDN)	Alpha Numeric	8	12
4	Connect_start_time	����� ���ӿ�û �ð�	Alpha Numeric	20	15
5	Page_duration	�������� �̵��ð�	Alpha Numeric	35	10
6	Packet_count	������Ʈ	Alpha Numeric	45	10
7		���ð�	Alpha Numeric	55	15
8	Call_phone_type	�ܸ����	Alpha Numeric	70	16
9	Call_browser_version	�ܸ��� ������ ����	Alpha Numeric	86	16
10	�ܸ� IP	������ IP	Alpha Numeric	102	16
11	Response_code	HTTP ����ڵ�	Alpha Numeric	118	7
12	G/W IP	PAS G/W L4 IP	Alpha Numeric	125	16
13	Call_start_time	Callstarttime	Alpha Numeric	141	15
14	Cp_IP	CP ���� IP	Alpha Numeric	156	16
15	Cp_port	CP ���� Port	Alpha Numeric	172	7
16	Cp_name	CP Name	Alpha Numeric	179	21
17	service_code	Service Code	Alpha Numeric	200	21	Header�� Setting�Ǵ� Service Code
18	URL	���� URL	Alpha Numeric	221	variable	��� url
19	Line Feed		Alpha Numeric	variable	1	ASCII Code 10 ( 0x0a )

*/
	char trEndTime[32];
	char callstart[32];
	char response_time[32];
	time_t last_tr_time = sess->getLastTransactionTime();
	struct tm dateTM;

	// ȣ ó�� ���� �ð� �������� ���� -- 2006.10.24 -- �絿�� ����� ���� ����. - OLD PAS �� �ٸ�.
	localtime_r(&tr->phoneEndSec, &dateTM);
	strftime( trEndTime, 30, "%Y%m%d%H%M%S", &dateTM);

	localtime_r(&tr->callstarttime, &dateTM);
	strftime( callstart, 30, "%Y%m%d%H%M%S", &dateTM); // CP connect ���� �ð�

	localtime_r(&tr->cpEndSec, &dateTM);
	strftime( response_time, 30, "%Y%m%d%H%M%S", &dateTM);

	// ���� �ҽ����� CP connect ������ �ð���.
	
	//int res =
	myFilePasIdr.print( "%04d A %-11s %14s %09d %09d %-14s %-15s %-15s %-15s %-6d %-15s %-14s %-15s %-6d %-20s %-20s %s\n",
		125+9+68+strlen(tr->realUrl),
//		220+strlen(tr->realUrl),	// 2006-11-28 ���ڵ� ���� ����(������, ���� oldpas�� ���ڵ� ���̸� �߸� ����ϹǷ� �״�� �д�)
		tr->phoneNumber, 			// 3	Calling_MDN	�������ȭ��ȣ(MDN)	Alpha Numeric	8	12
		
		trEndTime,					 // 4	Connect_start_time	����� ���ӿ�û �ð�	Alpha Numeric	20	15
		
		// ���� transaction �� ���� transaction �Ϸ� �ð����� ���� 
		(int)(tr->phoneEndSec -last_tr_time) ,	// 5	Page_duration	�������� �̵��ð�	Alpha Numeric	35	10
		
		tr->phoneReqBytes + tr->phoneRespBytes, 		// 6	Packet_count	������Ʈ	Alpha Numeric	45	10
		// CP ���� �ð� -- ���� �ҽ����� CP ��û �ð��� ���� �Ǿ� �ִ�.  (������ ����)
		callstart,					//7	���  �ð� Alpha Numeric	55	15
		
		tr->msModel,			//8	Call_phone_type	�ܸ����	Alpha Numeric	70	16
		tr->browser,			//9	Call_browser_version	�ܸ��� ������ ����	Alpha Numeric	86	16
		tr->phoneIpAddr,		//10	�ܸ� IP	������ IP	Alpha Numeric	102	16
		tr->errorRespCode,		//11	Response_code	HTTP ����ڵ�	Alpha Numeric	118	7
		pasIpAddr,			//12	G/W IP	PAS G/W L4 IP	Alpha Numeric	125	16
		
		callstart,			//13	Call_start_time	Callstarttime	Alpha Numeric	141	15
		tr->cpIpAddr,	//14	Cp_IP	CP ���� IP	Alpha Numeric	156	16
		tr->cpPort,	//15	Cp_port	CP ���� Port	Alpha Numeric	172	7
		tr->cpName,	//16	Cp_name	CP Name	Alpha Numeric	179	21
		tr->svcCode,	//17	service_code	Service Code	Alpha Numeric	200	21	Header�� Setting�Ǵ� Service Code
		tr->realUrl	//18	URL	���� URL	Alpha Numeric	221	variable	��� url
	);	 
}

// 5 ��
#define OVER10_CP_DELAY	(5)  

/** ����(over10) �α� - bill/k_over10.YYYYMMDDhh.log  - 1 �ð� ���� */
void	PasDataLog::writeOver10(UserInfo* /* user */, Session* /* sess */, Transaction *tr)
{
	if (tr->cpRespSec  >= 5  || tr->errorRespCode  >= 400 ||  tr->errorRespCode == 299)
	{
		//int res =

		myFileOver10.print( "%s %-11s %-14s %.4f %d %s\n",
			lineTime,
			tr->phoneNumber, tr->msModel, 
			tr->cpRespMicrosec,
			tr->errorRespCode,
			tr->realUrl  // HotNumber, ACL ���� ��ģ ��
			//tr->orgUrl  // ���� URL
			);
	}
}

/** ���� ��� �α� - cmslog/k_pasiss.YYYYMMDDhhmm.log  - 5 �� ���� */
void	PasDataLog::writePasIss(UserInfo* /* user */, Session* /* sess */, Transaction *tr)
{
/*
Data	���� ��¥	ex) 2004/01/08
Time	���� �ð�	ex) 16:53:17
MIN	���� ��ȭ��ȣ	ex) 01690109782	10~11�ڸ� ����
Process Time	ó���ð�	����: ��  / ex) 2.15	�������� ����
Process Byte	ó�� �����ͷ�	����: Byte  / ex) 5120	�������� ����
msModel �ܸ� �� 
cpname	CP ��ü �ڵ�	ex) k_dms	���� ���ڿ�
svccode	Service �ڵ�	ex) k_dmspers100	���� ���ڿ�
HTTP Status	HTTP ���� ��	ex) 200	100 ~ 999 ���ڰ�
HTTP channel info	�� ���� ��	Ex) B	���� ���ڿ�
URL	����URL	ex) http://www.magicn.com/a...	10~11�ڸ� ����

2004/08/05^15:54:09^01033380059^0.036685^2182^PGK2500^polycube^polycubetm820^200^B^http://tmktf.polycube.co.kr/MYIidealsel.asp?chk1=3&retURL=/MYMmain.asp&ilook=53&ipurp=1&ichrt=1&istyle=1
*/
	char cpname[LEN_CP_NAME+1];
	char svccode[LEN_SVC_CODE+1];

	if (tr->cpName[0]=='\0')
		strcpy(cpname, "(N/A)");
	else
		STRCPY(cpname, tr->cpName, LEN_CP_NAME);

	if (tr->svcCode[0]=='\0')
		strcpy(svccode, "(N/A)");
	else
		STRCPY(svccode, tr->svcCode, LEN_SVC_CODE);

	if ( tr->chInfo[0] == '\0' || tr->chInfo[0] == ' ' )
	{
		strcpy(tr->chInfo, "N/A");
	}

	// 2007-04-03 : ���α��� lineTime ���� ������ ���� �ʱ� ���� issLineTime �� ������ ���.
	char issLineTime[32];
	strcpy(issLineTime, lineTime);
	issLineTime[10] = '^';
	
	//int res =
	myFilePasIss.print( "%s^%s^%f^%d^%s^%s^%s^%d^%s^%s\n",
		issLineTime,
		tr->phoneNumber, 
		tr->cpRespMicrosec,
		tr->phoneReqBytes + tr->phoneRespBytes,
		tr->msModel, 
		cpname, 
		svccode,
		tr->errorRespCode, 
		tr->chInfo, 
		tr->realUrl);

}

void PasDataLog::openDuration()
{
	char fname[64];
	char timeval[32];

	snprintf(timeval, sizeof(timeval)-1, "%04d%02d%02d%02d",
		t.tm_year+1900, t.tm_mon+1,
		t.tm_mday, t.tm_hour);

	snprintf(fname, sizeof(fname)-1, "%sduration.%s.log", Config::instance()->getLogPrefix().c_str(), timeval);

	int ret = myFileDuration.open( fname );
	if( ret == -1 )
		printErr(fname,  errno );
}

void PasDataLog::writeDuration( UserInfo* /* user */, Session* /* sess */, Transaction *tr )
{
	// time MDN IMSI msmodel browser santa guide cp total TransactionStart COUNTER ResCode URL
	myFileDuration.print("%s %-11s %s %-10s %-10s %.4f %.4f %.4f %.4f %d.%06d %3s %d %s\n",
		lineTime,
		tr->phoneNumber, 
		tr->IMSI,
		tr->msModel, 
		tr->browser,
		getDiffTime(tr->santaStartTime, tr->santaEndTime),
		getDiffTime(tr->guideStartTime, tr->guideEndTime),
		getDiffTime(tr->cpStartTime, tr->cpEndTime),
		getDiffTime(tr->phoneStartTime, tr->phoneEndTime),
		tr->phoneStartTime.sec(), tr->phoneStartTime.usec(),
		tr->counterStr,
		tr->cpRespCode,
		tr->orgUrl);
}

double PasDataLog::getDiffTime( const ACE_Time_Value& start, const ACE_Time_Value& end )
{
	if(end < start)
	{
		PAS_INFO4("Time screw. StartTime[%d.%06d] EndTime[%d.%06d]", start.sec(), start.usec(), end.sec(), end.usec());
		return -1.0;
	}

	ACE_Time_Value diff = end - start;

	return (double)diff.sec() + (diff.usec() / 1000000.0);
}



