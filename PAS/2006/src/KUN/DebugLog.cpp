#include <iostream>
#include <fstream>

#include <ace/Log_Msg.h>
#include "Config.h"

using namespace std;

ACE_OSTREAM_TYPE* pLogFile = NULL;
int	lastday = 0;
int	lastminute = 0;

bool checkDayChange(char *nowtime)
{
	time_t t_val;
	struct tm t;
	
	time(&t_val);
	localtime_r(&t_val, &t);

	if (lastday )
		lastday = 0;
	
	#ifdef TEST_DEBUGLOG

	if (lastday != t.tm_mday  || lastminute !=  t.tm_min )
	
	#else
	
	if (lastday != t.tm_mday)
	
	#endif
	
	{
		snprintf(nowtime, 30, "%02d%02d",  t.tm_mon+1, 	t.tm_mday);
		nowtime[30] = 0;
		
		lastday = t.tm_mday;
		lastminute = t.tm_min;
		return true;
	}
	else
		return false;
}

int	openMainLogFile()
{
	char today[32];
	char filename[128];
	const char *fname = NULL;

	if(Config::instance()->log.outputType != LOT_FILE)
		return 0;
		
	if (checkDayChange(today)==false && pLogFile!=NULL)
	{
		return 0;
	}


	// ���ο� �α� ���� ����
	{
		if (Config::instance()->log.filename.isEmpty())
		{
			fname = "debug";
		}
		else
		{
			fname = Config::instance()->log.filename.toStr();
		}

		snprintf(filename, sizeof(filename),  "%s-%d.%s.log", fname, Config::instance()->network.listenPort, today);




		ACE_LOG_MSG->acquire();

		ACE_OSTREAM_TYPE* oldLogFile = pLogFile;

		pLogFile = new std::ofstream(filename, ios_base::out | ios_base::app);


		ACE_LOG_MSG->msg_ostream(NULL, 0);
		ACE_LOG_MSG->msg_ostream(pLogFile, 0);
		PAS_NOTICE2("LOG file change because day changed >> new fname=%s pLogFile=%X", filename,  pLogFile);
		ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
		ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR);

		ACE_LOG_MSG->release();


		usleep(1000);

		if (oldLogFile != NULL)
		{
			//oldLogFile->flush();
			//ACE_LOG_MSG->msg_ostream(NULL, 0);
			//delete oldLogFile;
			//oldLogFile = NULL;
		}

	}

	return 1;
}

