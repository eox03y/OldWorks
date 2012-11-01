#include "HttpBaseHeader.h"
#include "PasLog.h"
#include "Util.h"

using namespace HTTP;

BaseHeader::BaseHeader(void)
{
	elements.reserve(30);
}

BaseHeader::~BaseHeader(void)
{
	clear();
}

/**
HTTP header�� body�� ���� ��ġ�� ã��, header�� ���̸� return.
header�� body�� ���й��ڿ�: "\r\n\r\n", "\n\n", "\r\n\n" �� ��� ó���� �� �ֵ��� �Ѵ�. 
header�� �濡���� ���� ���ڿ� ���̱��� �����Ѵ�.
@return -1 if not found
*/
int	BaseHeader::searchHeaderEnd(const char* src, size_t srcSize)
{
	if (srcSize == 0) 
		return 0;

	const char* srcPtr = src;
	size_t	n=0;
	while (n < srcSize) 
	{
		if (srcPtr[0]=='\n')
		{
			if (n+1 < srcSize && srcPtr[1]=='\n') 
			{
				return (int)n + 2;
			}
			else if (n+2 < srcSize && srcPtr[1]=='\r'  && srcPtr[2]=='\n') 
			{
				return (int)n + 3;
			}
		}
		srcPtr++;
		n++;
	}
	return 0;
}

/**
������ ���� ����Ÿ���� http header �κб����� �߶� FastString (header_t) �� �����Ѵ�.
*/
int BaseHeader::getHeader(header_t* pDest, const char* srcBuf, size_t srcBufSize)
{
	ASSERT(pDest != NULL);
	ASSERT(srcBuf != NULL);
	ASSERT(srcBufSize > 0);

	pDest->clear();

	const size_t headerSize = BaseHeader::searchHeaderEnd(srcBuf, srcBufSize);
	if (headerSize == 0) 
		return -1;

	// ��� �߰��� NULL �� ����ִ��� Ȯ���Ѵ�. (NULL�� ������ �ȵȴ�.)
	if(memchr(srcBuf, 0x00, headerSize) != NULL)
	{
		PAS_NOTICE("NULL char in request header.");
		PAS_NOTICE_DUMP("Request header", srcBuf, headerSize);			
		return -1;
	}

	pDest->append(srcBuf, headerSize);
	
	return 0;
}

/**
�� ������ ����� �Ľ��Ͽ� key/value �� ���Ѵ�.
���� ��Ȳ)  key�� ���� ���.  ":" �� ���� ���. 
2006-11-29
*/
int	BaseHeader::parseElement(const line_t& line)
{
	// ����� ":" �� ���� ���.
	if(line.find(':') < 0)
		return -1;

	key_t key;
	int pos = line.split(&key, ':');
	key.trim();

	// key �� ���� ���
	if(key.isEmpty())
		return -1;

	// extract value
	if(pos+1 < static_cast<int>(line.size()))
	{				
		value_t value;
		line.substr(&value, pos+1);
		value.trim();
		addElement(key, value);
	}

	// value is empty
	else
	{
		value_t value;
		addElement(key, value);
	}

	return 0;
}

/**
Ư�� ����� �� �տ� �ְ� �ʹ�.
*/
int BaseHeader::addElementAtFront(const key_t& key, const value_t& value)
{
	// add
	elements.insert(elements.begin(), HeaderElement(key, value));

	//PAS_TRACE2("addElementAtFront(): key[%s] value[%s]", key.toStr(), value.toStr());

	return 0;
}

/**

������ addElement() �� ���ο� Element �� �߰��ϴ� ���̾���.
������ search ���� ������ insert. 
���� Element �� �ߺ��� �� �ִ�.   --> Set-Cookie ��
*/
int BaseHeader::addElement(const key_t& key, const value_t& value)
{
	// add
	
	if (key.incaseEqual("Host"))
		addElementIfNot(key, value);	
	else
	
		elements.push_back(HeaderElement(key, value));

	//PAS_TRACE2("addElement(): key[%s] value[%s]", key.toStr(), value.toStr());

	return 0;
}

/**

������ addElement() �� �̸��� addElementIfNot ���� ����.
*/
int BaseHeader::addElementIfNot(const key_t& key, const value_t& value)
{
	HeaderElement* pElem = getElementPtr(key);

	// key exist
	if(pElem != NULL)
	{
		PAS_TRACE1("key[%s] is exist", (const char*)key);
		return -1;
	}

	// add
	elements.push_back(HeaderElement(key, value));

	//PAS_TRACE2("put header elem key[%s] value[%s]", key.toStr(), value.toStr());

	return 0;
}


int BaseHeader::replaceElement(const key_t& key, const value_t& value)
{
	HeaderElement* pElem = getElementPtr(key);

	// key exist
	if(pElem != NULL)
	{
		//PAS_TRACE2("replaceElement(): Replace key[%s] value[%s]", key.toStr(), value.toStr());
		// replace
		pElem->value = value;
	}

	// not exist
	else
	{
		//PAS_TRACE2("replaceElement(): Add key[%s] value[%s]", key.toStr(), value.toStr());
		// add
		elements.push_back(HeaderElement(key, value));
	}

	return 0;
}

