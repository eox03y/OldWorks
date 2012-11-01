#ifndef __FAST_STRING_HPP__
#define __FAST_STRING_HPP__

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>

#include "basicDef.h"

#ifndef IS_WHITE_SPACE
#define	IS_WHITE_SPACE(X)	((X)==' ' || (X)=='\t' || (X)=='\n' || (X)=='\r')
#endif

/// ���������� ���ڿ� �ڵ鸵 Ŭ����
/**
 * ���������� ���� �޸� �Ҵ��� �ϴ� ���������� ���ڿ� �ڵ鸵 Ŭ�����ʹ� �޸�
 * ���������� ���۸� ����ϹǷμ� �����Ҵ��� �ð��� ������ ���ڿ� �ڵ鸵 Ŭ����
 *
 * ���� �����÷ο찡 �߻����� �ʵ��� ���������� ������ üũ�� �Ѵ�.
 **/
template<size_t MaxSize>
class FastString
{
public:
	FastString()
	{
		clear();

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
	}

	FastString(const char* str, size_t size)
	{
		clear();
		this->insert(0, str, size);

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
	}

	FastString(const char* str)
	{
		clear();
		this->insert(0, str);

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
	}

	template<typename T1>
	FastString(const T1& value)
	{
		clear();
		this->assign(value);

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
	}

	~FastString()
	{
//		ASSERT(buf[MaxSize-1] == '\0');
//		ASSERT(strlen(buf) == bufLen);
	}

	char firstCh()
	{
		if (bufLen > 0)
			return buf[0];
		else
			return '\0';
	}

	/*
	/// Ư�� ��ġ�� ���� ��ȯ
	const char& operator [] (size_t index) const
	{
		ASSERT(index < bufLen);

		return buf[index];
	}
	*/

	/// Ư�� ��ġ�� ���� ��ȯ
	char& operator [] (size_t index)
	{
		ASSERT(index < bufLen);

		return buf[index];
	}

	/// ��ҹ��ڸ� �����ϴ� ���ڿ� ��
	/**
	 * @seealso casecmp
	 **/
	bool operator == (const char* rhs) const
	{
		return strcmp(buf, rhs) == 0;
	}

	/// ��ҹ��ڸ� �����ϴ� ���ڿ� ��
	bool operator != (const char* rhs) const
	{
		return strcmp(buf, rhs) != 0;
	}

	/// ���ڿ� ũ�� ��
	bool operator < (const char* rhs) const
	{
		return strcmp(buf, rhs) < 0;
	}

	/// ���ڿ� ���ϱ�
	template<typename T1>
	FastString<MaxSize>& operator += (const T1& value)
	{
		this->append(value);
		return *this;
	}

	/// ���ڿ� ���ϱ�
	template<typename T1>
	FastString<MaxSize> operator + (const T1& rhs) const
	{
		FastString<MaxSize> newString(*this);
		newString.append(rhs);
		return newString;
	}

	/// ���ڿ� ����
	template<typename T1>
	FastString<MaxSize> operator = (const T1& rhs)
	{
		this->assign(rhs);
		return *this;
	}


	/// ���ڿ� ���� (null terminated string)
	operator const char* () const
	{
		return buf;
	}

	/// ���ڿ� ����
	template<typename T1>
	int assign(const T1& value)
	{
		clear();
		this->append(value);
		return bufLen;
	}

	/// ���ڿ� ���� (null terminated string)
	const char* toStr() const
	{
		return buf;
	}

	/// ���ڿ��� int�� ��ȯ
	int toInt() const
	{
		return atoi(buf);
	}

	/// ���ڿ��� float�� ��ȯ
	float toFloat() const
	{
		return atof(buf);
	}

	/// ���ڿ��� double�� ��ȯ
	double toDouble() const
	{
		return atof(buf);
	}

	/// ��ҹ��� ������ ���� ���ڿ� ��
	/**
	 * @return ������ true, �ٸ��� false
	 **/
	bool incaseEqual (const char* rhs) const
	{
		return strcasecmp(buf, rhs) == 0;
	}

	/// ���ڿ��� �¿� ���� ����
	int trim()
	{
		trimRight();
		trimLeft();
		return bufLen;
	}

	/// ���ڿ��� ���� ���� ����
	int trimLeft()
	{
		if(bufLen == 0)
			return 0;

		char* str = buf;
		size_t n;
		for(n=0; n<bufLen; ++str, ++n)
		{
			if(!IS_WHITE_SPACE(*str))
				break;
		}

		if(n > 0)
		{
			// (bufLen-n) + [null char size = 1]
			memmove(buf, &buf[n], (bufLen-n)+1);

			bufLen -= n;
		}

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return bufLen;
	}

