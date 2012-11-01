#include <string.h>

#include <ace/Thread_Mutex.h>
#include <ace/Thread_Manager.h>

#include "Common.h"
#include "DNSQuerier.h"
#include "DNSManager.h"
#include "Mutex.h"
#include "Config.h"
#include "DNSCache.h"
#include "Util.h"

using namespace DNS;

Manager::Manager()
{
	numOfQueriers = 0;
	MaxNumOfQueriers = Config::instance()->dns.maxNumOfQueriers;    
	QueryTimeoutSec = Config::instance()->dns.queryTimeoutSec;
	CacheEnable = Config::instance()->dns.cacheEnable;
	CacheTimeoutSec = Config::instance()->dns.cacheTimeoutSec;
}

Manager::~Manager()
{
}

Manager* Manager::instance(void)                  
{                                                             
	return ACE_Singleton<Manager, ACE_SYNCH_MUTEX>::instance();
}                      		

void Manager::removeAllQuerier()
{
	PAS_NOTICE("DNS Manager >> Stop All Querier");

	while(size() > 0)
	{
		bool result = removeQuerier();
		if(result == false)
			sleep(1);
	}
}

bool Manager::getHostByName(const char* domainName, char* dotstrIP, size_t destlen)
{
	if(CacheEnable)
		return getHostByNameCache(domainName, dotstrIP, destlen);
	else
		return getHostByNameDirect(domainName, dotstrIP, destlen);
}

bool Manager::getHostByNameDirect(const char* domainName, char* dotstrIP, size_t destlen)
{
	PAS_TRACE1("DNS Manager >> Quering %s", domainName);

	bool queryResult = queryToDNS(domainName, dotstrIP, destlen);

	// success
	if(queryResult)
	{
		PAS_TRACE("DNS Manager >> DNS Query success");
		return true;
	}

	// fail
	else
	{
		PAS_TRACE("DNS Manager >> DNS Query fail");
		return false;
	}
}

bool Manager::getHostByNameCache(const char* domainName, char* dotstrIP, size_t destlen)
{
	PAS_TRACE1("DNS Manager >> Querying %s", domainName);

	CacheData cacheData;
	const host_t host(domainName);

	// Look up in cache
	bool foundCache = cache.get(host, cacheData);

	// found in cache
	if(foundCache)
	{
		ACE_ASSERT(cacheData.host == domainName);

		time_t diff = time(NULL) - cacheData.updateTime;
		bool isExpired = (diff > CacheTimeoutSec);

		// if not expired then using cached data
		if(isExpired == false)
		{
			Util::int2ipaddr(cacheData.ip, dotstrIP, destlen);

			PAS_TRACE2("DNS Manager >> Found in cache, %s => %s", domainName, dotstrIP);

			return true;
		}

		PAS_TRACE1("DNS Manager >> Cache expired, %s", domainName);
	}
	else
	{
		PAS_TRACE1("DNS Manager >> Not found in cache, %s", domainName);
	}

	// cache expired or not found in cache
	bool queryResult = queryToDNS(domainName, dotstrIP, destlen);

	// success
	if(queryResult)
	{
		PAS_TRACE("DNS Manager >> DNS Query success");

		// update cache
		if(foundCache)
		{
			host_t host(domainName);
			cacheData.set(host, Util::ipaddr2int(dotstrIP));
			cache.set(cacheData);
		}
		
		// add to cache
		else
		{
			// Cache�� ������ �޸� ������ �������ؼ�, Cache �����Ͱ� 10���� ���ϸ� �߰��ϰ�, �ƴϸ� �߰����� �ʴ´�.
			if(cache.size() < 100000)
			{
				host_t host(domainName);
				cacheData.set(host, Util::ipaddr2int(dotstrIP));
				cache.set(cacheData);
			}
		}

		return true;
	}

	// fail
	else
	{
		PAS_INFO("DNS Manager >> DNS Query fail");

		// DNS Query �� fail �϶�, ����� Cache �����Ͱ� �ִٸ� cache �� �����͸� ����Ѵ�
		if(foundCache)
		{
			Util::int2ipaddr(cacheData.ip, dotstrIP, destlen);

			PAS_INFO2("DNS Manager >> Use expired cache data, %s => %s", domainName, dotstrIP);

			return true;
		}

		return false;
	}

}

