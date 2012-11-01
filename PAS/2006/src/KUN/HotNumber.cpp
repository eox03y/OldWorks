/** @file  HotNumber.cpp
  * @author ����â

     @brief VoiceN, WINC, HotNumber, ���հ˻� ���� Ư�� URL ��ȯ.
     
      ��ȯ�ÿ� ������ ���� ������ ����.
      VoiceN, WINC, HotNumber ���� ���ڿ� ������ ���񽺿��� ���ڸ� �����Ͽ� ��ȯ�� URL�� ���Խ�Ų��.  ���� ���ڿ� �ںκ���'/'���� ���ڴ� �����Ͽ��� �Ѵ�.
     ���հ˻�(�ѱ�URL) ���񽺸� ���� URL�� ��ȯ�� ���� �� ����'/'���ڸ� �����Ͽ��� �Ѵ�.

*/

#include "HotNumber.h"
#include "Config.h"
#include "CGI.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
WINC, Hotnumber ���� ��ȣ���� ���Ͽ� dest �� �����Ѵ�.
*/
int	HotNumber::getHotDigits(const char *src, char *dest, int destMax)
{
	int	i=0;
	while(*src && i < destMax)
	{
		if (*src == '/') break;
		if (*src == 0x0B)
			*dest = '#';
		else
			*dest = *src;

		src++;
		dest++;
		i++;
	}
	
	*dest = 0;
	return i;
}

/**
@brief  **, ## ���� �� Ư�� URL�� �������� �޾�, �׿� ���õ�  magicn ����Ʈ�� �ּҷ� ��ȯ�Ѵ�.
@param url
@return 1:true(�ֳѹ�) 0:false(�Ϲ�url)

��ȯ�ÿ� ������ ���� ������ ����.
   VoiceN, WINC, HotNumber ���� ���ڿ� ������ ���񽺿��� ���ڸ� �����Ͽ� ��ȯ�� URL�� ���Խ�Ų��.  ���� ���ڿ� �ںκ���'/'���� ���ڴ� �����Ͽ��� �Ѵ�.
   ���հ˻�(�ѱ�URL) ���񽺸� ���� URL�� ��ȯ�� ���� �� ����'/'���ڸ� �����Ͽ��� �Ѵ�.
*/

int HotNumber::convert(const char *url, int urllength)
{
	const char *pPos = NULL;
	char oddsharp[3] = { '0x0b', '0x0b', 0 };
	int RetVal = 0;
	
	if( urllength == 0 )
		urllength = strlen(url);

	// �ּ����� �����͸� �����ϴ��� üũ
	if( urllength < 8 )
		return RetVal;

	// �����ؾ� �� ���ۺ��� �����Ͱ� ũ�� ����
	if( urllength > MAXLEN_MAGICN_URL )
		return RetVal;

	pPos = &url[7];

	if( pPos[0] == '*' )
	{
		pPos++;

		char	digits[32];

		getHotDigits(pPos, digits, sizeof(digits)-1);		

		// *** �� 3�� �ֳѹ�
		if( strncmp(digits, "**", 2) == 0 )
		{
			snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://hot.k-merce.com/nkey.asp?num=%s", digits );
		}

		// ** �� 2�� �ֳѹ�
		else
		if( strncmp(digits, "*", 1) == 0 )
		{
			snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://hot.k-merce.com/nkey.asp?num=%s", digits );
		}

		// *## �� �ΰ� �ֳѹ� Ȥ�� *[0x0b][0x0b] �ֳѹ�...�ܸ��� ������ ���� �ٸ��� �� �� �ִ�.
		else
		if( !strncmp(digits, "##", 2) || !strncmp(digits, oddsharp, 2) )
		{
			snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://hot.k-merce.com/nkey.asp?num=%s", digits );
		}

		// *# �� �Ѱ� �ֳѹ� Ȥ�� *[0x0b] �ֳѹ�...�ܸ��� ������ ���� �ٸ��� �� �� �ִ�.
		else
		if( digits[0] == '#')
		{
			snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://hot.k-merce.com/nkey.asp?num=%s", digits );
		}

		// * �� �Ѱ� �ֳѹ�
		else
		{
			// *0 �� ���� �ֳѹ� ó��
			if( digits[0] == '0' )
			{
				snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://nkey.magicn.com/nkeymain.asp?mobile=%s", digits );

			}
			// *1 ~ 9 �� ���� WINC �ֳѹ� ó��
			// @todo '#' ��ȣ�� %23 ���� ��ȯ.
			else if( digits[0] >= '1' && digits[0] <= '9' )
			{
				char escapedUrl[256];
				CGI::cgiUnescapeChars(escapedUrl, sizeof(escapedUrl)-1, digits, strlen(digits));
				
					snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://ktf.winc.or.kr/hot.asp?winc=%s", digits );
			}

			// �� ���� ���(��:*aa, *�� ��)���� �Ϲ� url �� ó��
			// �ܸ��⿡�� �ֳѹ��� �Է� �� �ٽ� ����� ������Ű�� ������ PAS�� http://* �� ���� ���´�.
			// �� �� ���� PAS ������ WINC URL �� �Ѱ��ش�.
			else
			{
				snprintf( magicnUrl, sizeof(magicnUrl)-1, "http://ktf.winc.or.kr" );
			}
		}

		RetVal = 1;
	}

	else	if( pPos[0] == '!' )
	{
		// KTF Search URL - OLD: "http://ktfsearch.magicn.com/search.asp?brtype=go&keyword=%s"
		// KTF Search URL - OLD: "http://ktfsearch.magicn.com/MagicN/KUN/TypeB/list_result_category.asp?search_category=%%C5%%EB%%C7%%D5&search_keyword=%s"

		// keyword �� )   http://!010%C0%CD/
		
		pPos++;

		char escapedUrl[256];
		//CGI::cgiUnescapeChars(escapedUrl, sizeof(escapedUrl)-1, pPos, strlen(pPos));
		STRNCPY(escapedUrl, pPos, sizeof(escapedUrl)-1);
		
		int urlleng =  strlen(escapedUrl);
		
		if (urlleng > 1 && escapedUrl[urlleng-1] != '/')
		{
			escapedUrl[urlleng] = '/';
			escapedUrl[urlleng+1] = '\0';
		}
		Config *conf = Config::instance();
	
		snprintf( magicnUrl, sizeof(magicnUrl)-1, "%s%s", conf->hotnumber.ktfSearchUrl.toStr(), escapedUrl );

		RetVal = 1;
	}

	else
		return RetVal;

	

	return RetVal;
}

