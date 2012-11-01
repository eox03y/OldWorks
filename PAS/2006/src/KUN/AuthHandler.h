#ifndef __AUTH_EVENT_HANDLER_H__
#define __AUTH_EVENT_HANDLER_H__

/**
@brief PasAuth ���� ó��

��ְ� �߻��ϸ� ���ī��Ʈ�� ������Ű��
config �� ������ ���� �����ϸ� ��� ��Ȳ���� �Ǵ��ϰ�
������ ��� ������ ���� ó���Ѵ�.
��ֻ�Ȳ ������, ���ī��Ʈ�� ��� �����ϸ� �����ȴ�.
*/

#include "AuthTypes.h"
#include "PasEventHandler.h"
#include "UserInfo.h"


class AuthHandler :
	public PasHandler
{
// ����Լ�
public:
	AuthHandler(ACE_Reactor* pReactor);
	~AuthHandler(void);

	int handle_timeout(const ACE_Time_Value& current_time, const void* act);

	int start(const int seq, const char* pMin, const unsigned int ip, const int port, const int startConn, 
		const int newBrowser, int g3GCode, const char* pMdn = NULL, const char* pMsModel = NULL);

	AuthState getState();
	bool isPassed();
	bool isRemovable();
	void set(UserInfo *userInfo);
	AUTH::RequestBody *getRequestBody();
	UserInfo *getUserInfo();
	virtual void close();

protected:
	virtual void onConnect();
	virtual void onRecvFail();
	virtual void onSendFail();
	virtual char *setMyInfo();

private:
	int connect();
	int request();
	int onCompletedReceiveResponse();
	void init();
	virtual void onReceived();

// �������
public:

protected:

private:
	AUTH::RequestBody requestBody;

	AUTH::ResponseBody responseBody;

	AuthState authState;

	UserInfo *puserInfo;
};

#endif
