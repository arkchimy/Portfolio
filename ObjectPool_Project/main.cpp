// ObjectPool_Project.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "../_Lib/Profiler_lib/Profiler_lib.h"
#include "../_Lib/Single_Profiler_lib/Single_Profiler_lib.h"
#include "CObjectPool.h"
#include <conio.h>
#include <iostream>
#include <thread>


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

void UnReleaseFunc(A *obj)
{
    POOL_TOUCH(objPool, obj);
}
void ReleaseFunc(A *obj)
{
    POOL_TOUCH(objPool, obj);
    objPool.Release(obj);
}

unsigned int ThreadFunc(void *arg) 
{
    while (1)
    {
        std::vector<A *> vec;
        int randNum = rand() % 1000;
        vec.reserve(randNum);
        A *a;
        for (int i = 0; i < randNum; i++)
        {
            {
                stProfile profile2(L"ObjectPool");
                a = static_cast<A *>(objPool.Alloc());
                vec.push_back(a);
            }
        }

        for (int i = 0; i < randNum; i++)
        {
            ReleaseFunc(vec[i]);
        }
    }
}
int main()
{

    srand(3);

    objPool.SetCapacity(20000);
    for (int i = 0; i < 10; i++)
    {
        _beginthreadex(nullptr, 0, ThreadFunc, nullptr, 0, nullptr);
    }

    
    while (1)
    {
        printf("objPool %lld : \n", objPool.GetAllocNodeCnt());
        if (_kbhit())
        {
            char ch = _getch();
            if (ch == 'A')
            {
                PROFILE_Manager::Instance.createProfile();
                stProfileManager::GetInstance()->CreateProfile();
            }
            else if (ch == 'D')
            {
                PROFILE_Manager::Instance.resetInfo();
                stProfileManager::GetInstance()->ResetEntry();
            }
        }
    
    }
}
