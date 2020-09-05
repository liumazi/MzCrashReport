// test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CrashReport.h"

struct struct1
{
	int aa;
	int* p;
};

void fun1(struct1 s1[2], int ii[6])
{
	s1[0].aa = *s1[0].p;
}

int _tmain(int argc, _TCHAR* argv[])
{
	InitCrashReport();

	if (argc == 2)
	{
		int crashno = _tstoi(argv[1]);
		printf("crashno %d\r\n", crashno);

		switch (crashno)
		{
		case 0:
			struct1 test1[2];
			test1[0].p = nullptr;
			int ii[6];
			fun1(test1, ii);
			//*((int*)6) = 6;
			break;

		default:
			break;
		}
	}

	return 0;
}