int BaseHeader::delElement(const key_t& key)
{
	HeaderElements::iterator it = elements.begin();
	HeaderElements::iterator itE = elements.end();

	for( ; it != itE; ++it)
	{
		if(it->key.incaseEqual(key))
		{
			elements.erase(it);
			return 0;
		}
	}

	return -1;
}

value_t BaseHeader::getElement(const key_t& key) const
{
	const HeaderElement* pElem = getElementPtr(key);
	if(pElem == NULL)
		return value_t();

	return pElem->value;
}

/**
key�� ���� value��  ����(FastString)�Ǵ� ���� �ƴ϶�, �־��� buffer�� �������ش�.
@param value_buf  ���� ������ ����.
@param buf_len NULL ���ڸ� ������ ������ ���̿��� �Ѵ�.
@return 1 if key found, 0 if not found.
*/
int BaseHeader::getElement(const key_t& key, char *value_buf, int buf_len) const
{
	const HeaderElement* pElem = getElementPtr(key);
	if(pElem == NULL) {
		value_buf[0] = '\0';
		return 0;
	}
	else {
		strncpy(value_buf, pElem->value.toStr(), buf_len);
		value_buf[buf_len] = '\0';
		return 1;
	}
}


const HeaderElement* BaseHeader::getElementPtr(const key_t& key) const
{
	HeaderElements::const_iterator it = elements.begin();
	HeaderElements::const_iterator itE = elements.end();

	for( ; it != itE; ++it)
	{
		if(it->key.incaseEqual(key))
			return &(*it);
	}

	return NULL;
}

HeaderElement* BaseHeader::getElementPtr(const key_t& key)
{
	HeaderElements::iterator it = elements.begin();
	HeaderElements::iterator itE = elements.end();

	for( ; it != itE; ++it)
	{
		if(it->key.incaseEqual(key))
			return &(*it);
	}

	return NULL;
}


/**
getElement �� �̸��� ��ġ�ϴ� ���� ã�´� ( ��ҹ��� ����)
wildSearchElement �� �־��� key �� �����ϴ� ���ڿ��̸� true.
*/
value_t BaseHeader::wildSearchElement(const key_t& key)
{
	HeaderElements::iterator it = elements.begin();
	HeaderElements::iterator itE = elements.end();

	for( ; it != itE; ++it)
	{
		if(strstr(it->key.toStr(), key.toStr()) != NULL)
			return it->value;
	}

	return value_t();
}

/**
��� ��� �߿� wildkey ���ڿ��� �����ϴ� key �� value �� return �ϸ�, �߰ߵ� key�� ���� matchedkey �� �����Ѵ�.
2006.12.8
*/
value_t BaseHeader::wildSearchElement_getkey(const key_t& wildkey, char *matchedkey, int matchkeylen)
{
	HeaderElements::iterator it = elements.begin();
	HeaderElements::iterator itE = elements.end();

	for( ; it != itE; ++it)
	{
		if(strstr(it->key.toStr(), wildkey.toStr()) != NULL)
		{
			strncpy(matchedkey, it->key.toStr(), matchkeylen);
			matchedkey[matchkeylen] = 0;
			return it->value;
		}
	}

	
	return value_t();
}

void BaseHeader::clear()
{
	if (!elements.empty())
		elements.clear();
}

Keys BaseHeader::getKeys() const
{
	Keys keys;
	keys.reserve(elements.size());

	HeaderElements::const_iterator it = elements.begin();
	HeaderElements::const_iterator itE = elements.end();

	for( ; it != itE; ++it)
	{
		keys.push_back(it->key);
	}

	return keys;
}

int BaseHeader::buildAllLines(header_t* pDestHeader) const
{
	ASSERT(pDestHeader != NULL);

	HeaderElements::const_iterator it = elements.begin();
	HeaderElements::const_iterator itE = elements.end();

	line_t line;
	bool host_found = false;
	for( ; it != itE; ++it)
	{
		// elements �� host ������ ������ ���� ���, �ߺ� ������ ���� ����
		if (it->key.incaseEqual("Host"))
		{
			if(host_found)
				continue;
			
			host_found = true;
		}

		// ���� �ϳ��� �־� ����. KTF. CP ���� ���� ������ �־��ش�.
		// ���� ���� ����. (2006.10.11 �絿�� ����� �ǰ�. HASH_KEY element ������)
		line.sprintf("%s: %s", it->key.toStr(), it->value.toStr());

		if(line.freeSize() == 0)
			PAS_INFO2("Header element is too big. KEY[%s] VALUE[%s]", it->key.toStr(), it->value.toStr());
		
		// �뷮 �ʰ�?
		// +2 for carriage return and new line
		if(pDestHeader->freeSize() < line.size() + 2)
		{
			PAS_NOTICE("Not enough space for write http header.");
			continue;
		}
		
		pDestHeader->append(line);
		pDestHeader->append("\r\n");
	}

	return 0;
}
