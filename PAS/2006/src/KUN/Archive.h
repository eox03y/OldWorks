#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "Common.h"

#define MAX_RAW_PACKET_SIZE 4096

class Archive
{
public:
	Archive();
	Archive(const char* srcBuf, int size);

	~Archive();

	/// ������ ����
	/**
	 * ������ �ڿ� �����͸� �߰��Ѵ�.
	 * ����ϴ� �������� ������� ������ ����.
	 **/	
	int push(const char* srcBuf, int srcSize);
	int push(char ch);

	/// ������ �б�
	/**
	 * ������ �տ������� �����͸� �о� �´�.
	 *
	 * @return destBuf �� ��ϵ� ������ ������
	 **/
	int pop(char* destBuf, int popSize);

	/// ������ �б�
	/**
	* ������ �տ������� token �� �ִ� ������ �����͸� �о� �´�.
	* token ���ڴ� destBuf �� ��ϵ��� �ʴ´�.
	* 
	* @return destBuf �� ��ϵ� ������ ������
	**/
	int popFirstOf(char* destBuf, int bufSize, char token);

	/// ������ ����
	/**
	 * ������ ��ϵ� �����͸� �����ذ�, ���ο� �����ͷ� ���� ����.
	 **/
	void set(const char* srcBuf, int size);

	/// ������ �б�
	/**
	 * ����� ������ ����� bufSize ���� ũ�ٸ�, bufSize ������ �о� �´�.
	 *
	 * @param destBuf ����� ����
	 * @param bufSize destBuf �� ������
	 * @return destBuf �� ����� ������ ������
	 **/
	int get(char* destBuf, int bufSize);

	/// �������� ���� ������
	/**
	 * @return �������� ���� ������, empty �� ��� NULL
	 *
	 * @date 2007/02/22
	 * @author Sehoon Yang
	 **/
	char* front();
	const char* front() const;

	/// ������ ����
	void clear();

	/// ������ ������
	int size() const;

	/// ���� ����� �о� �´�.
	int getMaxSize();

	/// ���� ����� �����Ѵ�.
	void setMaxSize(int size);

	/// ���� ������ ������
	int readSize();

	/// read index �� �о� ����
	int getReadIdx();

	/// read index �� ���� �ϱ�
	void setReadIdx(int idx);

	/// write index �� �о� ����
	int getWriteIdx();

	/// write index �� ���� �ϱ�
	void setWriteIdx(int idx);

	/// ���� ����
	int space();

private:
	void init();

private:
	char _fixedBuf[MAX_RAW_PACKET_SIZE];
	char* _buf;
	int _bufSize;
	int _readIdx;
	int _writeIdx;
};

#endif
