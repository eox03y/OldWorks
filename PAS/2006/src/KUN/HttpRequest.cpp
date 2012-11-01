#include "HttpRequest.h"
#include "PasLog.h"

#define	CHUNK_INIT_BUFFSIZE	(1024*16)

using namespace HTTP;

int	Request::HeaderBuffBytes = 4*1024;
Request::Request()
{
	contentLength = 0;
	headLeng =  0;
	bodyLeng = 0;
	
	pMBManager = MessageBlockManager::instance();
	ACE_ASSERT(pMBManager != NULL);
	
	pRawHeader = pMBManager->alloc(HeaderBuffBytes);
	ACE_ASSERT(pRawHeader != NULL);

	pRawBody = NULL;

	seqNum = 0;
}

Request::~Request()
{
	ACE_ASSERT(pRawHeader != NULL);
	pMBManager->free(pRawHeader);
	
	if (pRawBody != NULL)
		pMBManager->free(pRawBody);
}

const ACE_Message_Block* Request::getRawHeader() const
{
	return pRawHeader;
}

const ACE_Message_Block* Request::getRawBody() const
{
	return pRawBody;
}

RequestHeader* Request::getHeader()
{
	return &header;
}

const RequestHeader* Request::getHeader() const
{
	return &header;
}

size_t Request::getContentLength() const
{
	return contentLength;
}

int Request::getHeadLeng()
{
	if (headLeng==0)
		headLeng = pRawHeader->length();
	return headLeng;
}

int Request::getBodyLeng()
{
	if (bodyLeng==0 && pRawBody)
		bodyLeng = pRawBody->length();
	return bodyLeng;
}
int Request::setHeader(const header_t& headerIN)
{
	return setHeader(headerIN, headerIN.size());
}

int Request::setHeader(const char* buf, const size_t bufSize)
{
	ACE_ASSERT(buf);
	ACE_ASSERT(bufSize > 0);

	int pasingSuccess = header.parse(buf, bufSize);
	if(pasingSuccess < 0)
	{
		PAS_INFO("Header parsing fail.");
		return -1;
	}

	// ��� ���� ���� ���� Ȯ�� (resize)
	if(pRawHeader->size() < bufSize)
	{
		pMBManager->free(pRawHeader);
		pRawHeader = pMBManager->alloc(bufSize);
		if(pRawHeader == NULL)
		{
			PAS_ERROR("ReAllocMessageBlock fail!!");
			return -1;
		}
	}
	
	// ��� ����
	pRawHeader->reset();
	pRawHeader->copy(buf, bufSize);

	// body ������
	contentLength = header.getContentLength();

	// re-calculate
	headLeng = pRawHeader->length();
	return 0;
}


int Request::setBody(const char* buf, const size_t bufSize)
{
	ACE_ASSERT(buf);
	ACE_ASSERT(bufSize > 0);

	size_t newSize = std::max(bufSize, contentLength);

	if (pRawBody == NULL) {
		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Request::setBody, AllocMessageBlock fail!!");
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
			PAS_ERROR("Request::setBody, ReAllocMessageBlock fail!!");
			return -1;
		}
	}

	// copy
	pRawBody->reset();
	pRawBody->copy(buf, bufSize);

	// re-calculate
	bodyLeng = pRawBody->length();

	return 0;
}

