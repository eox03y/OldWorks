#ifndef __MUTEX_LIST_H__
#define __MUTEX_LIST_H__

#include "Common.h"
#include <list>
#include "Mutex.h"

template<typename T1>
class MutexList
{
// ��� �Լ�
public:
	void push(T1& val)
	{
		WriteMutex<PasMutex> writeLock(lock);
		nullMutexList.push(val);
	}

	T1 front()
	{
		ReadMutex<PasMutex> readLock(lock);
		return nullMutexList.front();
	}

	int pop(T1& val)
	{
		int result = 0;

		WriteMutex<PasMutex> writeLock(lock);
		
		if(nullMutexList.empty())
		{
			result = -1;
		}
		else
		{			
			val = nullMutexList.front();
			nullMutexList.pop();
		}

		return result;
	}
	
	bool empty()
	{
		ReadMutex<PasMutex> readLock(lock);
		bool ret = nullMutexList.empty();
		return ret;
	}

	size_t size()
	{
		ReadMutex<PasMutex> readLock(lock);
		size_t ret = nullMutexList.size();
		return ret;
	}

private:

// ��� ����
private:
	PasMutex lock;
	std::list<T1> nullMutexList;
};

#endif
