#ifndef __PAS_EVENT_HANDLER_H__
#define __PAS_EVENT_HANDLER_H__

/**
@brief ��� �ڵ鷯�� �θ� Ŭ����

��� �ڵ鷯�� �θ� Ŭ�����̸� ���������� ������ �� ����.(�������)
ACE_Event_Handler �� ��� �ϰ� �����Ƿ� ACE Reactor �� ����Ͽ�
�̺�Ʈ�� ���� �� �ִ�.
ClientHandler, SantaHandler, AuthHandler, CPHandler ���� �� Ŭ������ ��ӹ޴´�.

���� ����(�ܸ�, CP) �����͸� ������ Reactor �� PasHandler �� handle_input �� ȣ��ȴ�.
*/


#include "Common.h"
#include <ace/Event_Handler.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Stream.h>
#include <ace/Message_Block.h>
#include <ace/Mutex.h>
#include <ace/Pipe.h>
#include "WorkInfo.h"
#include "SystemStatistic.h"
#include "ActiveObjectChecker.h"

#include "MyLog.h"

#define	MYID_LEN	(255)
class PasHandler :
	public ACE_Event_Handler, public ActiveObjectChecker
{
// ��� Ÿ��
public:
	enum CommandID
	{
		CID_CP_Connected,
		CID_CP_Completed,
		CID_CP_Closed,
		CID_CP_SSLData,
		CID_CP_Received_Header,
		CID_CP_Received_PartOfBody,
		CID_Santa_Completed,
		CID_Santa_Closed,
		CID_Santa_TimeOut
	};

	enum HandlerTypeValue
	{
		HT_ClientHandler = 1,
		HT_CpHandler,
		HT_AuthHandler,
		HT_SantaHandler
	};

// ��� �Լ�
public:
	PasHandler(ACE_Reactor* pReactor, int handlertype=0);
	virtual ~PasHandler(void);

	virtual int handle_input(ACE_HANDLE fd=ACE_INVALID_HANDLE);
	virtual int handle_output(ACE_HANDLE fd=ACE_INVALID_HANDLE);
	virtual int handle_close(ACE_HANDLE fd, ACE_Reactor_Mask close_mask);
	virtual int handle_exception(ACE_HANDLE fd=ACE_INVALID_HANDLE);
	virtual int handle_exit(ACE_Process *);
	virtual int handle_signal(int signum, siginfo_t* =0, ucontext_t* =0);

	virtual ACE_HANDLE get_handle(void) const;
	virtual void set_handle(ACE_HANDLE fd);

	int _onReceived();
	virtual void onReceived();
	
	virtual int onSendable();
	virtual void onCloseByPeer();

	virtual void onCommand(CommandID cid, PasHandler* pEH, void* arg1 = NULL, void* arg2 = NULL);
	
	void requestClose();

	/// ����
	/**
	 * ����� ������ �����ϰ�, ������Ʈ�� �޸𸮿��� �����Ѵ�.
	 **/
	virtual void close();

	void addEvent(const int eventMask);
	void removeEvent(const int eventMask);

	/// timer �̺�Ʈ ���
	/**
	 * �ð��� ����Ǹ� handle_timeout()�� ȣ��ȴ�.
	 * return ���� timer id�� removeTimerEvent() ���� ����Ѵ�.
	 *
	 * @see removeTimerEvent()
	 *
	 * @return �����ϰ�� timer id, ������ ��� -1
	 **/
	long addTimerEvent(const void* arg, const ACE_Time_Value& delay, const ACE_Time_Value& interval = ACE_Time_Value::zero);
	long addTimerEvent(PasHandler* pEH, const void* arg, const ACE_Time_Value& delay, const ACE_Time_Value& interval = ACE_Time_Value::zero);
	void removeTimerEvent(const long timerID);

	void addNotifyEvent();
	void addNotifyEvent(ACE_Event_Handler* pEH);

	const ACE_Time_Value& getReceiveTime() const;
	const ACE_Time_Value& getSendTime() const;
	const ACE_Time_Value& getConnectTime() const;
	const ACE_Time_Value& getConnectionRequestTime() const;
	const ACE_Time_Value& getCreateTime() const;
	const ACE_Time_Value& getCloseTime() const;


	void startTimeTick(const int intervalSec, const int intervalUSec = 0);
	void stopTimeTick();

	void setTraceLog(MyLog *log);

	const char* getMyInfo() const;

	bool isConnected();

	int connect(const host_t &host, int port);

protected:
	/// recv �߿� connection close �� ������ ��� ȣ���
	virtual void onRecvFail();
	
	/// send �߿� connection close �� ������ ��� ȣ���
	virtual void onSendFail();

	int deleteSendQueue();
	int enSendQueue(const char* buf, size_t bufSize);

	/// �۽� �����Ͱ� ��� �۽ŵǾ��� ���
	virtual void onSendQueueEmpty();


	/// ���� ����
	void sockClose();

	virtual void onConnect();
	void setJobDone();

	virtual	char* setMyInfo();

	void setCloseAfterSend();
	
	void startReceiveTimer();
	void stopReceiveTimer();
	bool isIdle(const ACE_Time_Value& currentTime, ACE_Time_Value &maxIdleTime);
	
	void startConnectTimer();
	void stopConnectTimer();
	bool isConnectingNow();
	bool isConnectTimeOut(const ACE_Time_Value& current_time, ACE_Time_Value &maxtime);


private:
	int	sendData(const char* buf, size_t bufSize);
	

// ��� ����
protected:
	ACE_SOCK_Stream sock;
	ACE_Message_Block recvBuffer;
	SystemStatistic* pSysStat;
	size_t totalReceiveSize;
	int	handlerType;

	MyLog *tracelog; /// phone trace log
	bool jobDone;
	PasMessageQueue sendQueue;

	bool connectedFlag;
	ACE_Time_Value _receiveTime; // ���� ��� �� �� ���� �ð������� ����.
	ACE_Time_Value _sendTime; // �ֱ� �۽� �ð�
	ACE_Time_Value _connectTime; // ���� �õ� ������ �� ���� �ð������� ����. ���ӵǸ� 0 ���� ����.

	char myinfo[MYID_LEN+1];
	
private:

	long timeTickID;

	bool closeAfterSend; // �ܸ� ��û ����� Connection: close �� ���Ե� ���.
	bool __activeObject; // ���� object�� ���������� �޸𸮿� �Ҵ�� object ������ ����ϴ� flag

};

#endif
