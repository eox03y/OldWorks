#ifndef __HTTP_PARSER__
#define __HTTP_PARSER__

#include <string>
#include <vector>
#include <map>
#include <ace/Message_Block.h>
#include "FastString.hpp"
#include "HttpTypes.h"
#include "HttpBaseHeader.h"

namespace HTTP
{
	class RequestHeader : public BaseHeader
	{
	// �޼ҵ� ����
	public:
		RequestHeader(void);
		virtual ~RequestHeader(void);

		void setUrl(const url_t& url);
		void setHostPort(const host_t& host_, int port_);
		void setHost(const host_t& host);
		void setPort(int port);
		void setPath(const path_t& path);
		void setVersion(const version_t& version);
		void setMethod(RequestMethod method);

		RequestMethod getMethod() const;
		method_t getMethodStr() const;
		url_t getUrl() const;
		void getUrl(char *urlBuff, int size)  const;
		url_t getOrgUrl() const;
		void getOrgUrl(char *urlBuff, int size) const;
		host_t getHost() const;
		void getHost(char *hostBuff, int size)  const;

		int getPort() const;
		path_t getPath() const;
		version_t getVersion() const;
		size_t getContentLength() const;

		/// ��� �Ľ�
		int parse(const char* srcBuf, size_t srcBufSize);

		/// ��� ����
		int build(header_t* pDestHeader) const;
		header_t build() const;

		/// �Ľ� ��� clear
		virtual void clear();

		/// @todo  KTF browser ���� �ش��ϴ� header ���� ó���� ���� �͵� �߰�.

	private:
		int parseStartLine(const line_t& line);
		void parseHostInfo();

		int buildStartLine(line_t* pLine) const;
		void buildHostInfo(const host_t& host_, const int port_);

		method_t methodToString(RequestMethod method) const;


	// ������� ����
	private:
		// request info
		RequestMethod method;
		method_t methodStr;
		host_t host;
		int port;
		path_t path;
		version_t version;
		url_t orgUrl;
	};
};

#endif
