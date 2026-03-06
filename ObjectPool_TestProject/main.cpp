// ObjectPool_TestProject.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "../_Lib/CObjectPoolLib/CObjectPoolLib.h"
#include <iostream>
#include <thread>

#include <conio.h>

HANDLE hStartEvent;
int cnt;
bool bOn;

class A
{
  public:
    int a;
};
CObjectPool<A> rpool;

unsigned int Foo(void *arg)
{
    WaitForSingleObject(hStartEvent, INFINITE);

    CObjectPool<A> *pool = static_cast<CObjectPool<A> *>(arg);

    std::vector<A *> vec;
    vec.reserve(cnt);
    while (bOn)
    {
        for (int i = 0; i < cnt; i++)
        {
            
            A *a = static_cast<A *>(pool->Alloc());
            vec.push_back(a);
            POOL_TOUCH(rpool, a);
        }
        for (int i = 0; i < cnt; i++)
        {
            if(rand()% 1000 != 0)
                pool->Release(vec[i]);
        }
        vec.clear();
    }
    
    return 0;
}

#define THREAD_CNT 10
#define RT_ASSERT(x)        \
    do                      \
    {                       \
        if (!(x))           \
        {                   \
            __debugbreak(); \
        }                   \
    } while (0)
int main()
{
    hStartEvent = CreateEvent(nullptr, 1, false, nullptr);

    srand(3);

    while (1)
    {
        CObjectPool<A> pool;
        HANDLE hThreads[THREAD_CNT];

        cnt = rand() % 100 + 1;
  
        printf(" LoopCnt : %5d ,", cnt);

        for (int i = 0; i < THREAD_CNT; i++)
            hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, Foo, &pool, 0, nullptr);
        bOn = true;

        RT_ASSERT(hStartEvent != 0);
        SetEvent(hStartEvent);

        //Sleep(5000);
        bOn = false;
        WaitForMultipleObjects(THREAD_CNT, hThreads, true, INFINITE);
        for (int i = 0; i < THREAD_CNT; i++)
            CloseHandle(hThreads[i]);

        ResetEvent(hStartEvent);

        printf("   Pool AllocCnt : %5lld  Pool ActiveCnt : %5lld \n", pool.GetAllocNodeCnt(), pool.GetActiveNodeCnt());
        //Sleep(1000);
    }
}
