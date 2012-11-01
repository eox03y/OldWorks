#ifndef __EVENT_INFO_MESSAGE_BLOCK_H__
#define __EVENT_INFO_MESSAGE_BLOCK_H__

/**
@brief Message Queue ����

ACE Message Queue �� ����ϱ� ���ؼ� ACE_Message_Block �� ��ӹ��� Ŭ����
���� ACE Message Queue �� ������, Block ���ο� �����͸� �����ؼ� ����ؾ� ������
���ʿ��� ���簡 �̷������, �� �������� ũ�Ⱑ Ŀ���� ���ɿ� �ǿ����� ��ġ�Ƿ�
WorkInfo ���� ������ ����ü�� ����ؾ� �Ѵ�.
WorkerID, WorkType �� ������ ������, ���� �ڵ鷯���� ���ǹǷ�
���� Block �� ������ �����ϱ� �����̴�.
*/

#include <ace/Message_Block.h>
#include <ace/Event_Handler.h>

enum WorkerID
{
	WID_PAS,
	WID_CP,
	WID_CLIENT,
	WID_SANTA,
	WID_AUTH
};

enum WorkType
{
	WT_ONRECEIVE,
	WT_ONSENDABLE,
	WT_DELETE,
	WT_TIMEOUT
};

class WorkInfo
{
public:
	WorkerID workerID;
	WorkType workType;
	ACE_Event_Handler* pEventHandler;

	WorkInfo() {}
	WorkInfo(WorkerID iWorkerID, WorkType iWorkType, ACE_Event_Handler* ipEventHandler)
	{
		this->workerID = iWorkerID;
		this->workType = iWorkType;
		this->pEventHandler = ipEventHandler;
	}
};

class WorkInfoMessageBlock :
	public ACE_Message_Block
{
public:
	WorkInfoMessageBlock(WorkerID workerID, WorkType workType, ACE_Event_Handler* pEventHandler);
	virtual ~WorkInfoMessageBlock(void);

	WorkInfo workInfo();
	void workInfo(const WorkInfo& info);
};

#endif
