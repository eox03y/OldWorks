#ifndef __CP_LIST_H__
#define __CP_LIST_H__

/**
@brief CP ��� ����

�� Ŭ������ ����Ե� ������
����ϰ� CP �� �����ϰ� �����ν� ����/���� �� ���� ������尡 �߻��ϰ� �Ǿ�
CP ���� Ŀ�ؼ��� �����Ͽ� ������带 ���̰� �����ӵ��� ���̱� ���ؼ� �������.

������, CP�� WEB �����̰� Ÿ�Ӿƿ��� �����ϹǷ�, ����ŭ ��������� ���� ����.
����(2006.11.15)�� config���� ReuseCpConnection ������ on �� ���� ����Ѵ�. 
*/

#include "Common.h"
#include "CpEventHandler.h"
#include <vector>
#include "HttpTypes.h"
#include <ace/Thread_Mutex.h>

typedef std::vector<CpHandler*> CpPtrs;

class CpList
{
// ��� �Լ�
public:
	int add(CpHandler* pCP);
	int addIfNot(CpHandler* pCP);
	CpHandler* get(const host_t host, const int port);
	int del(CpHandler* pCP);
	int deleteAll();
	int size();

	bool isExist(const host_t host, const int port);
	bool isExist(const CpHandler* pCP);

	/// cpList �� ��ϵ� ��� CP���� ���� ���� ��û�� �Ѵ�.
	void requestCloseToAll();
	
private:

// ��� ����
private:
	CpPtrs cpList;
};

#endif
