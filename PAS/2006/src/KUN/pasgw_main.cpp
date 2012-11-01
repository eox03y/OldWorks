#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <strings.h>


#include <ace/Log_Msg.h>
#include <ace/Reactor.h>
#include <ace/Select_Reactor.h>
#include <ace/streams.h>
#include <ace/Thread_Manager.h>
#include <ace/Task_T.h> 
#include <ace/Message_Queue_T.h>
#include <ace/TP_Reactor.h>
#include <ace/Dev_Poll_Reactor.h>

#include "Common.h"
#include "PasSignalHandler.h"
#include "PasAcceptor.h"
#include "PasLog.h"
#include "AclRouter.h"
#include "LogLevel.h"
#include "Config.h"
#include "MemoryPoolManager.h"
#include "ReactorPool.h"

#include "UserInfoMng.h"
#include "PhoneTraceMng.h"
#include "StatFilterMng.h"
#include "Util2.h"
#include "SysInfo.h"
#include "PasDataLog.h"
#include "StrSplit.h"
#include "HashKey.h"
#include "CGI.h"
#include "DNSManager.h"

#include "HttpRequest.h"
#include "HttpResponse.h"

#include "AuthAgent.h"

#include "DebugLog.h"

#include "MonitorReporter.h"
#include "WatchReporter.h"
//---------------------------------------------------------------------------

/* 
Sun Compiler ���� :  

http://predef.sourceforge.net/precomp.html#sec43  -- ��� ��¥.


http://www.cs.wfu.edu/docs/CC.html 
http://predef.sourceforge.net/
*/
/*
GCC predefined macros
http://gcc.gnu.org/onlinedocs/gcc-3.4.0/cpp/Common-Predefined-Macros.html#Common%20Predefined%20Macros
*/
//static const char* version = __VERSION__;
static const char* compile_date = "PAS2006_DATE: " __DATE__ " " __TIME__;

#ifdef __SUNPRO_CC
static const char* compiler = "PAS2006_COMPILER: "  XSTR (__SUNPRO_CC);
#endif

extern char *optarg;
extern int optind, opterr, optopt;

const char* defaultConfigFilename = "9090.cfg";

//---------------------------------------------------------------------------
void initSignal(PasSignalHandler* pSignalHandler);
void initACL();
void stopACL();
int initConfig(const char * confname);
ACE_Configuration_Heap* createAndOpenConfig(const char* filename);
int makeDaemon();
int initLog();
int initMessageBlockManager();
void helpMsg(const char* programName);
void increseFdLimit();
int readMBconfig(void);
void procArguments(int argc, char** argv);

