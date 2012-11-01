// -*- C++ -*-

/**
@file Session.h

@author DMS

@brief �� ����ں� ���� ���� ����

���� ������ �Ʒ��� ����.
��û Ƚ��
���� Ƚ��

�ܸ� IP
�ܸ� Port

�ܸ� ��ȣ
IMSI
MsModel
Browser
CP Name
SVC Code

���� ���� �ð�
������ Transaction �Ϸ� �ð�

SSL ���� ���� Sequence ��ȣ
SSL ȣ��Ʈ
SSL Port
SSL ���� �ð�
SSL ���� �ð�
SSL Request Bytes
SSL Response Bytes
*/


#ifndef SESSION_H
#define	SESSION_H

#include "Common.h"

#include "TransactionInfo.h"
#include "Util.h"
#include "MDN.h"
#include "CookieDB.h"

class Session
{
public:
	Session()
	{
		clear();
	}

	void clear();

	void start(const char *phone_ip, int phone_port, int sock);
	void setThreadId();
	void finish();
	
	void print();
	
	int	getSeqNum()
	{
		return reqNums;
	}

	int getRespNum()
	{
		return respNums;
	}

	bool isDone()
	{
		return reqNums == respNums;
	}
	
	int	beginTransaction(Transaction *tr);
	int	endTransaction(Transaction *tr);

	void copyTransaction(Transaction * tr);
	
	int	incSeqNum();
	
	bool isFirst()
	{
		return (reqNums == 1);
	}

	bool isSSL()
	{
		return is_SSL;
	}
	
	void beginSSL();
	void endSSL();
	void clearSSL();

	

	void setNumber(char *phone_num);
	
	void setFirstInfo(Transaction *tr);

	char* getIpAddr()
	{
		return ipAddr;
	}
	
	char* getNumber()
	{
		return phoneNumber;
	}
	
	int	getSock()
	{
		return sockfd;
	}

	int	getPort()
	{
		return port;
	}
	
	void setSSLHost(const host_t& host)
	{
		sslHost = host;
	}

	host_t getSSLHost()
	{
		return sslHost;
	}

	void setSSLPort(int _port)
	{
		sslPort = _port;
	}

	int getSSLPort()
	{
		return sslPort;
	}

	void setLastTransactionTime(time_t given_time=0);
	time_t getLastTransactionTime()
	{
		return lastTrSec;
	}

	void onSslDataRecv(int recvBytes);
	void onSslDataSend(int sendBytes);

	void setCPname(Transaction *tr);

	bool needCookieHandler() const;

public:
	
	int reqNums; // ���� ���ǿ����� �ܸ� request ȸ��
	int respNums; // ���� ���ǿ����� �ܸ� response ȸ��

	bool isSantaNumber; // SANTA ��ȸ ����� session �� �����Ͽ����� ǥ��.
	int threadId; // thread ��ȣ
	int sockfd; // ���� ��ȣ
	char ipAddr[LEN_IPADDR+1]; // �ܸ����� IP �ּ� ���ڿ�(123.123.123.123)
	int	port; // �ܸ��� port
	unsigned int intIpAddr;	// �ܸ����� IP �ּ� unsigned int ��

	url_t lastRequestURL;

	time_t pasauthTime;	// PasAUTH �����ð� (���� ��û �ð�)
	time_t santaTime; // Santa �����ð� (���� ��� ���� �ð�)

	char phoneNumber[LEN_PHONE_NUM+1]; // MIN or MDN
	char IMSI[LEN_IMSI+1];
	char msModel[LEN_MS_MODEL+1];
	char browser[LEN_BROWSER+1];
	char cpName[LEN_CP_NAME+1];
	char svcCode[LEN_SVC_CODE+1];

	time_t	startSec; // session start time
	time_t	lastTrSec; // ������ transaction �Ϸ� �ð�.
	int	lastHttpTransactionReqnum;  // SSL ������ ������ HTTP request �� seq num

	bool is_SSL; // �� ������ ���� SSL ���������� ǥ��.
	host_t sslHost; // SSL ��� ȣ��Ʈ �̸� �Ǵ� �ּ�.
	int sslPort;  //  SSL ��� ��Ʈ
	time_t sslStartTm; //SSL ȣ ���� �ð� 
	time_t sslEndTm; // SSL ȣ ���� �ð�

	
	int	sslReqBytes; // SSL ȣ ���� �ܸ����� ������  ����Ʈ ����ġ.
	int	sslRespBytes; // SSL ȣ ���� �ܸ��� �۽��� ����Ʈ ����ġ.
};


#endif