	/// ���ڿ� ������ ���� ����
	int trimRight()
	{
		if(bufLen == 0)
			return 0;

		char* str = &buf[bufLen-1];
		int n;
		for(n=bufLen-1; n>=0; --str, --n)
		{
			if(!IS_WHITE_SPACE(*str))
				break;
		}

		n+= 1;
		buf[n] = '\0';
		bufLen = n;

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return bufLen;
	}

	/// ���ڿ� �ִ� ������
	/**
	 * ������ �������� null�� �߰��ǹǷ�, Ŭ���� �����Ҷ��� MaxSize ���� 1 �۴�.
	 **/
	static size_t maxSize()
	{
		return MaxSize-1;
	}

	/// ���ڿ� ����
	size_t size() const
	{
		return bufLen;
	}

	/// ���� ������
	size_t freeSize() const
	{
		return maxSize() - size();
	}

	/// ���� ����
	/**
	 * ���۰� ���� á�� ��� ���ڿ��� �޺κ��� ������ ���ڸ� �����Ѵ�.
	 *
	 * @param index ������ ��ġ(zero based index)
	 * @param ch ������ ����
	 * @return ���ڿ� ����
	 **/
	int insert(size_t index, char ch)
	{
		ASSERT(index < MaxSize-1);

		int rightStringLen = size - index;

		if(index < MaxSize-2)
		{
			// rightStringLen + [null char size = 1]
			copy(index+1, index, rightStringLen+1);
		}

		buf[index] = ch;
		++bufLen;

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return bufLen;
	}

