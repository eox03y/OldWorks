#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <ace/Message_Block.h>
#include "HttpTypes.h"
#include "HttpResponseHeader.h"
#include "MutexQueue.hpp"
#include "NullmutexQueue.hpp"
#include "MemoryPoolManager.h"
#include "Common.h"
#include "ActiveObjectChecker.h"

namespace HTTP
{
	class Response : public ActiveObjectChecker
	{
	public:
		//-------------------------------------------------------
		// ��� �Լ�
		//-------------------------------------------------------

		Response();
		virtual ~Response();

		void setSeqNum(int _seq)
		{
			seqNum = _seq;
		}

		int getSeqNum()
		{
			return seqNum;
		}

		// ACE_Message_Block ������ ���̱� ���� head, body ���� �Լ� �߰� 2006.10.15 -- handol
		int	getHeadLeng() const;
		int	getBodyLeng() const;
		int getBodyBufferExpandCount() const;
		
		// input function
		int setHeader(const ResponseHeader& header);
		int setHeader(const header_t& header);
		int setHeader(const char* buf, const size_t bufSize);
		int setBody(const char* buf, const size_t bufSize);
		int appendBody(const char* buf, const size_t bufSize);
		void setChunkSize(const size_t size);
		void setRecevicedChunkSize(const size_t size);

		// output function
		const ACE_Message_Block* getRawHeader() const;
		const ACE_Message_Block* getRawBody() const;
		const ResponseHeader* getHeader() const;
		size_t getContentLength() const;
		size_t getChunkSize() const;
		size_t getReceivedChunkSize() const;

		bool hasChunked() const;
		void refreshContentLength();

		void clear();

		//-------------------------------------------------------
		// ��� ����
		//-------------------------------------------------------
		static int HeaderBuffBytes;
	
	private:
		//-------------------------------------------------------
		// ��� ����
		//-------------------------------------------------------
		ResponseHeader header;
		ACE_Message_Block* pRawHeader;
		ACE_Message_Block* pRawBody;
		size_t contentLength;				///< ����� ��� content-length, rawBody �� ���� ������ʹ� ���þ���
		size_t chunkSize;					///< �����ؾ��� chunkSize
		size_t receivedChunkSize;			///< ������ chunkSize
		MessageBlockManager* pMBManager;	///< MessageBlock �޸� Ǯ �Ŵ���
		int	seqNum;

		int bodyBufferExpandCount;
		
		
	};
};

#endif
