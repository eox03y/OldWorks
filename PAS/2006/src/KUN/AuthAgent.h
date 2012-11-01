/**
@file AuthAgent.h

@brief PASGW�� AUTH ���� ��� ���

Pasgw �� Auth �� ����� ����Ѵ�.
��� ������ UserInfo �� �Ѵ�.
Auth�� ������ ��������, ��ֻ��·� ����Ǹ� ������ ��� ������ ������ �� �� ���� ���� ó���Ѵ�.
��ֻ��°� �Ƿ���, ����ī��Ʈ�� ���� ��ġ�� �Ǿ�� �ϸ� config �� ���� �� �� �ִ�.(ErrorCount)
Auth�� ������ �õ��� ���� �ð��� ������ �õ��Ѵ�. config �� ���� �� �� �ִ�.(RetryDelay)
���� ���� �� �������� �ϴ� ������, userinfo �� ������ų� config �� Authentication �ð��� ��� �Ͽ��� ��� �������� ��ģ��.


@author ����â
@date 2006.10.30
*/
#ifndef __AUTH_AGENT_H__
#define __AUTH_AGENT_H__

#include "AuthInfo.h"
#include "MyLog.h"
#include "AuthTypes.h"

#include <ace/SOCK_Stream.h>

class AuthAgent : public ACE_Task<PAS_SYNCH>
{
	// Member Functions
public:
	static AuthAgent *instance(ACE_Thread_Manager* threadManager=0);
	void stop();
	int putWork(AUTH::RequestBody &reqBody, UserInfo *userinfo);
	virtual ~AuthAgent();
	virtual int svc();

protected:
	bool connecttoAuth(const char* ip, const int port);
	void closesocket();
	AuthInfoMessageBlock *getAuthInfo(ACE_Message_Block *mesg);

private:
	static AuthAgent *oneInstance;
	AuthAgent(ACE_Thread_Manager* threadManager=0);
	int sendtoAuth(AUTH::RequestBody *sendbuff, int size);
	int recvfromAuth(AUTH::ResponseBody *recvbuff, int size);
	AuthState getAuthState( AUTH::ResponseBody *respBody );
	bool connCheck();
	void incErrCount();
	void decErrCount();
	void responseLog(const AUTH::ResponseBody &responseBody, const char *pment=NULL);
	void requestLog(const AUTH::RequestBody &requestBody, const char *pment=NULL);

	// Member Variables
public:
protected:
private:
	int errCount;
	long errTime;
	bool run;
	bool isconnected;
	MyLog *authlog;
	ACE_SOCK_Stream sock;
};

#endif
