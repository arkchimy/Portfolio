// Single_Profiler_lib.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//
#include "Single_Profiler_lib.h"

// TODO: 라이브러리 함수의 예제입니다.
void fnSingleProfilerlib()
{
#define PROFILE // 전처리기
    stProfile profile(L"TagName");
    //Logic
    // 소멸자에서 측정.


    //main문에서 
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
#include <strsafe.h>

const wchar_t *captionFormat[2] =
    {
        L"+--------------+------------+------------------------+\n"
        L"| %-12ls | %10ls | %20ls   |\n"
        L"+--------------+------------+------------------------+\n",
        L"+--------------+------------+------------------------+\n"};
const wchar_t *format[1] = {
    L"| %-12ls | %10d | %20.5f us|\n",
};
void stProfileManager::CreateProfile()
{
    wchar_t buffer[1000];
    _wfopen_s(&hFile, _fileName.c_str(), L"w+ ,ccs=UTF-16LE");
    if (hFile == nullptr)
    {
        __debugbreak();
        return;
    }
    StringCchPrintf(buffer, sizeof(buffer) / sizeof(wchar_t), captionFormat[0], L"Tag", L"Called", L"Avg");
    fwrite(buffer, sizeof(wchar_t), wcslen(buffer), hFile);

    for (int i = 0; i < EnTryMaxSize; i++)
    {
        ZeroMemory(buffer, sizeof(buffer));
        if (_entrys[i]._totalCnt > AbnormalBufferSize * 2)
        {
            double totalTIme = static_cast<double>(_entrys[i]._totalTime);
            int64_t abnormalSum = 0;
            for (int i = 0; i < AbnormalBufferSize; i++)
            {
                abnormalSum += _entrys[i]._min[i];
                abnormalSum += _entrys[i]._max[i];
            }
            totalTIme -= abnormalSum;
       
            _entrys[i]._avg = (totalTIme / (_entrys[i]._totalCnt - AbnormalBufferSize * 2)) * 1e6 / _QPC_frequency.QuadPart;

            StringCchPrintf(buffer, sizeof(buffer) / sizeof(wchar_t), format[0], _entrys[i]._tag, _entrys[i]._totalCnt, _entrys[i]._avg);
            fwrite(buffer, sizeof(wchar_t), wcslen(buffer), hFile);
        }
    }

    fwrite(captionFormat[1], sizeof(wchar_t), wcslen(captionFormat[1]), hFile);
    fclose(hFile);
}

void stProfileManager::ResetEntry()
{
    for (int i = 0; i < EnTryMaxSize; i++)
    {
        ZeroMemory(&_entrys[i], sizeof(_entrys[i]));
    }
    _wfopen_s(&hFile, _fileName.c_str(), L"w+ ,ccs=UTF-16LE");
    if (hFile != nullptr)
        fclose(hFile);
}
