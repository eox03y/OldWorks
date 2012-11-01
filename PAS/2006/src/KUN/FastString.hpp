#ifndef __FAST_STRING_HPP__
#define __FAST_STRING_HPP__

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <vector>
#include <algorithm>

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
		#ifndef NDEBUG
			initForDebug();
		#endif

		clear();
	}

	FastString(const char* str, const size_t size)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(str, size);
	}

	FastString(const char* str)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(str);
	}

	explicit FastString(const char value)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(value);
	}

	explicit FastString(const int value)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(value);
	}

	explicit FastString(const unsigned int value)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(value);
	}

	explicit FastString(const int64_t value)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(value);
	}

	explicit FastString(const float value)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(value);
	}

	explicit FastString(const double value)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(value);
	}

	// FastString(const FastString<SrcMaxSize>& str)�� ���� ������ �� �� �ִ� ��������
	// �Լ������� SunCC�� ���׷� ���� �� �Լ��� �߰��ؾ� �Ѵ�.
	FastString(const FastString<MaxSize>& str)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(str);
	}

	template<size_t SrcMaxSize>
	FastString(const FastString<SrcMaxSize>& str)
	{
		#ifndef NDEBUG
			initForDebug();
		#endif

		assign(str);
	}

	~FastString()
	{
		assert(valid());
	}

	char firstCh()
	{
		return buf[0];
	}

	/// Ư�� ��ġ�� ���� ��ȯ
	const char& operator [] (int index) const
	{
		assert(0 <= index);
		assert(index < static_cast<int>(bufLen));

		return buf[index];
	}

	/// Ư�� ��ġ�� ���� ��ȯ
	char& operator [] (int index)
	{
		assert(0 <= index);
		assert(index < static_cast<int>(bufLen));

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
	FastString<MaxSize>& operator = (const T1& rhs)
	{
		assign(rhs);
		return *this;
	}

	// FastString<MaxSize>& operator = (const T1& rhs)�� ���� ������ �� �� �ִ� ��������
	// �Լ������� SunCC�� ���׷� ���� �� �Լ��� �߰��ؾ� �Ѵ�.
	FastString<MaxSize>& operator = (const FastString<MaxSize>& rhs)
	{
		assign(rhs);
		return *this;
	}

	/// ���ڿ� ���� (null terminated string)
	operator const char* () const
	{
		return buf;
	}

	/// ���ڿ� ����
	int assign(const char* str)
	{
		clear();
		append(str);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const char* str, int size)
	{
		clear();
		append(str, size);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const char value)
	{
		clear();
		append(value);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const int value)
	{
		clear();
		append(value);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const unsigned int value)
	{
		clear();
		append(value);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const int64_t value)
	{
		clear();
		append(value);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const float value)
	{
		clear();
		append(value);
		return bufLen;
	}

	/// ���ڿ� ����
	int assign(const double value)
	{
		clear();
		append(value);
		return bufLen;
	}

	/// ���ڿ� ����
	template<size_t SrcMaxSize>
	int assign(const FastString<SrcMaxSize>& str)
	{
		clear();
		append(str);
		return bufLen;
	}

	/// ���ڿ� ���� (null terminated string)
	const char* toStr() const
	{
		return buf;
	}

	/// ���ڿ� ���� (null terminated string)
	const char* c_str() const
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
			// moveSize + nullChar
			memmove(buf, &buf[n], (bufLen-n)+1);

			bufLen -= n;
		}

		assert(valid());
		
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

		assert(valid());
		
		return bufLen;
	}

	/// ���ڿ� �ִ� ������
	static inline size_t maxSize()
	{
		return MaxSize;
	}

	/// ���ڿ� ����
	inline size_t size() const
	{
		return bufLen;
	}

	/// ���� ������
	inline size_t freeSize() const
	{
		return MaxSize - bufLen;
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
		if(bufLen < index)
			index = bufLen;

		if(MaxSize <= index)
			return bufLen;

		int rightStringLen = bufLen - index;

		// rightStringLen + nullChar
		copy(index+1, index, rightStringLen+1);

		buf[index] = ch;

		if(bufLen < MaxSize)
			++bufLen;

		assert(valid());

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
		assert(size <= strlen(str));

		if(str == NULL || size == 0)
			return bufLen;

		if(bufLen < index)
			index = bufLen;

		if(MaxSize <= index)
			return bufLen;

		int rightStringLen = bufLen - index;
		size_t moveDestIndex = index+size;

		if(moveDestIndex < MaxSize)
		{
			// rightStringLen + nullChar
			copy(moveDestIndex, index, rightStringLen+1);
		}

		int copiedSize = copy(index, str, size);

		bufLen = std::min(bufLen+copiedSize, MaxSize);

		assert(valid());

		return bufLen;
	}

	int insert(size_t index, const char* str)
	{
		return insert(index, str, strlen(str));
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 *
	 * @seealso insert
	 **/
	int append(const char ch)
	{
		return insert(bufLen, ch);
	}

	/// ���ڿ� �������� ���ڿ� ���̱�
	/**
	  * @return ���ڿ� ����
	 *
	 * @seealso insert
	 **/
	int append(const char* str, size_t size)
	{
		return insert(bufLen, str, size);
	}

	/// ���ڿ� �������� ���ڿ� ���̱�
	/**
	* @return ���ڿ� ����
	*
	* @seealso insert
	**/
	int append(const char* str)
	{
		return insert(bufLen, str);
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const int val)
	{
		char str[32];
		snprintf(str, sizeof(str), "%d", val);
		return append(str);
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const unsigned int val)
	{
		char str[32];
		snprintf(str, sizeof(str), "%u", val);
		return append(str);
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	* @return ���ڿ� ����
	**/
	int append(const int64_t val)
	{
		char str[32];
		snprintf(str, sizeof(str), "%lld", val);
		return append(str);
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const float val)
	{
		char str[32];
		snprintf(str, sizeof(str), "%f", val);
		return append(str);
	}

	/// ���ڿ� �������� ���� ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	int append(const double val)
	{
		char str[32];
		snprintf(str, sizeof(str), "%lf", val);
		return append(str);
	}

	/// ���ڿ� �������� FastString ���̱�
	/**
	 * @return ���ڿ� ����
	 **/
	template<size_t SrcMaxSize>
	int append(const FastString<SrcMaxSize>& value)
	{
		return append(value.c_str(), value.size());
	}

	/// ���ڿ� ����
	/**
	 * @param index ������ ���ڿ��� ��ġ (zero based index)
	 * @param size ������ ���ڿ� ����
	 * @return ���ڿ� ����
	 **/
	int erase(size_t index, size_t size = 0xffffffff)
	{
		if(bufLen <= index)
			return bufLen;

		size_t rightStringLen = bufLen - index;
		if(size >= rightStringLen)
		{
			buf[index] = '\0';
			bufLen = index;
		}
		else
		{
			// + nullChar
			copy(index, index+size, rightStringLen+1);
			bufLen -= size;
		}

		assert(valid());
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
		int newBufLen = vsnprintf(buf, MaxSize+1, fmt, args);
		va_end(args);

		if(newBufLen < 0)
		{
			bufLen = 0;
			buf[0] = '\0';
		}
		else
		{
			bufLen = std::min(MaxSize, static_cast<size_t>(newBufLen));
		}

		assert(valid());

		return bufLen;
	}

	int append_sprintf(const char *fmt, ...)
	{
		char newString[MaxSize+1];

		va_list args;
		va_start(args, fmt);
		vsnprintf(newString, MaxSize, fmt, args);
		va_end(args);

		return append(newString);;
	}

	/// ���ڿ� ����
	/**
	 * @param index ������ ���ڿ� ��ġ(zero based index)
	 * @param size ������ ���ڿ� ����
	 * @return ����� ���ڿ� (���ڿ��� ��� �ִ� FastString�� ������ �������� FastString�� ����)
	 **/
	FastString<MaxSize> substr(size_t index, size_t size = 0xffffffffU) const
	{
		FastString<MaxSize> str;
		if(index >= bufLen)
			return str;

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
	int substr(FastString<SrcMaxSize>* pDestStr, size_t index, size_t size = 0xffffffffU) const
	{
		pDestStr->clear();
		if(index >= bufLen)
			return -1;

		size_t substrSize = std::min(bufLen - index, size);
		pDestStr->append(&buf[index], substrSize);
		return pDestStr->size();
	}

	template <size_t SrcMaxSize>
	int substr(FastString<SrcMaxSize>& destStr, size_t index, size_t size = 0xffffffffU) const
	{
		return substr(&destStr, index, size);
	}

	/// ���� ã��
	/**
	 * @param ch ã�� ����
	 * @return ���� ���� ��ġ(zero based index), ������ -1
	 **/
	int find(const char ch, size_t startPos = 0) const
	{
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
		if(bufLen <= startPos)
			return -1;

		const char* pPos = strstr(&buf[startPos], str);

		if(pPos == NULL)
			return -1;

		int findPos = (int)(pPos - buf);

		return findPos;
	}

	/// ��ҹ��ڸ� �������� �ʴ� ���ڿ� ã��
	/**
	* @param str ã�� ���ڿ� ����
	* @return ���ڿ� ���� ��ġ(zero based index), ������ -1
	**/
	int incaseFind(const char* str, size_t startPos = 0) const
	{
		if(bufLen <= startPos)
			return -1;

		const char* pPos = strcasestr(&buf[startPos], str);

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
		pDestStr->clear();

		if(bufLen <= startPos)
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
			assert(pos > (int)startPos);
			substr(pDestStr, startPos, pos-startPos);
		}

		return pos;
	}

	template <size_t SrcMaxSize>
	int split(FastString<SrcMaxSize>& destStr, const char delimiter, size_t startPos = 0) const
	{
		return split(&destStr, delimiter, startPos);
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
		FastString<MaxSize> str;

		if(startPos >= bufLen)
			return str;

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
		pDestStr->clear();

		if(startPos >= bufLen)
			return -1;

		int pos = find(delimiter, startPos);
		if(pos < 0)
		{
			substr(pDestStr, startPos);
			return bufLen;
		}
		else if(pos == static_cast<int>(startPos))
		{
			pDestStr->clear();
		}
		else
		{
			assert(pos > (int)startPos);
			substr(pDestStr, startPos, pos-startPos);
		}

		return pos;
	}

	template <size_t SrcMaxSize>
	int split(FastString<SrcMaxSize>& destStr, const char* delimiter, size_t startPos = 0) const
	{
		return split(&destStr, delimiter, startPos);
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
		pDestStr->clear();

		if(startPos >= bufLen)
			return -1;

		int pos = split(pDestStr, '\n', startPos);
		if(pos < 0)
			return -1;

		size_t len = pDestStr->size();
		if(len > 0)
		{
			if((*pDestStr)[len-1] == '\r')
			{
				pDestStr->erase(len-1, 1);
			}
		}

		return pos;
	}

	template <size_t SrcMaxSize>
	int getLine(FastString<SrcMaxSize>& destStr, size_t startPos = 0) const
	{
		return getLine(&destStr, startPos);
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
		FastString<MaxSize> fastStr;
		getLine(&fastStr, startPos);
		return fastStr;
	}

	/// ���ڿ��� ��� ����
	int clear()
	{
		bufLen = 0;
		buf[0] = '\0';
		
		assert(valid());
		return 0;
	}

	/// ���ڿ� ġȯ
	void replace(const char* niddle, const char* changeTo)
	{
		int pos = find(niddle);
		if(pos < 0)
			return;

		FastString<MaxSize> destStr;

		int begin = 0;
		int end = pos;
		int lenNiddle = strlen(niddle);

		while(end >= 0)
		{
			destStr.append(&buf[begin], end-begin);
			destStr.append(changeTo);
			begin = end+lenNiddle;
			end = find(niddle, begin);
		}

		// �ڿ� ���� �κ� �ٿ� �ֱ�
		if(begin < static_cast<int>(size()))
		{
			destStr.append(&buf[begin], size()-begin);
		}

		assign(destStr);
	}

	/// ���ڿ��� standard output���� ����Ѵ�.
	inline int print() const
	{
		return printf("%s", buf);
	}

	/// ���ڿ��� NewLine�� standard output���� ����Ѵ�.
	inline int println() const
	{
		return printf("%s\n", buf);
	}

	inline bool isEmpty() const
	{
		return bufLen == 0;
	}

	inline const char* front() const
	{
		return buf;
	}

	inline char* front()
	{
		return buf;
	}

private:
	/// ����
	int copy(size_t destIndex, const char* str, size_t copySize)
	{
		int overSize = static_cast<int>(destIndex + copySize) - static_cast<int>(MaxSize);

		if(overSize > 0)
		{
			copySize -= overSize;
		}

		memmove(&buf[destIndex], str, copySize);
		
		return copySize;
	}

	/// ����
	int copy(size_t destIndex, size_t srcIndex, size_t copySize)
	{
		int overSize = static_cast<int>(destIndex + copySize) - static_cast<int>(MaxSize);
		if(overSize > 0)
		{
			copySize -= overSize;
		}

		memmove(&buf[destIndex], &buf[srcIndex], copySize);

		return copySize;
	}

	bool valid()
	{
		#ifdef NDEBUG
			return strlen(buf) == bufLen;
		#else
			return (buf[MaxSize] == '\0' && strlen(buf) == bufLen);
		#endif
	}

	void initForDebug()
	{
		memset(buf, '#', sizeof(buf));
		buf[MaxSize] = '\0';
	}

	// ��ҹ��ڸ� �������� �ʴ� ���ڿ� ã��
	const char * strcasestr (const char *haystack, const char *needle) const
	{
		const char *p, *startn = 0, *np = 0;

		for (p = haystack; *p; p++) {
			if (np) {
				if (toupper(*p) == toupper(*np)) {
					if (!*++np)
						return startn;
				} else
					np = 0;
			} else if (toupper(*p) == toupper(*needle)) {
				np = needle + 1;
				startn = p;
			}	
		}

		return 0;
	}


private:
	char buf[MaxSize+1];///< ���ڿ� ���� (null terminated string)
	size_t bufLen;		///< ���ڿ� ����
};

template <typename FASTSTRING>
std::vector<FASTSTRING> explode(const FASTSTRING& str, const char delimiter)
{
	std::vector<FASTSTRING> strings;

	FASTSTRING tmpStr;
	int pos = -1;
	while(true)
	{
		pos = str.split(&tmpStr, delimiter, pos+1);
		if(pos < 0)
			return strings;

		strings.push_back(tmpStr);
	}
}

#endif

