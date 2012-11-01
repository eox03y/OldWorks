#ifndef __HTTP_HEADER_BUILDER_H__
#define __HTTP_HEADER_BUILDER_H__

#include "HttpTypes.h"
#include "HttpBaseHeader.h"

namespace HTTP
{
	class ResponseHeader : public BaseHeader
	{
	// �޼ҵ� ����
	public:
		ResponseHeader(void);
		~ResponseHeader(void);

		void setVersion(const char* str);
		void setStatusString(const char* str);
		void setContentLength(const int len);
		void setStatusCode(const int code);

		version_t getVersion() const;
		ResponseStatus getStatus() const;
		status_t getStatusString() const;
		
		/// content-length �ʵ��� ���� �����Ѵ�.
		/**
		* ����� content-length �ʵ尡 �������� �ʴ´ٸ�, 0�� �����Ѵ�.
		*/
		size_t getContentLength() const;
		int getStatusCode() const;
		bool hasChunked() const;

		/// ��� �Ľ�
		int parse(const char* srcBuf, size_t srcBufSize);

		/// ��� ����
		int build(header_t* pDestHeader) const;

		/// �Ľ� ��� clear
		void clear();

	private:
		int parseStartLine(const line_t& line);
		int buildStartLine(line_t* pLine) const;


	// ��� ���� ����
	private:
		// response info
		version_t version;
		int statusCode;
		status_t statusStr;
	};
};

#endif
