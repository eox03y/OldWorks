// -*- C++ -*-

/**
@file UserInfo.h

@author DMS

@brief ����� ���� ����

UserInfo �� �����Ѵ�.
Ư���ð�(config ��)�� ������ ��Ͽ��� �����Ѵ�.
���� IP, MDN ���� �˻� �� �� ������, ����/�߰�/���� �� �� �ִ�.
*/


#ifndef USERINFOMNG_H
#define	USERINFOMNG_H

#include "UserInfo.h"

#include <list>
#include <map>

//#include "ace/Thread_Mutex.h"

using namespace std;


class UserInfoMng:
	public ACE_Task<PAS_SYNCH>
{
public:
	static UserInfoMng *instance(ACE_Thread_Manager* threadManager=0);
	
	
	virtual ~UserInfoMng(void);

	virtual int svc(void);

	int	removeIdleUsers(void);
	
	UserInfo *getUser(char *MDN, char*ipAddr);
	UserInfo * search(char *MDN, char*ipAddr);
	UserInfo * searchByMdn(char *MDN, char*ipAddr);
	UserInfo * searchByAddr(char *MDN, char*ipAddr);
	UserInfo * add(char *MDN, char*ipAddr);
	int	getCount()
	{
		return userCount;
	}
	
	void	stop()
	{
		runFlag = false;
	}

private:
	static UserInfoMng *oneInstance;
	UserInfoMng(ACE_Thread_Manager* threadManager=0);
	
	PasMutex lockUserInfo;
	map<intMDN_t,  UserInfo*> MDNs;
	map<intIP_t,  UserInfo*> IPs;
	int	userCount;
	int	maxIdleSec;
	bool runFlag;

};

#endif