//---------------------------------------------------------------------------
int ACE_MAIN(int argc, ACE_TCHAR  *argv[])
{
	// ȯ�漳�� (���� ���� �ʱ�ȭ �ؾ���)
	procArguments(argc, argv);

	// �α�
	if(initLog() < 0)
	{
		printf("�α� �ʱ�ȭ�� ���������Ƿ� ���α׷��� �����մϴ�.\n");
		return -1;
	}

	//-----------------------------------------------------------------------
	// ���� ����� 
	//-----------------------------------------------------------------------
	// acceptor �� �ʱ�ȭ �� �� ������ ����� �Ǹ�, �θ����μ����� ���ŵǸ鼭
	// listen ���ϵ� ���� ���ŵȴ�. �׷��Ƿ� acceptor �� �ʱ�ȭ �ϱ� ����
	// ������ ���� ������ �Ѵ�.
	if(Config::instance()->process.daemon == true)
	{
		if(makeDaemon() < 0)
		{
			printf("�������� ����⸦ ���������Ƿ� ���α׷��� �����մϴ�.\n");
			return -1;
		}
	}
	
	PAS_NOTICE1("%s", compile_date);

	// �ñ׳� �ڵ鷯
	PasSignalHandler* pSignalHandler = new PasSignalHandler;
	initSignal(pSignalHandler);

	//------------------
	// �� ����� �ʱ�ȭ
	//------------------
	HTTP::Request::HeaderBuffBytes = HTTP::Response::HeaderBuffBytes = Config::instance()->process.HttpHeaderBufferBytes;
	
	increseFdLimit();

	
	// PAS ���� �α�
	char hostName[32];

	gethostname(hostName, sizeof(hostName)-1);
	PasDataLog::setPasAddr(hostName);
	
	// �޸� �Ŵ��� �ʱ�ȭ
	//initMessageBlockManager();
	if (readMBconfig() < 0)
	{
		printf("�޸� Ǯ ���Ǳ� �������� Ȯ���ϼ���.\n");
		PAS_ERROR("�޸� Ǯ ���Ǳ� �������� Ȯ���ϼ���.\n");
		return -1;
	}

	const int numWorkerThread = Config::instance()->thread.numWorker;

	// ������
	ACE_Reactor* pReactor = ReactorPool::instance()->createMaster();
	
	ReactorPool::instance()->createWorkers(numWorkerThread);

	ACE_Reactor* pGlobalReactor = ACE_Reactor::instance();

	PAS_NOTICE3("REACTOR: Master=%X, Global=%X, Master=%x", 
		pReactor,  pGlobalReactor, ReactorPool::instance()->masterReactor());

	// Listen ��Ʈ ����
	PAS_NOTICE("Listen Socket Activate");
	PasAcceptor* pAcceptor = new PasAcceptor(pReactor);
	
	const int listenPort = Config::instance()->network.listenPort;

	// mIDC ���� ������ �����ϴ� �ʿ� mwatch �����ϱ� ���� �޽��� �ۼ��Ѵ�.
	Util2::setMwatchMsg(listenPort);

	// acceptor �⵿ 
	if(pAcceptor->open(listenPort) < 0)
	{
		PAS_ERROR1("Listen ���� ���� ����, Port[%d]\n", listenPort);
		PAS_ERROR("���α׷� ����\n");

		printf("Listen ���� ���� ����, Port[%d]\n", listenPort);
		printf("���α׷� ����\n");
		return -1;
	}

	// ������ �Ŵ���
	ACE_Thread_Manager* pTManager = ACE_Thread_Manager::instance();	

	// monitor ������ ���� thread �� �����Ѵ�.
	MonitorReporter *monitor = MonitorReporter::instance(pTManager);
	monitor->activate(THR_NEW_LWP | THR_JOINABLE);

	WatchReporter *watch = WatchReporter::instance(pTManager);
	watch->activate(THR_NEW_LWP | THR_JOINABLE);

	// UserInfo ������ ���� thread �� �����Ѵ�.
	UserInfoMng *userInfoMng = UserInfoMng::instance(pTManager);
	userInfoMng->activate(THR_NEW_LWP | THR_JOINABLE);
	
	// phone trace �� ���� thread �� �����Ѵ�.
	PhoneTraceMng *phoneTraceMng = PhoneTraceMng::instance(pTManager);
	phoneTraceMng->setDataFile((char*)"trace.acl");
	phoneTraceMng->activate(THR_NEW_LWP | THR_JOINABLE);

	// ���� ó�� (Stat Filter)  �� ���� thread �� �����Ѵ�.
	StatFilterMng *statFilterMng = StatFilterMng::instance(pTManager);
	statFilterMng->setDataFile((char*)"k_stat.cfg");
	statFilterMng->activate(THR_NEW_LWP | THR_JOINABLE);

	// ACL �ʱ�ȭ
	if(Config::instance()->acl.enable)
		initACL();

	CGI::cgiSetupConstants();
	
	// Create AuthAgent Thread
	AuthAgent *authAgent = AuthAgent::instance(pTManager);
	authAgent->activate(THR_NEW_LWP | THR_JOINABLE);

	//usleep(1000);
	
	// ���� ���� (sysinfo) ��¸� ���� Thread
	SysInfo  *sysInfo = SysInfo::instance(pTManager);
	sysInfo->activate(THR_NEW_LWP | THR_JOINABLE);

	// hash key �α� �ۼ��� ���� �ʱ�ȭ.
	HashKey::prepare();
	
	// �α� ȭ���� ���� ����� ���´�.  �׽�Ʈ�ÿ� ���ϴ�. (tail -f )
	PasDataLog::instance();
	
	// accept event �ڵ鷯 ���
	pReactor->register_handler(pAcceptor, ACE_Event_Handler::ACCEPT_MASK);

	// �̺�Ʈ ���Ƽ�÷���
	PAS_NOTICE("Master Reactor Start");
	pReactor->run_event_loop();
	PAS_NOTICE("Master Reactor Stop");

	ReactorPool::instance()->stopWorkers();


	/*--- Reactor �� ����� ��� �Ʒ� �������� ����ȴ�. ---*/

	stopACL(); // ACL ����
	userInfoMng->stop();
	monitor->stop();
	watch->stop();
	phoneTraceMng->putq(new ACE_Message_Block());
	statFilterMng->putq(new ACE_Message_Block());
	sysInfo->putq(new ACE_Message_Block());
	authAgent->putq(new ACE_Message_Block());

	DNS::Manager::instance()->removeAllQuerier();
	
	// ��� ������ ���� ���
	PAS_NOTICE("Waiting for all threads to stop");
	pTManager->wait();

	delete phoneTraceMng;
	delete statFilterMng;
	delete sysInfo;
	// ������ ���� ��ü ����
	delete pSignalHandler;
	
	PAS_NOTICE("======= PAS GW Stop =======");

	return 0;
}

