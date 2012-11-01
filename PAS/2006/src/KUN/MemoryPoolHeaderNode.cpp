#include "MemoryPoolHeaderNode.h"

MemoryPoolHeaderNode::MemoryPoolHeaderNode(int maxFreeBlockCount_)
{
	blockSize = 0;
	allocCount = 0;
	setMaxFreeBlockCount(maxFreeBlockCount_); // 0 �� ������
}

MemoryPoolHeaderNode::~MemoryPoolHeaderNode()
{
	_clearBlocks();
}

ACE_Message_Block* MemoryPoolHeaderNode::alloc(int *resultFlag)
{
	WriteLock writeLock(lock); 

	allocCount++;
	// free block �� ������ ���� �����ؼ� ��ȯ
	if(ptrs.empty())
	{
		*resultFlag = 0;
		return new ACE_Message_Block(blockSize);
	}

	// free block �� ������ �ش� block�� ��ȯ
	else
	{
		*resultFlag = 1;
		ACE_Message_Block* pMB = ptrs.back();
		ptrs.pop_back();
		return pMB;
	}
}

void MemoryPoolHeaderNode::free(ACE_Message_Block* pMB, int *resultFlag)
{
	ACE_ASSERT(pMB != NULL);

	WriteLock writeLock(lock);

	allocCount--;
	
	// �����Ǵ� blockSize �� �ƴϸ� �׳� ����
	if(blockSize != (int)pMB->size())
	{
		*resultFlag = 0;
		delete pMB;
	}

	// �����Ǵ� blockSize �̸� free list �� �߰�
	else
	{
		// reach maxFreeBlockCount
		if(static_cast<int>(ptrs.size()) >= maxFreeBlockCount)
		{
			*resultFlag = 0;
			delete pMB;
		}

		// under maxFreeBlockCount
		else
		{
			*resultFlag = 1;
			pMB->reset();
			ptrs.push_back(pMB);
		}	
	}
}

void MemoryPoolHeaderNode::setMaxFreeBlockCount(int count)
{
	WriteLock writeLock(lock);

	maxFreeBlockCount = count;

	// remove over blocks
	while(static_cast<int>(ptrs.size()) > count)
	{
		delete ptrs.back();
		ptrs.pop_back();
	}
}

int MemoryPoolHeaderNode::getMaxFreeBlockCount()
{
	ReadLock readLock(lock);

	return maxFreeBlockCount;
}

void MemoryPoolHeaderNode::setBlockSize(int size)
{
	WriteLock writeLock(lock);

	if(blockSize == size)
		return;

	blockSize = size;
	_clearBlocks();
}

int MemoryPoolHeaderNode::getBlockSize()
{
	ReadLock readLock(lock);

	return blockSize;
}

void MemoryPoolHeaderNode::clearBlocks()
{
	WriteLock writeLock(lock);
	_clearBlocks();
}

// no mutex version of clearBlocks()
void MemoryPoolHeaderNode::_clearBlocks()
{
	while(!ptrs.empty())
	{
		ACE_Message_Block* pMB = ptrs.back();
		delete pMB;
		ptrs.pop_back();
	}
}

int MemoryPoolHeaderNode::getFreeBlockCount()
{
	ReadLock readLock(lock);
	
	return (int)ptrs.size();
}

int	MemoryPoolHeaderNode::getAllocBlockCount()
{
	return allocCount;
}

