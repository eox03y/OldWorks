#ifndef __ACTIVE_OBJECT_CHECKER_H__
#define __ACTIVE_OBJECT_CHECKER_H__

class ActiveObjectChecker
{
public:
	ActiveObjectChecker()
	{
		_activeObject = true;
	}

	virtual ~ActiveObjectChecker()
	{
		_activeObject = false;
	}

	bool valid()
	{
		return _activeObject;
	}

private:
	bool _activeObject; // ���� object�� ���������� �޸𸮿� �Ҵ�� object ������ ����ϴ� flag
};

#endif
