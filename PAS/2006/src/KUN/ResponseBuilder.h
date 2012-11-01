#ifndef ResponseBuilder_H
#define ResponseBuilder_H

/**
@brief ���� ��Ȳ ó�� �޽����� �ܸ��� ����

Pasgw �� Client(�ܸ���)�� ���� ���� ���������� �̸� ������ Ŭ����
�� �ɹ��Լ����� static ���� ����Ǿ� ��ü �������� ���� �� �ִ�.

���� Client <-> Pasgw <-> CP �� ������ ��û/������ �߻�������
Ư���� ���(��:CP ����, �������� ��)�� Pasgw�� ���� ������ �ؾ��Ѵ�.
���� ���뵵 �����ϹǷ� �̷��� �̸� �����θ� ���ϴ�.
*/


#include "HttpRequest.h"
#include "HttpResponse.h"

enum HttpStatusCode
{
	// Informational 1xx
	HTTP_SCODE_CONTINUE = 100,
	HTTP_SCODE_SWITCHING_PROTOCOLS = 101,
	
	// Successful 2xx
	HTTP_SCODE_OK = 200,
	HTTP_SCODE_CREATED = 201,
	HTTP_SCODE_ACCEPTED = 202,
	HTTP_SCODE_NON_AUTH_INFO = 203, // Non-Authoritative Information
	HTTP_SCODE_NO_CONTENT = 204,
	HTTP_SCODE_RESET_CONTENT = 205,
	HTTP_SCODE_PARTIAL_CONTENT = 206,
	
	// Redirection 3xx
	HTTP_SCODE_MULTI_CHOICES = 300, // Multiple Choices
	HTTP_SCODE_MOVED_PERMANENTLY = 301,
	HTTP_SCODE_FOUND = 302, 
	HTTP_SCODE_SEE_OTHER = 303,
	HTTP_SCODE_NOT_MODIFIED = 304,
	HTTP_SCODE_USE_PROXY = 305,
	HTTP_SCODE_UNUSED_306 = 306, // (Unused)
	HTTP_SCODE_TEMP_REDIRECT = 307, // Temporary Redirect
	
	// Client Error 4xx
	HTTP_SCODE_BAD_REQUEST = 400,
	HTTP_SCODE_UNAUTHORIZED = 401,
	HTTP_SCODE_PAYMENT_REQUIRED = 402,
	HTTP_SCODE_FORBIDDEN = 403, 
	HTTP_SCODE_NOT_FOUND = 404,
	HTTP_SCODE_METHOD_NOT_ALLOWED = 405,
	HTTP_SCODE_NOT_ACCEPTABLE = 406,
	HTTP_SCODE_PROXY_AUTH_REQUIRED = 407, // Proxy Authentication Required
	HTTP_SCODE_REQUEST_TIMEOUT = 408, 
	HTTP_SCODE_CONFILICT = 409,
	HTTP_SCODE_GONE = 410,
	HTTP_SCODE_LENGTH_REQUIRED = 411,
	HTTP_SCODE_PRECONDITION_FAILED = 412,
	HTTP_SCODE_REQUEST_ENTITY_TOO_LARGE = 413,
	HTTP_SCODE_REQUEST_URI_TOO_LONG = 414,
	HTTP_SCODE_UNSUPPORTED_MEDIA_TYPE = 415,
	HTTP_SCODE_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
	HTTP_SCODE_EXPECTATION_FAILED = 417,
	
	// Server Error 5xx
	HTTP_SCODE_INTERNAL_SERVER_ERROR = 500,
	HTTP_SCODE_NOT_IMPLEMENTED = 501,
	HTTP_SCODE_BAD_GATEWAY = 502,
	HTTP_SCODE_SERVICE_UNAVAILABLE = 503,
	HTTP_SCODE_GATEWAY_TIMEOUT = 504,
	HTTP_SCODE_HTTP_VERSION_NOT_SUPPORTED = 505
};

class ResponseBuilder
{
public:

	/// Client���� PROXY ������ �˸��� ������������ �۽�
	/**
	 * Multi Proxy ��å�� ����, �ٸ� Proxy ������ ������ �ؾ��ϴ� Client����
	 * �ٸ� Proxy�� �����ؾ� ���� �˸���, � Proxy�� �����ؾ��ϴ��� ������ ���� ������ ������
	 * Http Response �� �����Ѵ�.
	 *
	 * @note
	 * Client�� ���� �����ؾ��� Proxy ������ host/port�� AclRouter���� �����ϸ� ACL_DENY_ACL�� �����Ѵ�.
	 * ACL_DENY_ACL������ �� �Լ� ȣ�� ���ڷ� �ѱ� host�� port�� ���� �����ؾ��ϴ� Proxy ������ �ּҸ� ����� �ֹǷ�
	 * AclRouter�� ���� ���� Host/Port �� Client���� �����ϵ��� �����ϸ� �ȴ�.
	 *
	 * @see AclRouter::searchALL
	 *
	 * @param host Client�� ���� �����ؾ��� Proxy ������ host
	 * @param port Client�� ���� �����ؾ��� Proxy ������ port
	 **/
	static int ProxyChange(HTTP::Response*pResponse, const host_t& host, const int port);
	static int InvalidProxy_toME(HTTP::Response*pResponse);
	static int InvalidProxy_toKUN(HTTP::Response*pResponse);
	static int InvalidAgent(HTTP::Response*pResponse);	

	static int AuthFailed(HTTP::Response*pResponse);
	static int SantaFailed(HTTP::Response*pResponse);
	static int CpSSLConnSuccessed(HTTP::Response*pResponse);
	static int CpSSLConnFailed(HTTP::Response* pResponse);
	static int CpConnFailed(HTTP::Response*pResponse);
	static int CpTimeout(HTTP::Response*pResponse);
	static int DnsFail(HTTP::Response*pResponse);
	static int StatFilterBlocked(HTTP::Response*pResponse, char *body, int bodylen);
	static int TimeOutError(HTTP::Response*pResponse, const char *body, int bodylen, int resCode=408);
	static int Redirect(HTTP::Response*pResponse, const url_t& url);
	static int Forbidden(HTTP::Response*pResponse);

private:

};


#endif
