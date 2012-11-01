#ifndef __MUTEX_H__
#define __MUTEX_H__

/**
@brief Read/Write Mutex ����

�ʱ� ��(WorkerThread Pool)���� ACE Mutex �� ����� ��
acquire/release �� �Ǽ��� ���ϱ� ���Ͽ� ���� ���ø� Ŭ����

���� ��(Reactor Pool)������
MemoryPool, UserInfoMng, PhoneTraceMng, StatFilterMng ����� ���ȴ�.
*/


#include "Common.h"
#include "PasLog.h"

template<typename ACE_MUTEX>
class ReadMutex
{
public:
	ReadMutex(ACE_MUTEX& lock) : lock_(lock)
	{
		//PAS_TRACE0("lock.acquire_read try...");
		lock_.acquire_read();
		//PAS_TRACE0("lock.acquire_read acquired");
	}

	~ReadMutex()
	{
		lock_.release();
		//PAS_TRACE0("lock.release_read");
	}

protected:
	ACE_MUTEX& lock_;
};

template<typename ACE_MUTEX>
class WriteMutex
{
public:
	WriteMutex(ACE_MUTEX& lock) : lock_(lock)
	{
		//PAS_TRACE0("lock.acquire_write try...");
		lock_.acquire_write();
		//PAS_TRACE0("lock.acquire_write acquired");
	}

	~WriteMutex()
	{
		lock_.release();
		//PAS_DEBUG0("lock.release_write");
	}

protected:
	ACE_MUTEX& lock_;
};

#endif
