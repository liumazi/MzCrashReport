// test.cpp : 定义控制台应用程序的入口点。
//

#include <string>
#include <vector>
#include <assert.h>
#include <windows.h>
#include <exception>
#include <stdexcept>

#include "stdafx.h"
#include "CrashReport.h"

struct struct1
{
	int aa;
	int* p;
};

void fun0(struct1 s1[2], int ii[6], std::string s)
{
	s1[0].aa = *s1[0].p;
}

void fun1()
{
	std::vector<int> a;

	a.insert(a.begin(), 6);
	a.insert(a.begin(), 66);
	a.insert(a.begin(), 666);

	auto itr = a.begin();
	for (; itr != a.end(); itr++)
	{
		if (*itr == 6)
		{
			a.erase(itr);
			//break; vector iterator not incrementable
		}
	}

	if (itr == a.end()) // vector iterators incompatible
	{

	}
}

void fun2()
{
	assert(false);
}

class test3base
{
public:
	test3base() { call_fun3(); }
	void call_fun3() { this->fun3(); }
	virtual void fun3() = 0;
};

class test3Dervied : public test3base
{
	virtual void fun3() { };
};

void fun3()
{
	test3Dervied t3;
}

void fun4()
{
	new int[0x1fffffff];
}

void fun5()
{
	printf(nullptr);
}

void fun6()
{
	abort();
}

int fun7()
{
	int a = 1;
	int b = 0;
	return a / b;
}

int fun8()
{
	//terminate();
	throw("a");
}

int fun9(int b)
{
	__try
	{
		printf("fun9 begin \r\n");
		return 1 / b;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf("fun9 exception \r\n");
	}

	printf("fun9 end \r\n");

	return 66;
}

void fun10(int n)
{
	int b[1024*1024];
	int x = 0;
	for (int i = 0; i < 1024*1024; i++)
	{
		x += b[i];
	}

	if (n > 0)
	{
		fun10(n - 1);
	}	
}

void fun11()
{
	try
	{
		throw std::runtime_error("mz error");
	}
	catch (std::range_error)
	{
		printf("fun11 catch\r\n");
	}	
}

int _tmain(int argc, _TCHAR* argv[])
{
	InitCrashReport();

	int crashno = 10; // _tstoi(argv[1]);
	printf("crashno %d\r\n", crashno);

	switch (crashno)
	{
	case 0:
		struct1 test1[2];
		test1[0].p = nullptr;
		int ii[6];
		fun0(test1, ii, "mz");
		//*((int*)6) = 6;
		break;

	case 1:
		fun1();

	case 2:
		fun2();

	case 3:
		fun3();

	case 4:
		fun4();

	case 5:
		fun5();

	case 6:
		fun6();

	case 7:
		fun7();

	case 8:
		fun8();

	case 9:
		fun9(0);

	case 10:
		fun10(6);

	case 11:
		fun11();

	default:
		break;
	}

	return 0;
}