#include "DNSQuerier.h"
#include "PasLog.h"
#include "MemoryPoolManager.h"
#include "Util2.h"
#include "Mutex.h"

using namespace DNS;

Querier::Querier()
{
	setIdle();
}

Querier::~Querier()
{
	// ť�� �ִ� MessageBlock ����
	ACE_Message_Block* pMB;
	ACE_Time_Value timeout(0, 0); // non block
	
	while(getq(pMB, &timeout) >= 0)
	{
		MemoryPoolManager::instance()->free(pMB);
	}

	while(responseQueue.dequeue(pMB, &timeout) >= 0)
	{
		MemoryPoolManager::instance()->free(pMB);
	}
}

bool Querier::isIdle()
{
	return state == DQS_IDLE;
}

bool Querier::isTimeout()
{
	return state == DQS_TIMEOUT;
}

ACE_Message_Block* Querier::getResponse(ACE_Time_Value* pTimeout)
{
	ACE_Message_Block* pMB;
	int ret = responseQueue.dequeue(pMB, pTimeout);
	if(ret < 0)
		return NULL;

	return pMB;
}

void Querier::query(const ACE_Message_Block* pMB)
{
	const int destLen = 1024;
	char dotstrIP[destLen];

	PAS_TRACE1("DNS Querier >> Query %s", pMB->rd_ptr());
	bool result = Util2::getHostByName_threadsafe(pMB->rd_ptr(), dotstrIP, destLen);
	
	{
		ReadMutex<ACE_Thread_Mutex> guard(lock);
		
		// Timeout ���� ���õǾ� ���� �ʴٸ�, �ܺο��� ������ ��ٸ��� �ִ� ���¶�� �� �� �ִ�.
		if(isTimeout() == false)
		{
			ACE_Message_Block* pDestMB = MemoryPoolManager::instance()->alloc(destLen);
			ACE_ASSERT(pDestMB != NULL);
			ACE_ASSERT(static_cast<int>(pDestMB->size()) >= destLen);

			// ������ ��� IP �� ť�� �ִ´�.
			if(result == true)
			{
				pDestMB->copy(dotstrIP, strlen(dotstrIP)+1);
				PAS_TRACE2("DNS Querier >> Receive Response, %s => %s", pMB->rd_ptr(), dotstrIP);
				responseQueue.enqueue(pDestMB);
			}

			// ������ ��� empty �� ť�� �ִ´�.
			else
			{
				responseQueue.enqueue(pDestMB);
				PAS_INFO1("DNS Querier >> DNS Lookup Fail, %s", pMB->rd_ptr());
			}
		}

		// �ܺο��� Timeout ���� �����ߴٸ�, DNS Query ����� ����ť�� ���� �ʴ´�.
		else
		{
			// do nothing
			PAS_INFO1("DNS Querier >> Timeout, %s", pMB->rd_ptr());
		}
	}
}

int Querier::svc(void)
{
	PAS_NOTICE("Start DNS Querier");
	
	ACE_Message_Block* pMB;
	while(getq(pMB) >= 0)
	{
		ACE_ASSERT(pMB != NULL);

		setBusy();
		
		// ���� �ñ׳�
		if(pMB->length() == 0)
		{
			MemoryPoolManager::instance()->free(pMB);
			break;
		}

		query(pMB);

		MemoryPoolManager::instance()->free(pMB);

		setIdle();
	}

	PAS_NOTICE("End DNS Querier");
	return 0;
}

void Querier::setBusy()
{
	state = DQS_BUSY;
}

void Querier::setIdle()
{
	state = DQS_IDLE;
}


void Querier::setTimeout()
{
	WriteMutex<ACE_Thread_Mutex> guard(lock);

	state = DQS_TIMEOUT;

	// DNS Query ����� ����ť(responseQueue)�� push�ϴ� ���̿� �ܺο����� 
	// setTimeout() �� ȣ���Ѵٸ�, ����ť�� ����޽����� ����ְԵȴ�.

	ACE_Message_Block* pMB;
	ACE_Time_Value timeout(0, 0);
	// ���� ��û�ڸ� ���� response queue �� ������ �� �ִٸ� ��������.
	while(responseQueue.dequeue(pMB, &timeout) >= 0)
	{
		MemoryPoolManager::instance()->free(pMB);

		// ����ť�� �����Ͱ� �ִٴ� ���� DNS Query�� �Ϸ������ �ǹ��Ѵ�.
		// �׷��Ƿ� ���¸� IDLE �� �����Ѵ�.
		state = DQS_IDLE;
	}
}


