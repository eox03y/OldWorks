/**
@file StrSplit.h

@brief String �и� ��ƿ��Ƽ

�־��� line (string) �� ���� �������� ������, string array �� �����Ѵ�.
string array�� �޸𸮴� �������� �Ҵ��Ѵ�.

MemSplit�� �ܼ�ȭ�ϰ�, ������ �����丵. handol@gmail.com 2006/08/02

MemSplit	line(12, 64);

while (fgets(buf, 1024, fp) != NULL) {
	line.split(buf);
	if (line.numFlds()==0) continue;

	somefunc( line.fldVal(0), line.fldLen(0) );  // ù��° �ʵ� ó��

	if (line.has("zeta"))
		printf("This line has zeta !");
}
*/

#ifndef STRSPLIT_H
#define STRSPLIT_H


#define	MAX_SPLIT_FLDS (32)  /**< �и��� �ʵ���� �ִ� ���� */

#ifndef ISSPACE
/* �ѱ� ó�� �ÿ� isspace () ����ϸ� ������ �Ǵ� ������ ����. */
#define	ISSPACE(X)	((X)==' ' || (X)=='\t' || (X)=='\n' || (X)=='\r')
#endif

class StrSplit
{
	public:
		StrSplit() { init(); }
		StrSplit(int numOfFlds, int lengOfFld) {
			init();
			alloc(numOfFlds, lengOfFld);
		}


		~StrSplit() {
			clear();
		}

		void    init() {
			maxNumFlds = maxLengFlds = numSplit = 0;
		}

		void alloc(int numOfFlds, int lengOfFld);
		void clear();

		int split(char *src); // �������� �и�.
		int split(char *src, char ch); // �־��� ���ڷ� �и�.
		int splitWords(char *src); // alphabet ���� ������ �ܾ ����.

		int has(char *str); // �и��� �ʵ�� �߿�  �־��� ���ڿ��� ������ ���� �ִ��� �˻�.


		char *fldVal(int index) { return fldVals[index]; } // �ʵ� ��. ���ڿ�.
		int fldLen(int index) { return fldLengs[index]; } // �ʵ� ����
		int numFlds() { return numSplit; } // �и���  �ʵ��� ����.

		char **argv() { return fldVals; } // �ʵ带 ���ڿ��� array ó�� ����ϰ��� �� ��
		int argc() { return numSplit; }

		void print(char *msg=NULL); // ��� �ʵ带 ���.

	private:
		int maxNumFlds;
		int maxLengFlds;
		int numSplit;							  /* split �� ��� string�� ���� */
		char *fldVals[MAX_SPLIT_FLDS];
		int fldLengs[MAX_SPLIT_FLDS];
		char    *memory;

};
#endif
