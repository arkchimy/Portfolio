// ObjectPool_Project.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "CObjectPool.h"

class A
{
public:


    void Foo()
    {
        std::cout << "Foo of Class \'A\' \n";
    }
    int arr[100]{0,};

};

#include <vector>

int main()
{

    srand(3);
    CObjectPool<A> objPool;
    while (1)
    {

        std::vector<A*> vec;

        int randNum = rand()% 1000;
        vec.reserve(randNum);

        A* a;
        for (int i = 0; i < randNum; i++)
        {
            a = static_cast<A*>(objPool.Alloc());
            vec.push_back(a);
            POOL_TOUCH(objPool, a);
        }
        
        for (int i = 0; i < randNum; i++)
        {
            objPool.Release(vec[i]);
        }


        printf("GetAllocNodeCnt : %lld , GetActiveNodeCnt : %lld \n", objPool.GetAllocNodeCnt(), objPool.GetActiveNodeCnt());
  
    }
}

