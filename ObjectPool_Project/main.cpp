// ObjectPool_Project.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "../_Lib/Single_Profiler_lib/Single_Profiler_lib.h"
#include "../_Lib/CObjectPoolLib/CObjectPoolLib.h"

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
bool bOn = true;
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
    while (bOn)
    {
        int randNum = rand() % 1000;
        A *a;
        for (int i = 0; i < randNum; i++)
        {
            {
                // stProfile profile2(L"ObjectPool");
                a = static_cast<A *>(objPool.Alloc());
                ReleaseFunc(a);
            }
        }
    }
    return 0;
}
unsigned int ThreadFunc2(void *arg)
{


    while (bOn)
    {
        std::vector<A *> vec;
        vec.reserve(1000);

        int randNum = rand() % 1000;
        A *a;
        for (int i = 0; i < randNum; i++)
        {
            a = static_cast<A *>(objPool.Alloc());
            vec.push_back(a);
        }
        for (int i = 0; i < randNum; i++)
        {
            objPool.Release(vec[i]);
        }
    }
    return 0;
}

int main()
{
    HANDLE hThread[10];
    srand(3);

    objPool.SetCapacity(20000);
    for (int i = 0; i < 10; i++)
    {
        if (i % 2== 0)
            hThread[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadFunc, nullptr, 0, nullptr);
        else
            hThread[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadFunc2, nullptr, 0, nullptr);
    }

    
    while (1)
    {
        printf("objPool : %lld  \n", objPool.GetAllocNodeCnt());
        if (_kbhit())
        {
            char ch = _getch();
            if (ch == 'A' || ch == 'a')
            {
                stProfileManager::GetInstance()->CreateProfile();
            }
            else if (ch == 'D' || ch == 'd')
            {
                stProfileManager::GetInstance()->ResetEntry();
                bOn = false;
                break;
            }
        }
    
    }
    WaitForMultipleObjects(10, hThread, true, INFINITE);
}
