#ifndef REACTORPOOL_H__
#define REACTORPOOL_H__

/**
@brief ACE Reactor ���� Ŭ����

Client(�ܸ���)�� �����ϸ� ClientHandler �� �����ϰ�
���Ŀ� �߻��ϴ� ��� �̺�Ʈ�� ACE Reactor �� �����Ѵ�.
�� ��, � Reactor �� ����� ������ ReactorPool �� �����.
ReactorPool �� Reactor ���� ������� �˷��ش�.
*/


#include <ace/Reactor.h>

#include "ReactorInfo.h"
#include "MyLog.h"


class ReactorPool
{
// ����Լ�
public:
	
	static ReactorPool* instance();
	ACE_Reactor *createMaster();
	 ACE_Reactor *masterReactor();
	 ReactorInfo *workerReactor();
	 int createWorkers(int _numWorkers);
	 void stopWorkers();
	 int getNumWorkers()
	 {
	 	return numWorkers;
	 }

	 void	print(MyLog *log);


private:

	ReactorPool();
	~ReactorPool();
	void deleteAll();

// �������
public:

private:
	static ReactorPool *oneInstance;

	ACE_Reactor *master;
	ReactorInfo *workerList;
	int	numWorkers;
	int reactorIterator;

	ACE_Thread_Mutex	poolLock;
	
};

#endif
