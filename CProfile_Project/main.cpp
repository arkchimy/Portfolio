// CProfile_Project.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "CProfile.h"
#include <Windows.h>

#include <conio.h>

int main()
{
    while (1)
    {
        {
            stProfile profile(L"test");
            for (int i = 0; i < 1000; i++)
            {
                i++;
            }
        }
        {
            stProfile profile(L"test2");
            for (int i = 0; i < 100000; i++)
            {
                i++;
            }
        }
        {
            stProfile profile(L"test3");
            for (int i = 0; i < 1000000; i++)
            {
                i++;
            }
        }
        Sleep(100);
        #ifdef PROFILE
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
            }
        }
        #endif   
    }
    
}