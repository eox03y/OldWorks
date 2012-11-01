/**
@author handol@gmail.com �Ѵ��� 011-430-0258
@date 2004.11.30
@date 2006.9.12
*/

#ifndef NEW_MyLog_H
#define	NEW_MyLog_H

/**
@file MyLog.h

@brief SANTA, AUTH ���� �α� ó��

ACE framework �� Log ����� �䳻�� ���̴�.

������ ACE LOG �� �䳻�� Log_Msg.cpp ��ݿ��� ������ ���̴�.
static ���� ���� �ʰ�, ���� instance �� �����ϵ��� �Ͽ��� �Ѵ�.

phoneTrace ��� � ����ϱ� �����̴�.

PAS2006

== ���� ==
 1.  
     * MyLog::logprint() �� ���� ȣ������ �ʰ� MACRO (MY_DEBUG, MY_LOG_MSG, MY_HEX_DUMP) �� �̿��Ѵ�.
     * �⺻������ ���ξտ� thread ID, �ð� ���� ���� ��µȴ�.
     * overwrite() �� ȣ���ϸ� ���� �α׸� �����.
     * ���ξտ� thread ID, �ð� ������ ����� disable �ҷ��� set_head() ȣ��.
     
 2. main.cpp ���� ��� �Ƿ�

 main() {
	home = getenv("MASERVER_HOME");

	MY_LOG_MSG->open(home, "msger.log"); // msger.log ��� �̸��ڿ� MMDD(����)�� ���� �̸����� ȭ�� ����.

	MY_LOG_MSG->overwrite(); // ���� ȭ�� �����.
	
	MY_DEBUG ((LM_INFO, "\n==== %s BEGIN ====\n", argv[0]));
	MY_HEX_DUMP((LM_DEBUG, recvBuf, 64));
 }
 
 
 3. main.cpp ���� ���� ����

   * step 1: �α� ȭ���� open�Ѵ�.  home ���丮�� ������ �� �ִ�.
		 home ���丮�� �־�����, home/log/xxxx.0316 �̶�� ������ �����ȴ�.
		 
   * step 2: MY_DEBUG(( )); MY_HEX_DUMP(( )); ���� �׳� ����ϸ� �ȴ�.

   
   * ���ξտ� thread ID, �ð� ������ ����� disable �ҷ��� set_head() ȣ��.
   {
        MY_LOG_MSG->set_head(0);
	MY_DEBUG(( .... ));
	MY_LOG_MSG->set_head(1);
 }
   
   
 
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <time.h>

#include <ace/Thread_Mutex.h>

#include "ActiveObjectChecker.h"
#include "MyFile.h"

#ifndef byte
typedef unsigned char byte;
#endif
#ifndef MIN
#define MIN(A, B) ((A)<(B))?(A):(B)
#endif

	
#define PHTR_DEBUG(X)			if (tracelog) tracelog->logprint(LVL_DEBUG,X " \n")
#define PHTR_DEBUG1(X, Y)		if (tracelog) tracelog->logprint(LVL_DEBUG,X " \n",Y)
#define PHTR_DEBUG2(X, Y, Z)	if (tracelog) tracelog->logprint(LVL_DEBUG,X " \n",Y,Z)
#define PHTR_DEBUG3(X, Y, Z, A)	if (tracelog) tracelog->logprint(LVL_DEBUG,X " \n",Y,Z,A)
#define PHTR_DEBUG4(X, Y, Z, A, B)	if (tracelog) tracelog->logprint(LVL_DEBUG,X " \n",Y,Z,A, B)

#define PHTR_INFO(X)			if (tracelog) tracelog->logprint(LVL_INFO,X " \n")
#define PHTR_INFO1(X,Y)			if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y)
#define PHTR_INFO2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y,Z)
#define PHTR_INFO3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y,Z,A)
#define PHTR_INFO4(X,Y,Z,A, B)		if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y,Z,A, B)

#define PHTR_NOTICE(X)			if (tracelog) tracelog->logprint(LVL_INFO,X " \n")
#define PHTR_NOTICE1(X,Y)			if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y)
#define PHTR_NOTICE2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y,Z)
#define PHTR_NOTICE3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_INFO,X " \n",Y,Z,A)

#define PHTR_WARN(X)			if (tracelog) tracelog->logprint(LVL_WARN,X " \n")
#define PHTR_WARN1(X,Y)			if (tracelog) tracelog->logprint(LVL_WARN,X " \n",Y)
#define PHTR_WARN2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_WARN,X " \n",Y,Z)
#define PHTR_WARN3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_WARN,X " \n",Y,Z,A)
#define PHTR_WARN4(X,Y,Z,A, B)		if (tracelog) tracelog->logprint(LVL_WARN,X " \n",Y,Z,A, B)

#define PHTR_ERROR(X)			if (tracelog) tracelog->logprint(LVL_ERROR,X " \n")
#define PHTR_ERROR1(X,Y)			if (tracelog) tracelog->logprint(LVL_ERROR,X " \n",Y)
#define PHTR_ERROR2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_ERROR,X " \n",Y,Z)
#define PHTR_ERROR3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_ERROR,X " \n",Y,Z,A)

#define PHTR_HEXDUMP(X, LEN, MSG) if (tracelog) tracelog->hexdump(LVL_DEBUG,X, LEN, MSG)




#define MY_DEBUG(X)			logprint(LVL_DEBUG,X "\n")
#define MY_DEBUG1(X, Y)		logprint(LVL_DEBUG,X "\n",Y)
#define MY_DEBUG2(X, Y, Z)	logprint(LVL_DEBUG,X "\n",Y,Z)
#define MY_DEBUG3(X, Y, Z, A)	logprint(LVL_DEBUG,X "\n",Y,Z,A)
#define MY_DEBUG4(X, Y, Z, A, B)	logprint(LVL_DEBUG,X "\n",Y,Z,A, B)

#define MY_INFO(X)			logprint(LVL_INFO,X "\n")
#define MY_INFO1(X,Y)			logprint(LVL_INFO,X "\n",Y)
#define MY_INFO2(X,Y,Z)		logprint(LVL_INFO,X "\n",Y,Z)
#define MY_INFO3(X,Y,Z,A)		logprint(LVL_INFO,X "\n",Y,Z,A)


#define MY_NOTICE(X)			logprint(LVL_INFO,X "\n")
#define MY_NOTICE1(X,Y)			logprint(LVL_INFO,X "\n",Y)
#define MY_NOTICE2(X,Y,Z)		logprint(LVL_INFO,X "\n",Y,Z)
#define MY_NOTICE3(X,Y,Z,A)		logprint(LVL_INFO,X "\n",Y,Z,A)

#define MY_WARN(X)			logprint(LVL_WARN,X "\n")
#define MY_WARN1(X,Y)			logprint(LVL_WARN,X "\n",Y)
#define MY_WARN2(X,Y,Z)		logprint(LVL_WARN,X "\n",Y,Z)
#define MY_WARN3(X,Y,Z,A)		logprint(LVL_WARN,X "\n",Y,Z,A)

#define MY_ERROR(X)			logprint(LVL_ERROR,X "\n")
#define MY_ERROR1(X,Y)			logprint(LVL_ERROR,X "\n",Y)
#define MY_ERROR2(X,Y,Z)		logprint(LVL_ERROR,X "\n",Y,Z)
#define MY_ERROR3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_ERROR,X "\n",Y,Z,A)

enum MY_Log_Priority
{
	LVL_TRACE = 01,
	LVL_DEBUG = 02,
	LVL_INFO = 04,
	LVL_WARN = 010,
	LVL_ERROR = 020
};



class MyLog : public ActiveObjectChecker
{

public:
	MyLog();
	virtual ~MyLog();


	int open(const char *dir_name, const char *module_name);
	int openWithYear(const char *dir_name, const char *module_name);

	int overwrite();
	
	void	set_priority(int pri) {
		priority = pri;
	}
	void	set_time_print(int on_off) {
		timeprint = on_off;
	}
	void	set_thrid_print(int on_off) {
		thridprint =on_off;
	}

	void set_head(int on_off) {
		timeprint = on_off;
		thridprint =on_off;
	}

	int mkdir_ifnot(char *fname);

	void printHttpMesg(const char *mesg, char *src, int size, int flagPrintBody=0 );
	
	void logprint(int pri, const char *fmt, ...);
	void hexdump(int pri, char *src, int size, const char *mesg=NULL);

	int	close(void);
  
private:
	 MyLog * _instance;
	 
	int	priority;
	int	mask;
	int	timeprint;
	int	thridprint;

	char dirName[1024];
	char moduleName[256];
	char logPath[1024];
	MyFile logfd;
	int old_day;
	bool	withYear;

	int	logopen(void);
	char *getLogPath();
	void _goodhex(byte *ptr, int size, int maxsize);
	void _goodhex2(byte *ptr, int size, int maxsize);

};
#endif