int	HotNumber::test()
{
		HotNumber hotnum;
		typedef struct _hotnumTestCase {
			const char *orgUrl; /** ����(hotnumber) url */
			const char *magicnUrl; /** ��ȯ�� url. magicN �� url */
			const char *testCaseName; /* � �׽�Ʈ ���̽��ΰ�. �̸�.*/
		} hotnumTestCase;

		hotnumTestCase testcases[] = {
			{ "http://*01030101557", "http://nkey.magicn.com/nkeymain.asp?mobile=01030101557", "voiceN" },
			{ "http://*1004/", "http://ktf.winc.or.kr/hot.asp?winc=1004", "WINC" },
			{ "http://**1004/", "http://hot.k-merce.com/nkey.asp?num=*1004", "HotNumber *"},
			{ "http://***1004/", "http://hot.k-merce.com/nkey.asp?num=**1004", "HotNumber **"},
			{ "http://*#1004/", "http://hot.k-merce.com/nkey.asp?num=#1004", "HotNumber #"},
			{ "http://*##1004/", "http://hot.k-merce.com/nkey.asp?num=##1004", "HotNumber ##"},
			{ "http://!010%30%40%09/", "http://ktfsearch.magicn.com/search.asp?brtype=go&keyword=010%30%40%09", "Hangul 1"},
			// *[0x0b][0x0b] �� ���� �׽�Ʈ �ڵ� �߰�
			// *0 �� ���� �׽�Ʈ �ڵ� �߰�
			// *aa �� �Ϲ� url �� ���� �ڵ� �߰�
			// * Ȥ�� ! �� �ƴ� ���� �׽�Ʈ �ڵ� �߰�
			{ 0, 0, 0 }
		};

		/*
		hotnum.convert("http://*01030101557");
		if (strcmp(hotnum.getConverted(), "http://nkey.magicn.com/nkeymain.asp?mobile=01030101405") != 0)
		{
			printf("HotNumber::test - voiceN convert failed !\n");
		}
		*/

		for (int casenumber=0; casenumber < sizeof(testcases)/sizeof(hotnumTestCase); casenumber++)
		{
			if (testcases[casenumber].orgUrl == 0) break;

			hotnum.convert(testcases[casenumber].orgUrl);

			if (strcmp(hotnum.getConverted(), testcases[casenumber].magicnUrl) == 0)
			{
				printf("SUCC:");
			}
			else
			{
				printf("FAIL:");
			}
			printf(" - %s - %s --> %s\n", testcases[casenumber].testCaseName,
				testcases[casenumber].orgUrl, hotnum.getConverted());
		}

		return 0;
}

#ifdef TEST_MAIN
int main(int argc, char *argv[])
{
	HotNumber hn;
	hn.test();

	return 0;
}
#endif
