/**
@file PhoneTrace.h

@brief phone trace ��ȭ ��ȣ ��� ����

ȭ�Ͽ��� ����Ÿ�� �ε��ϰ�, search ����� �����Ѵ�.
mutex �ð��� ���̰�, ���� ������ ���߱� ���� 2���� ����� �����ϸ鼭, 2 �� ����� ������ ����Ѵ�.
��ȭ��ȣ ����� PhoneTrace Ŭ������ �����Ѵ�.

@author KTF
@date 2006.09.12
*/
#ifndef PHONETRACEMNG_H
#define PHONETRACEMNG_H

#include "PhoneTrace.h"
#include "MyLog.h"

class PhoneTraceMng:
	public ACE_Task<PAS_SYNCH>
{
public:
	static PhoneTraceMng *instance(ACE_Thread_Manager* threadManager=0);
	
	
	virtual ~PhoneTraceMng(void);

	virtual int svc(void);

	void	setDataFile(char *fname);
	
	/// �־��� ��ȭ��ȣ�� trace ��� ��ȣ���� �˻�
	bool isTrace(intMDN_t MDN);

	/// �־��� ��ȭ��ȣ�� ���� Log ȭ���� �޴´�.
	MyLog *getTraceLog(char * MDN);
	MyLog *getTraceLog(intMDN_t MDN);
	void stop();
	void    prn(StrStream_t  &out);
		
		
private:
	static PhoneTraceMng *oneInstance;
	PhoneTraceMng(ACE_Thread_Manager* threadManager=0);

	bool runFlag;

	/// ���� ���� ���� �˻�
	bool check();

	/// ȭ�� �ε�
	int	load();
	// ����Ÿ �ε� �Ŀ��� �ݵ�� setRealList() �����ؾ� �Ѵ�.
	void	setRealList();

	int	last_mtime;
	int	last_fsize;
	char	dataFile[256];
	
	PasMutex lock;
	PhoneTrace *phoneList;
	PhoneTrace *newList;
	PhoneTrace dataA, dataB;

	
};
#endif
