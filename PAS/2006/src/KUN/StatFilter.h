/**
@file newSTATErrFilter.h

@brief ������ STATErrFileter �ҽ��� ��ü

@author KTF
@date 2006.05.26
*/

#if !defined(STATFILTER_H)
#define STATFILTER_H

/*
#include "Thread_Mutex.h"
#include "simpleAlloc.h"
#include "simpleHash.h"

*/

#include "Common.h"
#include "MyLog.h"

#include <string>
#include <map>

/* �Ʒ� �� ������ session.cpp �� ���ǵ� �� */
#define FILTER_BY_URL           1
#define FILTER_BY_DOMAIN        2
#define FILTER_BY_MDN       3

#define MAX_NOTIMESG    (20)

#ifndef BETWEEN
#define BETWEEN(X, A, B) ((A)<=(X) && (X) <= (B))
#endif

using namespace std;

struct FilterInfo
{
	string key;	// Domain/URL/MDN �߿� �ϳ�
	int msgid;	// ���� �޽��� ��ȣ
	int keytype;	// Domain/URL/MDN �߿� �ϳ�
};

class StatFilter
{

	public:
		StatFilter();
		~StatFilter();
		
		bool isBlocked(int iType , char *keystr, int iport,  char* notimesg);

		int load(char *fpath);
		void prn();
		void test_normalizeUrl();
		void test_parseLine();
		void test();
		void	clear();

		int	count()
		{
			return itemCount;
		}
		
	private:

		map<string, int> urlList;
		map<string, int> domainList;
		map<intMDN_t, int> mdnList;
		string notiMesgs[MAX_NOTIMESG+1];

		int	itemCount;
		int parseLine(char *oneline);
		int normalizeUrl(char *org, int iType, int portNum, char *normalized,  int maxlen);
		MyLog	*tracelog;

};
#endif

