#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <ace/Message_Block.h>
#include "HttpTypes.h"
#include "HttpRequestHeader.h"
#include "MutexQueue.hpp"
#include "NullmutexQueue.hpp"
#include "MemoryPoolManager.h"
#include "ActiveObjectChecker.h"

namespace HTTP
{
	class Request : public ActiveObjectChecker
	{
	// ��� �Լ�
	public:
		static	int	HeaderBuffBytes;
		
		Request();
		virtual ~Request();

		void setSeqNum(int _seq)
		{
			seqNum = _seq;
		}

		int getSeqNum()
		{
			return seqNum;
		}

		// ACE_Message_Block ������ ���̱� ���� head, body ���� �Լ� �߰� 2006.10.15 -- handol
		int	getHeadLeng();
		int	getBodyLeng();
		
		int setHeader(const header_t& header);
		int setHeader(const char* buf, const size_t bufSize);
		int setBody(const char* buf, const size_t bufSize);
		int appendBody(const char* buf, const size_t bufSize);

		int setHost(const char* host);
		int setPort(const int port);
		int setHostPort(const char* host, const int port);
		int setUrl(const char* url);

		const ACE_Message_Block* getRawHeader() const;
		const ACE_Message_Block* getRawBody() const;
		RequestHeader* getHeader();
		const RequestHeader* getHeader() const;
		size_t getContentLength() const;

		void clear();

		// SSL ó���� -- http �м� ���� �׳� ����
		int	recvRaw(ACE_Message_Block &recvBuf);

	private:
		int buildToRawHeader();

	// ��� ����
	private:
		RequestHeader header;
		ACE_Message_Block* pRawHeader;
		ACE_Message_Block* pRawBody;
		size_t contentLength;
		MessageBlockManager* pMBManager;	///< MessageBlock �޸� Ǯ �Ŵ���
		int	seqNum;

		int	headLeng;
		int	bodyLeng;
		
	};
};

#endif
