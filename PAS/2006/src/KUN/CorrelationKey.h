#ifndef __CORRELATION_KEY_H__
#define __CORRELATION_KEY_H__

#include <time.h>

#include <ace/Time_Value.h>

#include "MDN.h"

class CorrelationKey
{
public:
	CorrelationKey();
	CorrelationKey(const MDN& mdn, const ACE_Time_Value& createTime);
	~CorrelationKey(void);

	/// Correlation key�� ���ڿ��� ��ȯ�Ѵ�.
	/**
	 * @return correlation key 
	 * @exception NONE
	 *
	 * @date 2007/03/23
	 * @author Sehoon Yang
	 *
	 * MDN, Create time, Host name �� �̿��ؼ� ������ correlation key �� ��ȯ�Ѵ�.
	 * �� �Լ��� ����ϱ� ������ �����ڳ� set �Լ��� ���� mdn�� createTime�� 
	 * �����Ǿ� �־�� �������� correlation key�� ���� �� �ִ�.
	 **/
	TinyString toString() const;

	void set(const MDN& mdn, const ACE_Time_Value& createTime);

private:
	/// Host name �� ���� pasgw �� ������ȣ�� �˾ƿ´�.
	/**
	 * @return PAS Gateway server number
	 * @exception NONE
	 *
	 * @date 2007/03/23
	 * @author Sehoon Yang
	 *
	 * PAS Gateway ��뼭���� hostname�� pasgw1, pasgw2, ... , pasgw6 ���� �����Ǿ� �ִ�.
	 * hostname�� ������ ���� int ������ ��ȯ�Ѵ�.
	 * ���� hostname�� ���� ��ȣ�� �� �� ���� ��� 0�� �����Ѵ�.
	 **/
	int getHostNo() const;

private:
	MDN _mdn;
	ACE_Time_Value _createTime;
};

#endif
