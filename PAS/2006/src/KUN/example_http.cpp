#include "HttpTypes.h"
#include "HttpUrlParser.h"
#include "HttpRequestHeader.h"
#include "HttpResponseHeader.h"

#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Stream.h>
#include <ace/INET_Addr.h>
#include <ace/Message_Block.h>

int main(const int argc, const char* argv[])
{
	// ����
	if(argc < 2)
	{
		printf("usage : %s URL\n", argv[0]);
		return 0;
	}

	// URL �Ľ�
	HTTP::url_t url(argv[1]);
	HTTP::UrlParser urlParser;
	
	urlParser.parse(url);

	HTTP::host_t host = urlParser.getHost();
	if(host.isEmpty())
	{
		printf("URL�� ȣ��Ʈ ������ �����ϴ�.");
		return 0;
	}

	int port = urlParser.getPort();

	// �� ���� ���� ���� ����
	ACE_SOCK_Connector connector;
	ACE_SOCK_Stream stream;
	ACE_INET_Addr addr(port, host);

	// �� ������ ����
	connector.connect(stream, addr);

	// ��û http ��� ����
	HTTP::RequestHeader reqHeader;

	HTTP::host_t hostAndPort;
	if(port == 80)
		hostAndPort = host;
	else
		hostAndPort.sprintf("%s:%d", host.toStr(), port);

	reqHeader.setHost(hostAndPort);
	reqHeader.setUrl(urlParser.getPath());
	reqHeader.setMethod(HTTP::RM_GET);
	reqHeader.setVersion("HTTP/1.1");
	
	// ��û http ��� ����
	HTTP::header_t reqRawHeader;
	reqHeader.build(&reqRawHeader);

	// rawHeader ���
	printf("[Request Header]\n%s\n", reqRawHeader.toStr());

	// ��û �۽�
	stream.send(reqRawHeader.toStr(), reqRawHeader.size());

	HTTP::ResponseHeader resHeader;
	HTTP::header_t resRawHeader;

	// ������ ����
	ACE_Message_Block recvBuffer(102400);
	while(true)
	{
		int recvSize = stream.recv(recvBuffer.wr_ptr(), recvBuffer.space());
		
		// ����
		if(recvSize < 0)
		{
			printf("���� ����\n");
			return -1;
		}

		// ���� ����
		if(recvSize == 0)
		{
			printf("���� ����\n");
			return -1;
		}

		// move write pointer
		ACE_ASSERT(recvSize <= recvBuffer.space());
		recvBuffer.wr_ptr(recvSize);

		// ��� ã��
		int ret = resHeader.getHeader(&resRawHeader, recvBuffer.rd_ptr(), recvBuffer.length());
		if(ret == 0)
		{
			// move read pointer
			recvBuffer.rd_ptr(resRawHeader.size());
			break;	
		}
	}

	// ���� ��� �Ľ�
	int ret = resHeader.parse(resRawHeader, resRawHeader.size());
	if(ret < 0)
	{
		printf("�Ľ� ����\n");
		return -1;
	}

	// ���� �ڵ� 
	HTTP::ResponseStatus resStatus = resHeader.getStatus();
	if(resStatus != HTTP::RS_OK)
	{
		printf("�� ���� ��û\n");
		return -1;
	}

	// ���� body ������ Ȯ��
	size_t contentLength = resHeader.getContentLength();
	if(contentLength == 0)
	{
		printf("���� ���� ����\n");
		return -1;
	}

	// body ����
	ACE_Message_Block resRawBody(contentLength+1);

	// ���� ���ۿ� ���� �ִ� �����Ͱ� �ֳ�?
	if(recvBuffer.length() > 0)
	{
		resRawBody.copy(recvBuffer.rd_ptr(), recvBuffer.length());
	}

	// body ������ ����
	while(resRawBody.length() < contentLength)
	{
		int recvSize = stream.recv(resRawBody.wr_ptr(), resRawBody.space());
		
		// ����
		if(recvSize < 0)
		{
			printf("���� ����\n");
			return -1;
		}

		// ���� ����
		if(recvSize == 0)
		{
			printf("���� ����\n");
			return -1;
		}
		
		// move write pointer
		ACE_ASSERT(recvSize <= resRawBody.space());
		resRawBody.wr_ptr(recvSize);
	}

	// rawBody �������� NULL �߰� (printf�� ����ϱ� ����)
	ACE_ASSERT(1 <= resRawBody.space());
	char* writePtr = resRawBody.wr_ptr();
	*writePtr = '\0';
	resRawBody.wr_ptr(1);

	// body ���
	printf("[Response Data]\n%s\n", resRawBody.rd_ptr());

	return 0;
}