//---------------------------------------------------------------------------
void procArguments(int argc, char** argv)
{
	const char* configFilename = NULL;

	int listenPort = -1;
	TinyString logLevel;

	bool debugMode = false;
	
	int c;
	while ((c = getopt(argc, argv, ":hdl:p:")) != -1) 
	{
		switch(c) 
		{
		case 'd':
			debugMode = true;
			break;

		case 'p':
			listenPort = atoi(optarg);
			break;

		case 'l':
			logLevel = optarg;
			break;

		case ':':
			printf("-%c is need more value\n", optopt);
			helpMsg(argv[0]);
			exit(0);
			break;

		case '?':
			if(optopt == '-')
			{
				if(strcmp(argv[optind-1], "--help") == 0)
				{
					helpMsg(argv[0]);
					exit(0);
				}
			}
			else
			{
				printf("Unknown arg %c\n", optopt);
				helpMsg(argv[0]);
				exit(0);
			}

		case 'h':
			helpMsg(argv[0]);
			exit(0);

		default:
			helpMsg(argv[0]);
			exit(0);
		}
	}

	if(optind < argc) 
	{
		configFilename = argv[optind];
		optind++;
	}

	if(configFilename == NULL)
		configFilename = defaultConfigFilename;

	if(initConfig(configFilename) < 0)
	{
		printf("���Ǳ� ������ ���� �� �������� �ٽ� Ȯ���� �ּ���.\n");
		exit(0);
	}

	if(debugMode)
	{
		Config::instance()->process.daemon = false;
		Config::instance()->log.outputType = LOT_STDOUT;
		Config::instance()->log.level = LLT_MEDIUM;
	}

	if(!logLevel.isEmpty())
	{
		if(logLevel == "HIGH" || logLevel=="4")
		{
			Config::instance()->log.level = LLT_HIGH;
		}
		else if(logLevel == "MEDIUM" || logLevel=="3")
		{
			Config::instance()->log.level = LLT_MEDIUM;
		}
		else if(logLevel == "LOW" || logLevel=="2")
		{
			Config::instance()->log.level = LLT_LOW;
		}
		else if(logLevel == "VERYLOW" || logLevel == "1")
		{
			Config::instance()->log.level = LLT_VERYLOW;
		}
	}

	if(listenPort > 0)
		Config::instance()->network.listenPort = listenPort;
}

//---------------------------------------------------------------------------
void helpMsg(const char* programName)
{
	printf("Usage : %s [-p listen_port] [-d] [-l log_level] [config]\n", programName);
	printf("            -d : Debug mode (No Daemon with MEDIUM log level)\n");
	printf("            -l : Log level [HIGH | MEDIUM | LOW | VERYLOW | 4 | 3 | 2 | 1]\n");
	printf("        config : Config file. default : %s\n", defaultConfigFilename);
}

