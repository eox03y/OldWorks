#ifndef __ACL_ROUTER_H__
#define __ACL_ROUTER_H__

/**
@file   AclRouter.h
@author DMS
@brief  magicN DNS ��� - PAS ��ü������ ���� ����

-ACL ���� ����-

[�ĺ���(4)] [����(14)]
## End of ���
[������(5)] [IP(15)] [PORT(5)] [??(1)]
## End of PAS
[Src Addr] [Src Port] [Dest Addr] [Dest Port] [������] [CP �� Proxy �����ΰ�]
## End of DNS
OV14			<- ACL DNS ����
[Src Addr] [Src Port] [Dest Addr] [Dest Port] [������] [CP �� Proxy �����ΰ�]
## End of ACL
ACLX            <- tail...end of file

-���-
1. �����(##)�� �� �ʵ��� ���� ��Ÿ����.

2. ACL ������ �ε��ϱ� ���� ������ üũ�Ѵ�.

3. DNS �������� �������� 00000 �� �ƴϰų�, �ڽ��� ������(��:KUN01)�� �ٸ��� �����Ѵ�.

4. �ε尡 ���� �� ACL Version�� ���� �Ǿ��ٸ� Monitor ������ ACL Info�� �۽��Ѵ�.
-ACL Info : ���� ȣ��Ʈ��, pasgw ��Ʈ, ACL DNS ����

5. ������ �˻��� ME �� KUN ���� �ִ�.
- DNS �˻��� KUN, ME ��
- ACL �˻��� KUN ��

6. ������ �˻� �˰���(KUN)
- ACL �� �˻��Ѵ�.
- DNS �� �˻��Ѵ�.
- DNS �����Ͱ� ������ Dest Addr�� �ٽ� ACL �˻� �Ѵ�.
- �Ѵ� ���� ��� URL�� nslookup �Ͽ� IP�� ������ ���� ACL�� �˻��Ѵ�.
- ACL �����Ͱ� ������ HOST ID �� �ڽŰ� ���Ѵ�. ������ Dest Addr�� �����ϰ�
  �ٸ��� �ش� HOST ID�� IP�� ��ƼProxy ó���Ѵ�.
- DNS �����Ͱ� ������ HOST ID �� �ڽŰ� ���Ѵ�. ������ Dest Addr�� �����ϰ�
  �ٸ��� �ش� HOST ID�� IP�� ��ƼProxy ó���Ѵ�.

7. ���ϰ��� �ǹ�
ACL_NOT_FOUND : �Է��� url �� DNS �� ACL �� �˻� �Ͽ����� �����Ͱ� ���
                url �� nslookup �Ͽ� ip �� ������ ���� ACL �� �˻������� ���� ����.

ACL_DENY_ACL  : ACL ���� �����͸� ã������, HOST(KUN01)�� �ٸ��� ������ Multi Proxy ������� ����
                ��, 9090 ��Ʈ�� KUN00 �̹Ƿ� 9091 ��Ʈ KUN01 �� ������ �϶�� �ǹ�

ACL_ALLOW_ACL : ACL ���� �����͸� ã�Ұ�, HOST(KUN01)�� �����Ƿ� ������ ����Ѵ�.
                �� ���� 9091 ��Ʈ�� Multi Proxy �Ǿ� ���� �����̰ų�
                ���ʿ� Multi Proxy �� �ʿ���� �����̰ų� ���� �Ѱ��� ����̴�.

ACL_FIND_DNS  : DNS ���� �����͸� ã�Ҵ�.

-Pasgw ���� AclRouter API ����-
1.ME ������ searchDNS �� ����Ѵ�.
���ϰ��� ACL_NOT_FOUND Ȥ�� ACL_FIND_DNS ���� �ϳ�
2.KUN ������ searchALL �� ����Ѵ�.
ACL_FIND_DNS, ACL_ALLOW_ACL �� ��� ��ȯ�� url �� port �� ����Ѵ�.
ACL_NOT_FOUND �� ��쿡�� ���� url �� port �� ����Ѵ�.
ACL_DENY_ACL �� ��쿡�� ��ȯ�� url �� port �� Multiproxy ó�� �ؾ��Ѵ�.

Multiproxy ��
KUN ���� �ش��ϴ� ����̰�, �ܸ��Ⱑ 9090 ������ �����ؼ� ��û�� ������ ���
pasgw�� �켱 ACL ����� �˻��ϰ� �ǰ� ã�� �������� �ִٸ� Host ���� ���ϰ� �ȴ�.
������ Host ���� 9090 �� ����(KUN00) �ƴ϶�� �ܸ��⿡ 9091(KUN01)�� ������ �϶��
�˷��ְԵǰ�, �ܸ���� 9091 ������ ������ �Ѵ�.
*/

#include <string>
#include <ace/Task_T.h>
#include <ace/Singleton.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>

#include "Common.h"
#include "MyFile.h"

using namespace std;


#define	MAXLEN_URL	(255)
#define DNS_URL_SIZE		99
#define HEAD_VERSION_SIZE	14
#define HOST_NAME_SIZE		16

