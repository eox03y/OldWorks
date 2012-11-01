#include "HttpResponse.h"
#include "PasLog.h"
#include <ace/Message_Block.h>

#define	BODY_BUF_INIT_SIZE	(16*1024)

using namespace HTTP;

int	Response::HeaderBuffBytes = 4*1024;
Response::Response()
{
	contentLength = 0;
	chunkSize = 0;
	receivedChunkSize = 0;
	bodyBufferExpandCount = 0;

	pMBManager = MessageBlockManager::instance();
	ACE_ASSERT(pMBManager != NULL);
	
	pRawHeader = pMBManager->alloc(HeaderBuffBytes);
	ACE_ASSERT(pRawHeader != NULL);

	pRawBody = NULL;

}

Response::~Response()
{
	ACE_ASSERT(pRawHeader != NULL);
	pMBManager->free(pRawHeader);
	
	if (pRawBody != NULL)
		pMBManager->free(pRawBody);
}

int Response::getBodyBufferExpandCount() const
{
	return bodyBufferExpandCount;
}


int Response::setHeader(const ResponseHeader& header_)
{
	header_t rawHeader_;
	int buildResult = header_.build(&rawHeader_);
	if(buildResult < 0)
	{
		PAS_ERROR("Response::setHeader >> raw header ���� ����");
		return -1;
	}

	return setHeader(rawHeader_);
}

int Response::setHeader(const header_t& headerIN)
{
	return setHeader(headerIN, headerIN.size());
}


/**
�� ���ڿ��� �Ľ��Ͽ�  header element�� �����Ͽ� �����ϰ�,
�� ���ڿ��� ���� �޽��� ���ۿ� �����Ѵ�.
*/
int Response::setHeader(const char* buf, const size_t bufSize)
{
	ACE_ASSERT(buf);
	ACE_ASSERT(bufSize > 0);

	// ��� �Ľ�
	int resultParse = header.parse(buf, bufSize);
	if(resultParse < 0)
	{
		PAS_DEBUG("��� �Ľ� ����");
		return -1;
	}

	// ��� ���� ���� ���� Ȯ�� (resize)
	if(pRawHeader->size() < bufSize)
	{
		pMBManager->free(pRawHeader);
		pRawHeader = pMBManager->alloc(bufSize);
		if(pRawHeader == NULL)
		{
			PAS_ERROR("Response::setHeader, ReAllocMessageBlock fail!!");
			return -1;
		}
	}
	
	// ��� ����
	pRawHeader->reset();
	pRawHeader->copy(buf, bufSize);

	// Content-Length
	contentLength = header.getContentLength();

	return 0;
}


const ACE_Message_Block* Response::getRawHeader() const
{
	return pRawHeader;
}

const ACE_Message_Block* Response::getRawBody() const
{
	return pRawBody;
}

const ResponseHeader* Response::getHeader() const
{
	return &header;
}

size_t Response::getContentLength() const
{
	return contentLength;
}

void Response::setChunkSize(const size_t size)
{
	chunkSize = size;
}

size_t Response::getChunkSize() const
{
	return chunkSize;
}

void Response::setRecevicedChunkSize(const size_t size)
{
	receivedChunkSize = size;
}

size_t Response::getReceivedChunkSize() const
{
	return receivedChunkSize;
}

bool Response::hasChunked() const
{
	return header.hasChunked();
}

int Response::getHeadLeng() const
{
	return pRawHeader->length();
}

int Response::getBodyLeng() const
{
	if( pRawBody == NULL )
		return 0;

	return pRawBody->length();
}

int Response::setBody(const char* buf, const size_t bufSize)
{
	ACE_ASSERT(buf);
	ACE_ASSERT(bufSize > 0);

	size_t newSize = std::max(bufSize, contentLength);
	newSize = std::max(newSize, (size_t)BODY_BUF_INIT_SIZE);

	if (pRawBody == NULL) {
		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Response::setBody, AllocMessageBlock fail!!");
			return -1;
		}
	}

	ACE_ASSERT(pRawBody != NULL);
	// resize
	if(pRawBody->size() < bufSize)
	{
		pMBManager->free(pRawBody);
		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Response::setBody, ReAllocMessageBlock fail!!");
			return -1;
		}
	}

	// copy
	pRawBody->reset();
	pRawBody->copy(buf, bufSize);

	// reset buf expand counter
	bodyBufferExpandCount = 0;

	refreshContentLength();

	return 0;
}


int Response::appendBody(const char* buf, const size_t bufSize)
{
	ACE_ASSERT(buf != NULL);
	ACE_ASSERT(bufSize > 0);

	if (pRawBody == NULL) 
	{
		// �Ʒ��� ���� �ָ� ��뷮 ó���� �� ������ �ȴ�.
		// �޸� �Ҵ�/���� ȸ���� ���Ϸ��� �Ʒ��� ���� �� �־�� �Ѵ�.
		size_t newSize = std::max(bufSize, contentLength);

		// chunk �� ��� ���� ���۸� ����� �Ҵ��ϱ� ����. (2007.1.3)
		// ����� ���� ũ�� Ȯ���� ���� ����, �ʱ� �ּ� ����� ����
		newSize = std::max(newSize, (size_t)BODY_BUF_INIT_SIZE);

		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Response::appendBody, AllocMessageBlock fail!!");
			return -1;
		}

		// reset buf expand counter
		bodyBufferExpandCount = 0;
	}

	ACE_ASSERT(pRawBody != NULL);
	
	// ���� ũ�� Ȯ��
	if(pRawBody->space() < bufSize)
	{
		// ����� ���� ũ�� Ȯ���� ���� ����
		size_t newSize = std::max((pRawBody->size() + (bufSize * 2)), (pRawBody->size() * 2));	
		
		PAS_DEBUG2("Response::appendBody OldSize[%d] NewSize[%d]", pRawBody->size(), newSize);

		// ���� ������ ����(resize)
		ACE_Message_Block* pTmpBlock = pRawBody;
		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Response::appendBody, AllocMessageBlock fail!!");
			return -1;
		}

		// ���� ���۷� ���� ������ ����
		int resultCopy = pRawBody->copy(pTmpBlock->rd_ptr(), pTmpBlock->length());
		if(resultCopy < 0)
		{
			PAS_ERROR("Response::appendBody >> ���� ������ ���� ����");
		}

		// �ӽ� ���� ����
		pMBManager->free(pTmpBlock);

		bodyBufferExpandCount++;
	}

	// append
	int resultCopy = pRawBody->copy(buf, bufSize);
	if(resultCopy < 0)
	{
		PAS_ERROR("Response::appendBody >> �ű� ������ ���� ����");
		return -1;
	}

	return 0;
}

void Response::refreshContentLength()
{
	ACE_ASSERT(pRawBody != NULL);
		
	int realLength = pRawBody->length();

	contentLength = realLength;
	if(realLength > 0)
	{
		header.replaceElement("Content-Length", realLength);
	}
	else
	{
		header.delElement("Content-Length");
	}
}

void Response::clear()
{
	header.clear();
	pRawHeader->reset();
	if (pRawBody != NULL)
		pRawBody->reset();
	contentLength = 0;
	chunkSize = 0;
	receivedChunkSize = 0;
}
