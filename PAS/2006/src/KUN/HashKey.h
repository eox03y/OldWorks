#ifndef HASHKEY_H
#define HASHKEY_H

#include "Common.h"
#include "MyLog.h"

class HashKey
{
public:
	
/**
@brief Hash Key ����

 * @param pDest key�� ����� ����, ��������� 17����Ʈ �̻�(HashKey 16bytes + null 1byte)
 * @param pSrcString HashKey�� ������ �� ������ ���ڿ� (null terminated string, ���� ���� ����)
 * @return ���� 0, ���� -1
*/
	static	void	prepare();
	static	void	writeLog(char *mdn, char *phoneIp, const char *hashkey, const char*url, int reqsize, int respsize);
	static	int	getKtfHashKey(char *pDest, const char *pSrcString);
	static	MyLog* hashkeylog;
};

#endif