#define ACL_PREFIX			"PAS1"
#define ACL_TAIL			"ACLX"
#define ALLTYPE_PASID		"00000"
#define ACL_HEADER_VERSION_DIGIT_START	2
#define ACL_APPLY_HOSTNAME_SIZE			24
#define ACL_MONITOR_SERVER	"221.148.247.32"
#define ACL_MONITOR_PORT	5015

#define ACL_NOT_FOUND		0
#define ACL_INPUT_ERR		-1
#define ACL_DENY_ACL		-2
#define ACL_ALLOW_ACL		-3
#define ACL_FIND_DNS		-4

#define ACL_SYNTAX_ERROR	-1
#define ACL_FILE_NOT_FOUND	-2
#define ACL_VERSION_ERROR	-3
#define ACL_TAIL_ERROR		-4


struct ACL_KEY
{
	string SrcAddr;
	int SrcPort;

	bool operator < (const ACL_KEY& ref) const
	{
		if (SrcAddr == ref.SrcAddr)
		{
			return SrcPort < ref.SrcPort;
		}
		else
			return SrcAddr < ref.SrcAddr;
	}
};

struct ACL_DATA
{
	ACL_DATA()
	{
		memset( SrcAddr, 0x00, sizeof(SrcAddr) );
		memset( SrcPort, 0x00, sizeof(SrcPort) );
		memset( DestAddr, 0x00, sizeof(DestAddr) );
		memset( DestPort, 0x00, sizeof(DestPort) );
		memset( Owner, 0x00, sizeof(Owner) );
		OnOff = 0;
	};

	char SrcAddr[DNS_URL_SIZE+1];
	char SrcPort[6];
	char DestAddr[DNS_URL_SIZE+1];
	char DestPort[6];
	char Owner[6];
	char OnOff;
};

struct ACL_HOST
{
	ACL_HOST()
	{
		memset( Name, 0x00, sizeof(Name) );
		memset( Addr, 0x00, sizeof(Addr) );
		memset( Port, 0x00, sizeof(Port) );
		memset( Opt, 0x00, sizeof(Opt) );
	};

	char Name[HOST_NAME_SIZE+1];
	char Addr[DNS_URL_SIZE+1];
	char Port[6];
	char Opt[2];
};

struct ACL_RECORD_INFO
{
	char szHostName[ACL_APPLY_HOSTNAME_SIZE];
	unsigned int nPort;
	unsigned int nVersion;
};

typedef map<ACL_KEY, ACL_DATA*>						mapACLDATA;
typedef map<ACL_KEY, ACL_DATA*>::iterator			iterACLDATA;
typedef map<string, ACL_HOST*>						mapACLHOST;
typedef map<string, ACL_HOST*>::iterator			iterACLHOST;

class AclRouter : public ACE_Task<PAS_SYNCH>
{
private:
	char headVersion[HEAD_VERSION_SIZE+1];		/// ACL ���� ����
	char PasID[HOST_NAME_SIZE+1];				/// KUN00, KUN01 ��...
	int ACLVer;									/// ACL ����(��:OV14)
	int mTime;									/// ACL ������ ������ ���� �ð�
	int PasgwPort;								/// PAS ���� ��Ʈ
	int run;									/// Worker thread ����� �÷�
	mapACLDATA mapDNS;							/// DNS ��� (KUN, ME ��)
	mapACLDATA mapACL;							/// ACL ��� (KUN ��)
	mapACLHOST mapHost;							/// PAS ȣ��Ʈ ���
	ACE_RW_Thread_Mutex rwMutex;				/// ����ȭ RW Mutex ��ü

private:
	bool setHead(MyFile &fp);
	bool setHost(char *pBuff);
	bool setDNS(char *pBuff);
	bool setACL(char *pBuff);
	void clearHost();
	void clearDNS();
	void clearACL();

protected:
	inline int chkACLHeader(const char* pszHeader);
	bool syntaxCheck(const char* pszFileName);
	//virtual int init (int argc, ACE_TCHAR *argv[]);

public:
	AclRouter(ACE_Thread_Manager* threadManager=0)
		: ACE_Task<PAS_SYNCH>(threadManager, NULL)
	{
		headVersion[0] = 0;
		PasID[0] = 0;
		ACLVer = 0;
		mTime = 0;
		PasgwPort = 9090;
		run = 1;

		STRNCPY( PasID, "KUN00", HOST_NAME_SIZE );
	}

	~AclRouter();

	virtual int svc();

	int initial(const char *pHostID, int nPasgwPort);
	void remove_worker_thread(void);
	int load(const char *pFileName);
	int	searchDNS(const char *orgHost, const int orgPort, char *destHost, int destHostSize, int &destPort);
	int	searchALL(const char *orgHost, const int orgPort, char *destHost, int destHostSize, int &destPort);

	static AclRouter *instance();
	void getHost(char *host, int hostsize, int &port, const char *hostname);

	void test();
};

#endif
