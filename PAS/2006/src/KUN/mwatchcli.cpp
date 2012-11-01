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
  �� �� �� : mwatchcli::mwatchcli()
  ��ɰ��� : ������ ������Ͽ� ������ ���� ȣ��Ʈ�� �ּҿ� ��Ʈ��ȣ
			 �ʱ�ȭ ��Ű�� �Լ��� ȣ���Ѵ
  ��    �� : ���
  �� �� �� : ���
--------------------------------------------------------------------*/
mwatchcli::mwatchcli()
{
	char msgWatchAddr[128] = "\0";
	strcpy(msgWatchAddr, MSGWATCH_ADDR);
	Initialize( msgWatchAddr, MSGWATCH_PORT );
}


/*--------------------------------------------------------------------
  �� �� �� : mwatchcli::mwatchcli( char *host )
  ��ɰ��� : ȣ��Ʈ�� ��Ʈ��ȣ�� ������Ͽ� �����س��� ���� �������
			 �ּҴ� ���� �Է¹޾� �ʱ�ȭ ��Ű�� �Լ��� ȣ���Ѵ
  ��    �� : char *host -> ȣ��Ʈ�� �ּ�
  �� �� �� : ���
--------------------------------------------------------------------*/
mwatchcli::mwatchcli( char *host )
{
	Initialize( host, MSGWATCH_PORT );
}


/*-----------------------------------------------------------------------
  �� �� �� : mwatchcli::mwatchcli( char *host, int port )
  ��ɰ��� : ȣ��Ʈ�� �ּҿ� ��Ʈ��ȣ�� �Է¹޾� ������ �ʱ�ȭ��Ű��
			 �Լ��� ȣ���Ѵ
  ��    �� : char *host -> ȣ��Ʈ�� �ּ�,  int port -> ȣ��Ʈ����Ʈ��ȣ
  �� �� �� : ���
-----------------------------------------------------------------------*/
mwatchcli::mwatchcli( char *host, int port )
{
	Initialize( host, port );
}


/*--------------------------------------------------------------------
  �� �� �� : mwatchcli::~mwatchcli()
  ��ɰ��� : ������ �ݴ´
  ��    �� : ���
  �� �� �� : ���
--------------------------------------------------------------------*/
mwatchcli::~mwatchcli()
{
	cltSock.Close();
}


/*--------------------------------------------------------------------
  �� �� �� : void mwatchcli::Initialize( char *host, int port )
  ��ɰ��� : ȣ��Ʈ�ּҿ� ��Ʈ��ȣ�� �Է¹޾� UDP������ �����ϰ
			 ���Ͽ� ȣ��Ʈ�ּҿ� ��Ʈ��ȣ�� ���ε� ��Ų�
  ��    �� : char *host -> ȣ��Ʈ��,   int port -> ��Ʈ��ȣ
  �� �� �� : ���
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
  �� �� �� : BOOL mwatchcli::addID( char * szID )
  ��ɰ��� : ���񽺾��̵�� �޽����Ǽ��� �߰��ϴ� �Լ��� ȣ���Ѵ
  ��    �� : char * szID -> ���񽺾��̵�
  �� �� �� : ���
--------------------------------------------------------------------*/
BOOL mwatchcli::addID( char * szID )
{
	return addID( szID, 1 );
}


/*----------------------------------------------------------------------
  �� �� �� : BOOL mwatchcli::addID( char * szID, int m_nMsgCnt )
  ��ɰ��� : ���񽺾��̵� �߰����Ѵ
  ��    �� : char * szID -> ���񽺾��̵�,   int m_nMsgCnt -> �޽����Ǽ�
  �� �� �� : ���
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
  �� �� �� : BOOL mwatchcli::addMsgCnt( char * szID )
  ��ɰ��� : ���񽺾��̵�� �޽����Ǽ��� �߰��� �Լ��� ȣ���Ѵ
  ��    �� : char * szID -> ���񽺾��̵�
  �� �� �� : ���
--------------------------------------------------------------------*/
BOOL mwatchcli::addMsgCnt( char * szID )
{
	return addMsgCnt( szID, 1 );
}


/*---------------------------------------------------------------------
  �� �� �� : BOOL mwatchcli::addMsgCnt( char * szID, int m_nMsgCnt )
  ��ɰ��� : ȣ��Ʈ��, ���񽺾��̵�, �޽����Ǽ��� UDP������ �����Ѵ
  ��    �� : char * szID -> ���񽺾��̵�,  int m_nMsgCnt -> �޽����Ǽ�
  �� �� �� : ���
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
		snprintf( szTmpBuf, sizeof(szTmpBuf)-1," SEND [%s : %d]��°..",
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
		sprintf( szTmpBuf, " NOT YET [%s : %d]��°.. ",
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

