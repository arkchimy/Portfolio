#include "CProfile.h"
#include <strsafe.h>

    const wchar_t *captionFormat[2] =
    {
        L"+--------------+------------+------------------------+\n"
        L"| %-12ls | %10ls | %20ls   |\n"
        L"+--------------+------------+------------------------+\n",
        L"+--------------+------------+------------------------+\n"
    };
    const wchar_t * format[1] = {
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
        if (_entrys[i]._totalCnt >= AbnormalBufferSize * 2)
        {
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