bool Manager::queryToDNS(const char* domainName, char* dotstrIP, size_t destlen)
{
	Querier* pQuerier = getIdleQuerier();
	if(pQuerier == NULL)
	{
		if(size() < MaxNumOfQueriers)
		{
			pQuerier = createQuerier();
			PAS_NOTICE2("DNS Manager >> Number of Querier is %d, Number of Timeout is %d", 
				size(), getNumOfTimeout());
		}
		else
		{
			PAS_NOTICE2("DNS Manager >> DNS Querier is Full, Number of Querier is %d, Number of Timeout is %d", 
				size(), getNumOfTimeout());

			return false;
		}
	}

	ACE_ASSERT(pQuerier != NULL);

	ACE_Message_Block* pMB = MemoryPoolManager::instance()->alloc(MAX_HOST_LEN);
	ACE_ASSERT(pMB != NULL);
	ACE_ASSERT(pMB->length() == 0);
	pMB->copy(domainName, strlen(domainName)+1);

	pQuerier->putq(pMB);

	ACE_Time_Value timeout(ACE_OS::gettimeofday());
	timeout += ACE_Time_Value(QueryTimeoutSec, 0);
	pMB = pQuerier->getResponse(&timeout);
	
	// DNS Query Timeout
	if(pMB == NULL)
	{
		// �ٸ� �����尡 ������� �ʵ��� TIMEOUT ���·� ť�� ����
		pQuerier->setTimeout();
		putQuerier(pQuerier);
		PAS_NOTICE2("DNS Manager >> DNS Timeout %d sec, %s", QueryTimeoutSec, domainName);
		return false;
	}

	// Get Response
	else
	{
		// �ٸ� �����尡 ����� �� �ֵ��� ť�� ����
		putQuerier(pQuerier);
	}

	// DNS Query fail
	if(pMB->length() == 0)
	{
		MemoryPoolManager::instance()->free(pMB);				
		PAS_INFO1("DNS Manager >> DNS Query Fail, %s", domainName);
		return false;
	}

	// dest buf size is small
	if(destlen < pMB->length())
	{
		MemoryPoolManager::instance()->free(pMB);
		PAS_NOTICE1("DNS Manager >> Dest Buf size is too small, %s", domainName);
		return false;
	}

	strncpy(dotstrIP, pMB->rd_ptr(), pMB->length());

	PAS_DEBUG2("DNS Manager >> %s => %s", domainName, dotstrIP);
	
	MemoryPoolManager::instance()->free(pMB);
	return true;
}

Querier* Manager::createQuerier()
{
	PAS_NOTICE("DNS Manager >> Create New Querier");

	Querier* pQuerier = new Querier();
	pQuerier->activate();

	numOfQueriers++;

	return pQuerier;
}

bool Manager::removeQuerier()
{
	WriteMutex<ACE_Thread_Mutex> guard(querierLock);

	if(queriers.empty() == true)
		return false;

	int queueSize = queriers.size();
	int scanSize = 0;
	bool fullScan = false;
	while(fullScan == false)
	{
		Querier* pQuerier = queriers.front();
		queriers.pop();

		// idle �� querier �� ã�Ƽ� �����Ѵ�.
		if(pQuerier->isIdle() == true)
		{
			ACE_Message_Block* pMB = MemoryPoolManager::instance()->alloc(1);
			ACE_ASSERT(pMB);
			ACE_ASSERT(pMB->length() == 0);
			
			// put empty MB for thread stop
			pQuerier->putq(pMB);
			ACE_Thread_Manager::instance()->wait_task(pQuerier);

			delete pQuerier;

			numOfQueriers--;

			return true;
		}

		queriers.push(pQuerier);
		
		++scanSize;
		fullScan = (scanSize >= queueSize);
	}

	return false;
}

Querier* Manager::getIdleQuerier()
{
	WriteMutex<ACE_Thread_Mutex> guard(querierLock);

	if(queriers.empty() == true)
		return false;

	int queueSize = queriers.size();
	int scanSize = 0;

	// ť�� ��ȸ�ϸ鼭 IDLE �� ���� ã�Ƽ� �����Ѵ�.
	bool fullScan = false;
	while(fullScan == false)
	{
		Querier* pQuerier = queriers.front();
		queriers.pop();
		
		if(pQuerier->isIdle() == true)
		{
			return pQuerier;
		}

		queriers.push(pQuerier);

		++scanSize;
		fullScan = (scanSize >= queueSize);
	}

	return NULL;
}

int Manager::getNumOfTimeout()
{
	ReadMutex<ACE_Thread_Mutex> guard(querierLock);

	if(queriers.empty() == true)
		return 0;

	int numOfTimeout = 0;
	int queueSize = queriers.size();
	int scanSize = 0;

	// ť�� ��ȸ�ϸ鼭 IDLE �� ���� ã�Ƽ� �����Ѵ�.
	bool fullScan = false;
	while(fullScan == false)
	{
		Querier* pQuerier = queriers.front();
		queriers.pop();
		
		if(pQuerier->isTimeout() == true)
		{
			numOfTimeout++;
		}

		queriers.push(pQuerier);

		++scanSize;
		fullScan = (scanSize >= queueSize);
	}

	return numOfTimeout;
}

void Manager::putQuerier(Querier* pQuerier)
{
	WriteMutex<ACE_Thread_Mutex> guard(querierLock);

	queriers.push(pQuerier);
}

int Manager::size()
{
	return numOfQueriers;
}