int Request::appendBody(const char* buf, const size_t bufSize)
{
	ACE_ASSERT(buf);
	ACE_ASSERT(bufSize > 0);

	// �Ʒ��� ���� �ָ�  ��뷮 ó���� �� ������ �ȴ�.
	// bufSize ��  recv Buffer ������� 4K �����̴�.
	// �޸� �Ҵ�/���� ȸ���� ���Ϸ��� �Ʒ��� ���� �� �־�� �Ѵ�.

	if (pRawBody == NULL) 
	{
		// �Ʒ��� ���� �ָ� ��뷮 ó���� �� ������ �ȴ�.
		// �޸� �Ҵ�/���� ȸ���� ���Ϸ��� �Ʒ��� ���� �� �־�� �Ѵ�.
		size_t newSize = std::max(bufSize, contentLength);

		// chunk �� ��� ���� ���۸� ����� �Ҵ��ϱ� ����. (2007.1.3)
		// ����� ���� ũ�� Ȯ���� ���� ����, �ʱ� �ּ� ����� ����
		newSize = std::max(newSize, (size_t)CHUNK_INIT_BUFFSIZE);

		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Request::appendBody, AllocMessageBlock fail!!");
			return -1;
		}
	}

	ACE_ASSERT(pRawBody != NULL);
	
	if(pRawBody->space() < bufSize)
	{
		// ���� ���ŵ����� ����� ���� ���� - ���� �������� ����� ����.
		size_t newSize = std::max((pRawBody->size() + (bufSize * 2)), (pRawBody->size() * 2));	
		
		PAS_DEBUG2("Request::appendBody OldSize[%d] NewSize[%d]", pRawBody->size(), newSize);

		// ���� ������ ����(resize)
		ACE_Message_Block* pTmpBlock = pRawBody;
		pRawBody = pMBManager->alloc(newSize);
		if(pRawBody == NULL)
		{
			PAS_ERROR("Request::appendBody, AllocMessageBlock fail!!");
			return -1;
		}

		// �ӽ� ���۷� ���� ������ ����
		int resultCopy = pRawBody->copy(pTmpBlock->rd_ptr(), pTmpBlock->length());
		if(resultCopy < 0)
		{
			PAS_ERROR("Response::appendBody >> ���� ������ ���� ����");
		}

		// �ӽ� ���� ����
		pMBManager->free(pTmpBlock);
	}

	// append
	int resultCopy = pRawBody->copy(buf, bufSize);
	if(resultCopy < 0)
	{
		PAS_ERROR("Response::appendBody >> �ű� ������ ���� ����");
		return -1;
	}

	// re-calculate
	bodyLeng = pRawBody->length();
	return 0;
}

int Request::setHost(const char* host)
{
	header.setHost(host);
	return buildToRawHeader();
}

int Request::setPort(const int port)
{
	header.setPort(port);
	return buildToRawHeader();
}

int Request::setHostPort(const char* host, const int port)
{
	header.setHost(host);
	header.setPort(port);

	return buildToRawHeader();
}

int Request::setUrl(const char* url)
{
	header.setUrl(url);

	return buildToRawHeader();
}

int Request::buildToRawHeader()
{
	header_t newRawHeader;
	int ret = header.build(&newRawHeader);
	if(ret < 0)
	{
		PAS_ERROR("Request::buildToRawHeader >> ��� ���� ����");
		return -1;
	}

	// ��� ���� ���� ���� Ȯ�� (resize)
	if(pRawHeader->size() < newRawHeader.size())
	{
		pMBManager->free(pRawHeader);
		pRawHeader = pMBManager->alloc(newRawHeader.size());
		if(pRawHeader == NULL)
		{
			PAS_ERROR("Request::buildToRawHeader, AllocMessageBlock fail!!");
			return -1;
		}
	}
	
	// ��� ����
	pRawHeader->reset();
	ret = pRawHeader->copy(newRawHeader, newRawHeader.size());
	if(ret < 0)
	{
		PAS_ERROR("Request::buildToRawHeader >> raw ��� ���ۿ� ���� ����");
		return -1;
	}

	// re-calculate
	headLeng = pRawHeader->length();
	
	return 0;
}

void Request::clear()
{
	pRawHeader->reset();

	if (pRawBody != NULL)
		pRawBody->reset();
	header.clear();
	contentLength = 0;	
	headLeng =  0;
	bodyLeng = 0;
}

/**
SSL ó���� -- http �м� ���� �׳� ����
*/
int Request::recvRaw(ACE_Message_Block &recvBuf)
{
	setBody(recvBuf.rd_ptr(), recvBuf.length());
	return 0;
}

