// ObjectPool_Project.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include <iostream>
#include "CObjectPool.h"
#include "../../_Portfolio/_Lib/Profiler_lib/Profiler_lib.h"
#include <conio.h>


class A
{
public:


	void Foo()
	{
		std::cout << "Foo of Class \'A\' \n";
	}
	int arr[700];

};

#include <vector>
CObjectPool<A> objPool;

void UnReleaseFunc(A* obj)
{
	POOL_TOUCH(objPool, obj);

}
void ReleaseFunc(A* obj)
{
	POOL_TOUCH(objPool, obj);
	objPool.Release(obj);
}

int main()
{

	srand(3);

	objPool.SetCapacity(5000);
	while (1)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'A')
			{
				PROFILE_Manager::Instance.createProfile();
			}
			else if (ch == 'D')
			{
				PROFILE_Manager::Instance.resetInfo();
			}
		}
		std::vector<A*> vec;
		std::vector<A*> vec2;
		A* a;

		int randNum = rand() % 1000;
		vec.reserve(randNum);
		vec2.reserve(randNum);


		for (int i = 0; i < randNum; i++)
		{
			{
				Profile profile(L"ObjectPool");
				a = static_cast<A*>(objPool.Alloc());
			}
			vec.push_back(a);
			{
				Profile profile(L"newAlloc");
				a = new A();
			}
			vec2.push_back(a);

		}

		for (int i = 0; i < randNum; i++)
		{
			ReleaseFunc(vec[i]);
			delete vec2[i];
		}
	}
}

