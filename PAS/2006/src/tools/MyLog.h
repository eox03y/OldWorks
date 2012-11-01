/**

������ ACE LOG �� �䳻�� Log_Msg.cpp ��ݿ��� ������ ���̴�.
static ���� ���� �ʰ�, ���� instance �� �����ϵ��� �Ͽ��� �Ѵ�.

phoneTrace ��� � ����ϱ� �����̴�.

PAS2006
@author handol@gmail.com �Ѵ��� 011-430-0258
@date 2004.11.30
@date 2006.9.12
*/

#ifndef MyLog_H
#define	MyLog_H

/**
@file MyLog.h

ACE framework �� Log ����� �䳻�� ���̴�.

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


#ifndef byte
typedef unsigned char byte;
#endif
#ifndef MIN
#define MIN(A, B) ((A)<(B))?(A):(B)
#endif

//#define PHTR_TRACE(X)	
	
#define PHTR_DEBUG(X)			if (tracelog) tracelog->logprint(LVL_DEBUG,X " [%X]\n", this)
#define PHTR_DEBUG1(X, Y)		if (tracelog) tracelog->logprint(LVL_DEBUG,X " [%X]\n",Y, this)
#define PHTR_DEBUG2(X, Y, Z)	if (tracelog) tracelog->logprint(LVL_DEBUG,X " [%X]\n",Y,Z, this)
#define PHTR_DEBUG3(X, Y, Z, A)	if (tracelog) tracelog->logprint(LVL_DEBUG,X " [%X]\n",Y,Z,A, this)
#define PHTR_DEBUG4(X, Y, Z, A, B)	if (tracelog) tracelog->logprint(LVL_DEBUG,X " [%X]\n",Y,Z,A, B, this)

#define PHTR_INFO(X)			if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n", this)
#define PHTR_INFO1(X,Y)			if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n",Y, this)
#define PHTR_INFO2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n",Y,Z, this)
#define PHTR_INFO3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n",Y,Z,A, this)


#define PHTR_NOTICE(X)			if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n", this)
#define PHTR_NOTICE1(X,Y)			if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n",Y, this)
#define PHTR_NOTICE2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n",Y,Z, this)
#define PHTR_NOTICE3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_INFO,X " [%X]\n",Y,Z,A, this)

#define PHTR_WARN(X)			if (tracelog) tracelog->logprint(LVL_WARN,X " [%X]\n", this)
#define PHTR_WARN1(X,Y)			if (tracelog) tracelog->logprint(LVL_WARN,X " [%X]\n",Y, this)
#define PHTR_WARN2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_WARN,X " [%X]\n",Y,Z, this)
#define PHTR_WARN3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_WARN,X " [%X]\n",Y,Z,A, this)

#define PHTR_ERROR(X)			if (tracelog) tracelog->logprint(LVL_ERROR,X " [%X]\n", this)
#define PHTR_ERROR1(X,Y)			if (tracelog) tracelog->logprint(LVL_ERROR,X " [%X]\n",Y, this)
#define PHTR_ERROR2(X,Y,Z)		if (tracelog) tracelog->logprint(LVL_ERROR,X " [%X]\n",Y,Z, this)
#define PHTR_ERROR3(X,Y,Z,A)		if (tracelog) tracelog->logprint(LVL_ERROR,X " [%X]\n",Y,Z,A, this)

#define PHTR_HEXDUMP(X, LEN) if (tracelog) tracelog->hexdump(LVL_DEBUG,X, LEN)


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

class MyLog {

public:
	MyLog();
	~MyLog();


	int open(char *dir_name, char *module_name);

	char *getLogPath();

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

	void	set_stderr(void);
	void	set_stdout(void);
	int mkdir_ifnot(char *fname);

	void logprint(int pri, const char *fmt, ...);
	void hexdump(int pri, char *src, int size, const char *mesg=NULL);

	void	overwrite();
	int	close(void);
  
private:
	 MyLog * _instance;
	 
	int	priority;
	int	mask;
	int	timeprint;
	int	thridprint;
	int	tostd; // bool: stderr, stdout ������ ���.

	char dirName[64];
	char moduleName[32];
	char logPath[256];
	FILE *logfd;
	int old_day;

	struct tm *datetm;

	void	logopen(void);
	void	_goodhex(byte *ptr, int size, int maxsize);
		
};
#endif

