/**
@file SysInfo.h

@brief ���� ���� ���� ���

�ֱ������� ���� ���� ������ sysinfo-9090.MMDD.log ȭ�Ͽ� �����Ѵ�.
�׸��� 1�ʸ���  ���� ���� ������ sysinfo-current-9090.MMDD.log ȭ�Ͽ� �����Ѵ�.
�ֱⰪ (�ʴ���)��  ���Ǳ� ȭ�Ͽ� �����Ѵ�.

���� ���� �������� thread ���� , user  ����, accpet  ����, 

@author 
@date 2006.09.12
*/
#ifndef SYSINFO_H
#define SYSINFO_H

#include "SysInfo.h"
#include "MyLog.h"

class SysInfo:
	public ACE_Task<PAS_SYNCH>
{
public:
	static SysInfo *instance(ACE_Thread_Manager* threadManager=0);
	
	
	virtual ~SysInfo(void);

	virtual int svc(void);

	void stop();

	void	printSysInfo();

	void	overWriteLog();
		
private:
	static SysInfo *oneInstance;
	SysInfo(ACE_Thread_Manager* threadManager=0);

	bool runFlag;
	int	last_mtime;
	MyLog *sysinfolog;
	MyLog *overwritelog;
	
};
#endif