//---------------------------------------------------------------------------
void increseFdLimit()
{
	struct rlimit limit;
	
	getrlimit(RLIMIT_NOFILE, &limit);
	PAS_NOTICE1("LIMIT: Current FD Limit: %d" ,limit.rlim_cur);
	
	limit.rlim_cur = limit.rlim_max;	
	//limit.rlim_cur = 65536;
	setrlimit(RLIMIT_NOFILE, &limit);

	getrlimit(RLIMIT_NOFILE, &limit);
	PAS_NOTICE1("LIMIT: New FD Limit: %d" ,limit.rlim_cur);

	getrlimit(RLIMIT_FSIZE, &limit);
	PAS_NOTICE1("LIMIT: Current FileSize Limit: %d" ,limit.rlim_cur);
	
	limit.rlim_cur = limit.rlim_max;	
	//limit.rlim_cur = (unsigned long)(2 << 30);
	setrlimit(RLIMIT_FSIZE, &limit);
	getrlimit(RLIMIT_FSIZE, &limit);
	PAS_NOTICE1("LIMIT: New FileSize Limit: %d" ,limit.rlim_cur);

	PAS_NOTICE1("FD_SETSIZE[%d]", FD_SETSIZE);
}
//---------------------------------------------------------------------------
void initSignal(PasSignalHandler* pSignalHandler)
{
	// �ñ׳� �ڵ鷯 ����
	ACE_Sig_Handler* pSigHandler = new ACE_Sig_Handler;
	
	// �ñ׳� �ڵ鸵
	pSigHandler->register_handler(SIGINT, pSignalHandler);
	pSigHandler->register_handler(SIGTERM, pSignalHandler);
	pSigHandler->register_handler(SIGQUIT, pSignalHandler);
	pSigHandler->register_handler(SIGUSR1, pSignalHandler);
	pSigHandler->register_handler(SIGUSR2, pSignalHandler);

	// �ñ׳� ����
	pSigHandler->register_handler(SIGPIPE, pSignalHandler);	
	pSigHandler->register_handler(SIGALRM, pSignalHandler);
	pSigHandler->register_handler(SIGHUP, pSignalHandler);	
	pSigHandler->register_handler(SIGCHLD, pSignalHandler);	
}
//---------------------------------------------------------------------------
int initConfig(const char * confname)
{
	Config* pConfig = Config::instance();

	if (pConfig->load(confname) < 0)
		return -1;

	return 0;
}
//---------------------------------------------------------------------------
void initACL()
{
	ACE_ASSERT(Config::instance()->acl.enable);
	const ProcessConfig& procConf = Config::instance()->process;
	const NetworkConfig& netConf = Config::instance()->network;

	AclRouter* pAcl = AclRouter::instance();
	
	if(pAcl->initial(procConf.serverID, netConf.listenPort) < 0)
	{
		PAS_ERROR("AclRouter �ʱ�ȭ ����");
		ACE_OS::exit();
	}

	pAcl->activate();
}
//---------------------------------------------------------------------------
void stopACL()
{
	AclRouter* pAcl = AclRouter::instance();
	pAcl->remove_worker_thread();
}
//---------------------------------------------------------------------------
int makeDaemon()
{
	int ret = fork();
	if(ret < 0)
	{
		printf("�������μ��� ���� ����\n");
		return -1;
	}
	
	// child
	if(ret == 0)
	{
		//pass
	}

	// parent
	else
	{
		// �θ�� ����
		exit(0);
	}

	return 0;
}

//---------------------------------------------------------------------------
int initLog()
{
	openMainLogFile();
	
	PAS_NOTICE("======= PAS GW Start =======");
	
	// �α� ���� ����
	const LogConfig& logConfig = Config::instance()->log;	
	LogLevel* logLevel = LogLevel::instance();
	logLevel->setLevel(logConfig.level);;
	
	return 0;
}

