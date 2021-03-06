/*--------------------------------------------------------
  PROGRAM NAME : mwatchcli.cpp
  DATE         : 2002.04.01
  AUTHOR       : LEE SANG GU
---------------------------------------------------------*/

#include "CommonSocket.h"
#include "mwatchcli.h"
#include <stdlib.h>
//#include <stream.h>
#include <stdio.h>

#include <iostream>

using namespace std;

/*--------------------------------------------------------------------
  함 수 명 : mwatchcli::mwatchcli()
  기능개요 : 소켓을 헤더파일에 정의해 놓은 호스트의 주소와 포트번호
			 초기화 시키는 함수를 호출한�
  인    수 : 없�
  리 턴 값 : 없�
--------------------------------------------------------------------*/
mwatchcli::mwatchcli()
{
	char msgWatchAddr[128] = "\0";
	strcpy(msgWatchAddr, MSGWATCH_ADDR);
	Initialize( msgWatchAddr, MSGWATCH_PORT );
}


/*--------------------------------------------------------------------
  함 수 명 : mwatchcli::mwatchcli( char *host )
  기능개요 : 호스트의 포트번호는 헤더파일에 정의해놓은 값을 가져오�
			 주소는 따로 입력받아 초기화 시키는 함수를 호출한�
  인    수 : char *host -> 호스트의 주소
  리 턴 값 : 없�
--------------------------------------------------------------------*/
mwatchcli::mwatchcli( char *host )
{
	Initialize( host, MSGWATCH_PORT );
}


/*-----------------------------------------------------------------------
  함 수 명 : mwatchcli::mwatchcli( char *host, int port )
  기능개요 : 호스트의 주소와 포트번호를 입력받아 소켓을 초기화시키는
			 함수를 호출한�
  인    수 : char *host -> 호스트의 주소,  int port -> 호스트의포트번호
  리 턴 값 : 없�
-----------------------------------------------------------------------*/
mwatchcli::mwatchcli( char *host, int port )
{
	Initialize( host, port );
}


/*--------------------------------------------------------------------
  함 수 명 : mwatchcli::~mwatchcli()
  기능개요 : 소켓을 닫는�
  인    수 : 없�
  리 턴 값 : 없�
--------------------------------------------------------------------*/
mwatchcli::~mwatchcli()
{
	cltSock.Close();
}


/*--------------------------------------------------------------------
  함 수 명 : void mwatchcli::Initialize( char *host, int port )
  기능개요 : 호스트주소와 포트번호를 입력받아 UDP소켓을 생성하�
			 소켓에 호스트주소와 포트번호를 바인드 시킨�
  인    수 : char *host -> 호스트명,   int port -> 포트번호
  리 턴 값 : 없�
--------------------------------------------------------------------*/
void mwatchcli::Initialize( char *host, int port )
{
	index = 0;
	cltSock.Create( UDP );
	cltSock.Bind( port, host );

	memset( WatchMsg.ServerID, 0x00, sizeof( WatchMsg.ServerID ) );
	gethostname( WatchMsg.ServerID, sizeof( WatchMsg.ServerID ) );

	cout << " WatchMsg.ServerID => " << WatchMsg.ServerID << endl;
}


/*--------------------------------------------------------------------
  함 수 명 : BOOL mwatchcli::addID( char * szID )
  기능개요 : 서비스아이디와 메시지건수를 추가하는 함수를 호출한�
  인    수 : char * szID -> 서비스아이디
  리 턴 값 : 없�
--------------------------------------------------------------------*/
BOOL mwatchcli::addID( char * szID )
{
	return addID( szID, 1 );
}


/*----------------------------------------------------------------------
  함 수 명 : BOOL mwatchcli::addID( char * szID, int m_nMsgCnt )
  기능개요 : 서비스아이디를 추가하한�
  인    수 : char * szID -> 서비스아이디,   int m_nMsgCnt -> 메시지건수
  리 턴 값 : 없�
-----------------------------------------------------------------------*/
BOOL mwatchcli::addID( char * szID, int m_nMsgCnt )
{
	if ( index < MAX_ID_NUM ) {
		WatchIndex[index] = ( watch_index_t * ) malloc( sizeof(watch_index_t) );

		WatchIndex[index]->cnt = 0;
		// WatchIndex[index]->overflow = m_nMsgCnt;
		WatchIndex[index]->lastsent = time(NULL) - MIN_SEND_TIME;

		if( szID != NULL )
			strcpy( WatchIndex[index]->id, szID );

		index += 1;

		return TRUE;
	}
	return FALSE;
}


/*--------------------------------------------------------------------
  함 수 명 : BOOL mwatchcli::addMsgCnt( char * szID )
  기능개요 : 서비스아이디와 메시지건수를 추가는 함수를 호출한�
  인    수 : char * szID -> 서비스아이디
  리 턴 값 : 없�
--------------------------------------------------------------------*/
BOOL mwatchcli::addMsgCnt( char * szID )
{
	return addMsgCnt( szID, 1 );
}


/*---------------------------------------------------------------------
  함 수 명 : BOOL mwatchcli::addMsgCnt( char * szID, int m_nMsgCnt )
  기능개요 : 호스트명, 서비스아이디, 메시지건수를 UDP서버로 전송한�
  인    수 : char * szID -> 서비스아이디,  int m_nMsgCnt -> 메시지건수
  리 턴 값 : 없�
---------------------------------------------------------------------*/
BOOL mwatchcli::addMsgCnt( char * szID, int m_nMsgCnt )
{
	int  m_nCnt;
	int  i;
	char sSendMsg[256];

	for( m_nCnt = 0; m_nCnt < index; m_nCnt++ ) {
		if( !strcmp( WatchIndex[ m_nCnt ]->id, szID ) )
			break;
	}

	if( m_nCnt==index ) {
		addID( szID, m_nMsgCnt );
	}

	WatchIndex[m_nCnt]->cnt += 1;
	time_t thistime = time( NULL );

	if ( ( thistime - WatchIndex[m_nCnt]->lastsent) >= MIN_SEND_TIME ) {
		strcpy(WatchMsg.ServiceID, szID);
		WatchMsg.MessageCount= WatchIndex[m_nCnt]->cnt;

		char szTmpBuf[1024];
		snprintf( szTmpBuf, sizeof(szTmpBuf)-1," SEND [%s : %d]번째..",
			szID, WatchIndex[m_nCnt]->cnt );

		memset( sSendMsg, 0x00, sizeof( sSendMsg ) );

		snprintf( sSendMsg, sizeof(szTmpBuf)-1, "%s:%s:%04d", WatchMsg.ServerID
			, WatchMsg.ServiceID,WatchMsg.MessageCount );

		if ( cltSock.Sendto( sSendMsg, sizeof( sSendMsg ) ) < 0 ) {
			return FALSE;
		}
		WatchIndex[m_nCnt]->cnt = 0;
		WatchIndex[m_nCnt]->lastsent = thistime;
		return TRUE;
	}
	else {
		char szTmpBuf[1024];
		sprintf( szTmpBuf, " NOT YET [%s : %d]번째.. ",
			szID, WatchIndex[m_nCnt]->cnt );

		return NOT_YET;
	}
}

#ifdef TEST_MAIN
int main(int argc, char *argv[])
{
	mwatchcli Util2::msgwatch;
	char 	Util2::mwatchMsg[256]={0};

	char hostname[64];
	gethostname(hostname, sizeof(hostname)-1);
	sprintf(mwatchMsg, "pasgw_%d(%s)", svrport, hostname);

}
#endif

