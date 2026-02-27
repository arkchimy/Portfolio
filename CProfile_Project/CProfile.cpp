#include "CProfile.h"


void stProfileManager::CreateProfile()
{
    _wfopen_s(&hFile, _fileName.c_str(), L"w+ ,ccs=UTF-16LE");
    if (hFile == nullptr)
    {
        __debugbreak();
        return;
    }

}