//---------------------------------------------------------------------------
int initMessageBlockManager(mapint &sizes)
{
	MessageBlockManager* pMBManager = MessageBlockManager::instance();

	BlockInfoList	blockInfoList;
	blockInfoList.reserve(sizes.size());
	BlockInfo oneblock;

	iterint iter = sizes.begin();
	for( ; iter != sizes.end(); iter++ )
	{
		oneblock.blockSize = iter->first;
		oneblock.maxNum = iter->second;
		blockInfoList.push_back( oneblock);
		PAS_INFO2( "Memory Pool: %6dK  %5d Blocks ",  (oneblock.blockSize >> 10) , oneblock.maxNum);
	}

	PAS_INFO1( "Memory Pool:  %d Pools", blockInfoList.size());
	pMBManager->setBlockSizeAndMax( blockInfoList );
	
	return 0;
}


//---------------------------------------------------------------------------
/**
@brief
�޸� ���� ũ��� ������ �о� initMessageBlockManager �� �Ѱ��ش�.
*/
int readMBconfig(void)
{
	filename_t fileName(Config::instance()->process.memconf);

	FILE *fp = NULL;
	fp = fopen( fileName, "rt" );
	if( fp == NULL )
	{
		PAS_ERROR1( "Memory Pool Config File:  [%s]  not found", fileName.toStr() );
		return -1;
	}

	// ���� ����ϴ� ������, Ű������ ��������, Value ������ �ִ� ����� ������ �ǰ�
	// ���� config ���Ͽ� �ߺ��� ��(��:16K, 16K)�� ���� ��� �޺κ��� �ڵ����� ���õǹǷ�
	// map �� ����ϴ� ���� ���� �����ϴٰ� �Ǵܵȴ�.
	mapint mapSizes;
	int	lineCount = 0;
	int	maxBlockSize =0;
	while( !feof(fp) )
	{
		char line[1024] = "\0";
		char *pResult = fgets( line, sizeof(line), fp );
		if( pResult == NULL )
			break;
		lineCount++;
		
		StrSplit MBSpliter( 2, sizeof(line) );
		MBSpliter.split( line );

		// skip comment
		if( MBSpliter.fldVal(0)[0] == '#' )
			continue;

		// �������� �����Ͱ� �ƴϸ� �н�
		if( MBSpliter.numFlds() != 2 )
		{
			PAS_ERROR2( "Memory Pool Config File: Error in line [%d], file[%s]", lineCount, fileName.toStr() );
			return -1;
		}

		char *pLeftItem = MBSpliter.fldVal( 0 );
		char *pRightItem = MBSpliter.fldVal( 1 );
		int nLeftLength = strlen( pLeftItem );

		int nBlockSize = 0;
		int nMaxCount = strtol( pRightItem, NULL, 10 );

		if( pLeftItem[nLeftLength-1] == 'K' || pLeftItem[nLeftLength-1] == 'k' )
		{
			pLeftItem[nLeftLength-1] = '\0';
			nBlockSize = strtol( pLeftItem, NULL, 10 );
			nBlockSize = nBlockSize * 1024;
		}

		else if( pLeftItem[nLeftLength-1] == 'M' || pLeftItem[nLeftLength-1] == 'm' )
		{
			pLeftItem[nLeftLength-1] = '\0';
			nBlockSize = strtol( pLeftItem, NULL, 10 );
			nBlockSize = nBlockSize * 1024 * 1024;
		}

		// �������� �� �� �Էµ� ��� �����Ѵ�.
		else
		{
			PAS_ERROR2( "Memory Pool Config File: Error in line [%d], file[%s]", lineCount, fileName.toStr());
			return -1;
		}

		if (nBlockSize < maxBlockSize)
		{
			PAS_ERROR2( "Memory Pool Config File: Block Size must be bigger than the previous size. Error in line [%d], file[%s]", lineCount, fileName.toStr());
			return -1;
			
		}

		maxBlockSize = nBlockSize;
		
		mapSizes.insert( make_pair(nBlockSize, nMaxCount) );
		
	}

	fclose( fp );

	initMessageBlockManager( mapSizes );

	return 0;
}