	/// ���ڿ� ����
	/**
	 * ������ ���ڿ��� null terminated string �̸� null�� ������ �������� ���ڿ��� �����Ѵ�.
	 * ������ ���ڿ��� null terminated string�� �ƴ϶�� size ���� ���� ���̸� ���� �� �� �ִ�.
	 * size �� ���� ������ ���ڿ��� ���̸� ���� �� �� �ִ�.
	 * ���۰� ���� á�� ��� ���ڿ��� �޺κ��� ������ ���ڿ��� �����Ѵ�.
	 *
	 * @param index ������ ��ġ(zero based index)
	 * @param str ������ ���ڿ�
	 * @param size ������ ���ڿ��� ������
	 * @return ���ڿ� ����
	 **/
	int insert(size_t index, const char* str, size_t size)
	{
		ASSERT(index <= bufLen);
		ASSERT(strlen(buf) == bufLen);

		if(size == 0)
			return bufLen;

		int rightStringLen = bufLen - index;
		size_t moveDestIndex = index+size;

		if(moveDestIndex < MaxSize-1)
		{
			// rightStringLen + [null char size = 1]
			copy(moveDestIndex, index, rightStringLen+1);
		}

		int copiedSize = copy(index, str, size);

		bufLen = std::min(bufLen+copiedSize, MaxSize-1);

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(buf[bufLen] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return bufLen;
	}

	int insert(size_t index, const char* str)
	{
		ASSERT(index <= bufLen);
		insert(index, str, strlen(str));
		return bufLen;
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 *
	 * @seealso insert
	 **/
	int append(const char ch)
	{
		ASSERT(strlen(buf) == bufLen);
		insert(bufLen, ch);
		return bufLen;
	}

	/// ���ڿ� �������� ���ڿ� ���̱�
	/**
	 * @return 0
	 *
	 * @seealso insert
	 **/
	int append(const char* str, size_t size)
	{
		ASSERT(strlen(buf) == bufLen);
		insert(bufLen, str, size);
		return bufLen;
	}

	int append(const char* str)
	{
		ASSERT(strlen(buf) == bufLen);
		insert(bufLen, str);
		return bufLen;
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const int val)
	{
		ASSERT(strlen(buf) == bufLen);
		char str[32];
		snprintf(str, sizeof(str), "%d", val);
		append(str);
		return bufLen;
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const unsigned int val)
	{
		ASSERT(strlen(buf) == bufLen);
		char str[32];
		snprintf(str, sizeof(str), "%u", val);
		append(str);
		return bufLen;
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const float val)
	{
		ASSERT(strlen(buf) == bufLen);
		char str[32];
		snprintf(str, sizeof(str), "%f", val);
		append(str);
		return bufLen;
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const double val)
	{
		ASSERT(strlen(buf) == bufLen);
		char str[32];
		snprintf(str, sizeof(str), "%lf", val);
		append(str);
		return bufLen;
	}

	/// ���ڿ� �������� FastString ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	template<size_t SrcMaxSize>
	int append(const FastString<SrcMaxSize>& value)
	{
		ASSERT(strlen(buf) == bufLen);
		this->append(value.toStr());
		return bufLen;
	}

	/// ���ڿ� ����
	/**
	 * @param index ������ ���ڿ��� ��ġ (zero based index)
	 * @param size ������ ���ڿ� ����
	 * @return ���ڿ� ����
	 **/
	int erase(size_t index, size_t size = 0xffffffff)
	{
		ASSERT(strlen(buf) == bufLen);
		ASSERT(index < MaxSize-1);

		size_t rightStringLen = bufLen - index;
		if(size >= rightStringLen)
		{
			buf[index] = '\0';
			bufLen = index;
		}
		else
		{
			// size + [null char size = 1]
			copy(index, index+size, size+1);
			bufLen -= size;
		}

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return bufLen;
	}

	/// �����ִ� ���ڿ� ���
	/**
	 * C ǥ�� �Լ��� sprintf �� �����ϴ�.
	 * @return ���ڿ� ����
	 **/
	int sprintf(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		bufLen = vsnprintf(buf, MaxSize, fmt, args);
		va_end(args);

		if(bufLen > MaxSize-1)
			bufLen = MaxSize-1;

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return bufLen;
	}

	/// ���ڿ� ����
	/**
	 * @param index ������ ���ڿ� ��ġ(zero based index)
	 * @param size ������ ���ڿ� ����
	 * @return ����� ���ڿ� (���ڿ��� ��� �ִ� FastString�� ������ �������� FastString�� ����)
	 **/
	FastString<MaxSize> substr(size_t index, size_t size = 0xffffffff) const
	{
		ASSERT(strlen(buf) == bufLen);
		ASSERT(index < MaxSize-1);

		FastString<MaxSize> str;
		size_t substrSize = std::min(bufLen - index, size);
		substr(&str, index, substrSize);

		return str;
	}

	/// ���ڿ� ����
	/**
	 * @param pDestStr ������ ���ڸ� ���� FastString
	 * @param index ������ ���ڿ� ��ġ(zero based index)
	 * @param size ������ ���ڿ� ����
	 * @return ����� ���ڿ� ����
	 **/
	template <size_t SrcMaxSize>
	int substr(FastString<SrcMaxSize>* pDestStr, size_t index, size_t size = 0xffffffff) const
	{
		ASSERT(strlen(buf) == bufLen);
		ASSERT(pDestStr != NULL);
		ASSERT(index < MaxSize-1);

		pDestStr->clear();
		size_t substrSize = std::min(bufLen - index, size);
		pDestStr->append(&buf[index], substrSize);
		return pDestStr->size();
	}

	/// ���� ã��
	/**
	 * @param ch ã�� ����
	 * @return ���� ���� ��ġ(zero based index), ������ -1
	 **/
	int find(const char ch, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		if(startPos >= bufLen)
			return -1;

		char* pPos = (char*)strchr(&buf[startPos], ch);

		if(pPos == NULL)
			return -1;

		int findPos = (int)(pPos - buf);
		return findPos;
	}

	/// ���ڿ� ã��
	/**
	 * @param str ã�� ���ڿ� ����
	 * @return ���ڿ� ���� ��ġ(zero based index), ������ -1
	 **/
	int find(const char* str, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		if(startPos >= bufLen)
			return -1;

		const char* pPos = strstr(&buf[startPos], str);

		if(pPos == NULL)
			return -1;

		int findPos = (int)(pPos - buf);

		return findPos;
	}

	/// ��ū ���ڰ� �ƴ� �� ã��
	/**
	 * @param ch ��ū ����
	 * @return ���� ���� ��ġ(zero based index), ������ -1
	 **/
	int findNotOf(const char ch, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		if(startPos >= bufLen)
			return -1;

		char* pCh = &buf[startPos];
		while(*pCh != '\0')
		{
			if(*pCh != ch)
				break;

			++pCh;
		}

		if(*pCh == '\0')
			return -1;

		int findPos = (int)(pCh - buf);
		return findPos;
	}

	/// ��ū ���ڿ��� �ƴ� �� ã��
	/**
	 * ��ū ���ڿ��� �����ϴ� ���ڰ� �ƴ� ���ڰ� ó�� ��Ÿ���� ��
	 *
	 * @param str ��ū ���ڿ� ����
	 * @return ���ڿ� ���� ��ġ(zero based index), ������ -1
	 **/
	int findNotOf(const char* str, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		if(startPos >= bufLen)
			return -1;

		const char* pCh = &buf[startPos];
		while(*pCh != '\0')
		{
			if(strchr(str, *pCh) == NULL)
				break;

			++pCh;
		}

		if(*pCh == '\0')
			return -1;

		int findPos = (int)(pCh - buf);
		return findPos;
	}

	/// ��ū�� �������� �� ���ڿ� ����
	/**
	 * startPos ���� startPos ���Ŀ� �߰ߵǴ� ù ��ū�� �ٷ� �ձ����� ���ڿ��� �����Ѵ�.
	 * startPos ���Ŀ� ��ū�� �������� �ʴ´ٸ�, startPos ���� ������������ ���ڿ��� �����ϰ�, ���ڿ� �������� �ִ� null ������ index�� �����Ѵ�.
	 *
	 * @param pDestStr ������ ���ڸ� ���� FastString
	 * @param delimiter ��ū(����)
	 * @param startPos ���� ���� ��ġ (zero based index)
	 * @return ��ū ��ġ (zero based index) �Ǵ� null ������ index
	 **/
	template <size_t SrcMaxSize>
	int split(FastString<SrcMaxSize>* pDestStr, const char delimiter, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		pDestStr->clear();

		if(startPos >= bufLen)
			return -1;

		int pos = find(delimiter, startPos);
		if(pos < 0)
		{
			substr(pDestStr, startPos);
			return bufLen;
		}
		else if(pos == (int)startPos)
		{
			pDestStr->clear();
		}
		else
		{
			ASSERT(pos > (int)startPos);
			substr(pDestStr, startPos, pos-startPos);
		}

		return pos;
	}

	/// ��ū�� �������� �� ���ڿ� ����
	/**
	 * startPos ���� startPos ���Ŀ� �߰ߵǴ� ù ��ū�� �ٷ� �ձ����� ���ڿ��� �����Ѵ�.
	 * startPos ���Ŀ� ��ū�� �������� �ʴ´ٸ�, startPos ���� ������������ ���ڿ��� �����ϰ�, ���ڿ� �������� �ִ� null ������ index�� �����Ѵ�.
	 *
	 * @param delimiter ��ū(����)
	 * @param startPos ���� ���� ��ġ (zero based index)
	 * @return ����� ���ڿ� (���ڿ��� ��� �ִ� FastString�� ������ �������� FastString�� ����)
	 **/
	FastString<MaxSize> split(const char delimiter, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		if(startPos >= bufLen)
			return -1;

		FastString<MaxSize> str;
		split(&str, delimiter, startPos);
		return str;
	}

	/// ��ū�� �������� �� ���ڿ� ����
	/**
	 * startPos ���� startPos ���Ŀ� �߰ߵǴ� ù ��ū�� �ٷ� �ձ����� ���ڿ��� �����Ѵ�.
	 * startPos ���Ŀ� ��ū�� �������� �ʴ´ٸ�, startPos ���� ������������ ���ڿ��� �����ϰ�, ���ڿ� �������� �ִ� null ������ index�� �����Ѵ�.
	 *
	 * @param pDestStr ������ ���ڸ� ���� FastString
	 * @param delimiter ��ū(���ڿ� - null terminated string)
	 * @param startPos ���� ���� ��ġ (zero based index)
	 * @return ��ū ��ġ (zero based index) �Ǵ� null ������ index
	 **/
	template <size_t SrcMaxSize>
	int split(FastString<SrcMaxSize>* pDestStr, const char* delimiter, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		pDestStr->clear();

		if(startPos >= bufLen)
			return -1;

		int pos = find(delimiter, startPos);
		if(pos < 0)
		{
			substr(pDestStr, startPos);
			return bufLen;
		}
		else if(pos == startPos)
		{
			pDestStr->clear();
		}
		else
		{
			ASSERT(pos > startPos);
			substr(pDestStr, startPos, pos-startPos);
		}

		return pos;
	}

	/// ��ū�� �������� �� ���ڿ� ����
	/**
	 * startPos ���� startPos ���Ŀ� �߰ߵǴ� ù ��ū�� �ٷ� �ձ����� ���ڿ��� �����Ѵ�.
	 * startPos ���Ŀ� ��ū�� �������� �ʴ´ٸ�, startPos ���� ������������ ���ڿ��� �����ϰ�, ���ڿ� �������� �ִ� null ������ index�� �����Ѵ�.
	 *
	 * @param delimiter ��ū(���ڿ� - null terminated string)
	 * @param startPos ���� ���� ��ġ (zero based index)
	 * @return ����� ���ڿ� (���ڿ��� ��� �ִ� FastString�� ������ �������� FastString�� ����)
	 **/
	FastString<MaxSize> split(const char* delimiter, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		FastString<MaxSize> fastStr;
		split(&fastStr, delimiter, startPos);
		return fastStr;
	}

	/// line ����
	/**
	 * line �� ����� NewLine ("\n") �Ǵ� CarigeReturn + NewLine ("\r\n") �̴�.
	 * startPos ���� ���� ���� �߰ߵǴ� "\n"(or "\r\n")������ ���ڿ��� �����Ѵ�.
	 * ����� ���ڿ����� "\n" �̳� "\r\n"�� ���ԵǾ� ���� �ʴ�.
	 * startPos ���Ŀ� "\n"(or "\r\n")�� ���ٸ�, ���ڿ� �������� �ִ� null ������ index �� �����Ѵ�.
	 *
	 * @param pDestStr ������ ���ڸ� ���� FastString
	 * @param startPos ���� ���� ��ġ (zero based index)
	 * @return "\n"�� ��ġ (zero based index) �Ǵ� null ������ index
	 **/
	template <size_t SrcMaxSize>
	int getLine(FastString<SrcMaxSize>* pDestStr, size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		pDestStr->clear();

		if(startPos >= bufLen)
			return -1;

		int pos = split(pDestStr, '\n', startPos);
		if(pos < 0)
			return -1;

		size_t len = pDestStr->size();
		if(len > 0 && (*pDestStr)[len-1] == '\r')
		{
			pDestStr->erase(len-1, 1);
		}

		return pos;
	}

	/// line ����
	/**
	 * line �� ����� NewLine ("\n") �Ǵ� CarigeReturn + NewLine ("\r\n") �̴�.
	 * startPos ���� ���� ���� �߰ߵǴ� "\n"(or "\r\n")������ ���ڿ��� �����Ѵ�.
	 * ����� ���ڿ����� "\n" �̳� "\r\n"�� ���ԵǾ� ���� �ʴ�.
	 *
	 * @param pDestStr ������ ���ڸ� ���� FastString
	 * @param startPos ���� ���� ��ġ (zero based index)
	 * @return ����� ���ڿ� (���ڿ��� ��� �ִ� FastString�� ������ �������� FastString�� ����)
	 **/
	FastString<MaxSize> getLine(size_t startPos = 0) const
	{
		ASSERT(strlen(buf) == bufLen);
		FastString<MaxSize> fastStr;
		getLine(&fastStr, startPos);
		return fastStr;
	}

	/// ���ڿ��� ��� ����
	int clear()
	{
		#ifndef NDEBUG
			memset(buf, '#', sizeof(buf));
			buf[MaxSize-1] = '\0';
		#endif

		bufLen = 0;
		buf[0] = '\0';
		/* handol */
		buf[MaxSize-1] = '\0';
		buf[MaxSize-2] = '\0';
		buf[MaxSize-3] = '\0';
		buf[MaxSize-4] = '\0';

		ASSERT(buf[MaxSize-1] == '\0');
		ASSERT(strlen(buf) == bufLen);
		return 0;
	}

	/// ���ڿ��� standard output���� ����Ѵ�.
	int print() const
	{
		return printf("%s", buf);
	}

	/// ���ڿ��� NewLine�� standard output���� ����Ѵ�.
	int println() const
	{
		return printf("%s\n", buf);
	}

	bool isEmpty() const
	{
		return bufLen == 0;
	}

private:
	/// ����
	int copy(size_t destIndex, const char* str, size_t copySize)
	{
		ASSERT(destIndex < MaxSize-1);
		ASSERT(str != NULL);
		ASSERT(copySize > 0);

		int overSize = (destIndex + copySize) - (MaxSize-1);

		if(overSize > 0)
		{
			ASSERT(copySize > (size_t)overSize);
			copySize -= overSize;
		}

		memmove(&buf[destIndex], str, copySize);

		ASSERT(buf[MaxSize-1] == '\0');
		return copySize;
	}

	/// ����
	int copy(size_t destIndex, size_t srcIndex, size_t copySize)
	{
		ASSERT(destIndex < MaxSize-1);
		ASSERT(srcIndex < MaxSize-1);

		int overSize = (destIndex + copySize) - (MaxSize-1);
		if(overSize > 0)
		{
			ASSERT(copySize > (size_t)overSize);
			copySize -= overSize;
		}

		memmove(&buf[destIndex], &buf[srcIndex], copySize);

		ASSERT(buf[MaxSize-1] == '\0');
		return copySize;
	}

private:
	/* handol : 4 bytes more */
	char buf[MaxSize+4];	///< ���ڿ� ���� (null terminated string)
	size_t bufLen;		///< ���ڿ� ����
};

#endif

