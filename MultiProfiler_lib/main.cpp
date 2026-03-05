// MultiProfiler_lib.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "CMultiProfiler.h"
#include <thread>
#include <vector>
#include <conio.h>

#include "../_Lib/CObjectPoolLib/CObjectPoolLib.h"

class A
{
  public:
    int a;
};
CObjectPool<A> pool;

unsigned int Foo(void *arg) 
{

    while (1)
    {
        std::vector<A*> vec;
        vec.reserve(100);
        {
            stProfile profile(L"newAllocTime");
            for (int i = 0; i < 100; i++)
            {
                A *a = new A();
                vec.emplace_back(a);
   
            }
            for (int i = 0; i < 100; i++)
            {
                delete vec[i];
            }
        }
        vec.clear();
        vec.reserve(100);

        {
            stProfile profile(L"PoolAlloc");
            for (int i = 0; i < 100; i++)
            {
                A *a = static_cast<A *> (pool.Alloc());
                vec.emplace_back(a);
   
            }
            for (int i = 0; i < 100; i++)
            {
      
                pool.Release(vec[i]);
            }
        }
    }
}

int main()
{
    for (int i =0; i < 1; i++)
        _beginthreadex(nullptr, 0, Foo, nullptr, 0, nullptr);
    pool.SetCapacity(5000);
    while (1)
    {
#ifdef PROFILE
        if (_kbhit())
        {
            char ch = _getch();
            if (ch == 'A' || ch == 'a')
            {
                CProfileRegistry::GetInstance().CreateProfile(L"Profile");
            }
            else if (ch == 'D' || ch == 'd')
            {
                CProfileRegistry::GetInstance().ResetEntry();
            }
        }
#endif
    }
}

