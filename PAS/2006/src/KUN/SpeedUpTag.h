#ifndef __SPEEDUPTAG_H__
#define __SPEEDUPTAG_H__

class SpeedUpTag
{
public:
	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------
	SpeedUpTag(void);
	~SpeedUpTag(void);

	static int insert(char *pszHTML, int iSize);
	static int remove(char *pszHTML, int iSize);

private:
	//-------------------------------------------------------
	// ��� �Լ�
	//-------------------------------------------------------
	static long isCompString2(const char *pSource, const char *pszComp, long *pPos, int icase);
};

#endif // __SPEEDUPTAG_H__
