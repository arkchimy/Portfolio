#pragma once
#define WIN32_LEAN_AND_MEAN
#define PROFILE

#include <Windows.h>
#include <stdint.h>
#include <timeapi.h>

#include <stack>
#include <string>

#pragma comment(lib, "winmm.lib")

struct stProfileManager
{
  private:
    stProfileManager()
    {
        QueryPerformanceFrequency(&_QPC_frequency);

        for (int32_t i = EnTryMaxSize - 1; i >= 0; i--)
        {
            _freeIndex.push(i);
        }
        _fileName += L"Profile_"; 
        _fileName += TEXT(__DATE__); 
        _fileName += L".txt"; 
    }

  public:
    static stProfileManager *GetInstance()
    {
        static stProfileManager *instance = nullptr;
        if (instance == nullptr)
            instance = new stProfileManager();
        return instance;
    }
    void UpdateEntry(const wchar_t *tag, int64_t distanceTime)
    {
        stProfileEntry *entry = nullptr;
        int32_t idx = 0;

        for (int32_t i = 0; i < EnTryMaxSize; i++)
        {
            if (_entrys[i]._tag == tag)
            {
                entry = &_entrys[i];
                break;
            }
        }
        if (entry == nullptr)
        {
            if (_freeIndex.empty())
            {
                // entry가 가득 참.
                __debugbreak();
            }
            idx = _freeIndex.top();
            _freeIndex.pop();
            _entrys[idx]._tag = tag;

            entry = &_entrys[idx];
        }
        entry->_totalCnt++;
        entry->_totalTime += distanceTime;

        // _min중에  가장 큰 값보다 작다면, 적절한 위치 탐색.
        if (distanceTime < entry->_min[AbnormalBufferSize - 1])
        {
            for (int i = 0; i < AbnormalBufferSize; i++)
            {
                if (entry->_min[i] > distanceTime)
                {
                    for (int j = AbnormalBufferSize - 1; j != i; j--)
                    {
                        entry->_min[j] = entry->_min[j - 1];
                    }
                    entry->_min[i] = distanceTime;
                    break;
                }
            }
        }

        // _max[] 내림 차순으로
        // _max중 가장 작은 것보다 크다면, 적절한 위치 찾기.
        if (entry->_max[AbnormalBufferSize - 1] < distanceTime)
        {
            for (int i = 0; i < AbnormalBufferSize; i++)
            {
                if (entry->_max[i] < distanceTime)
                {
                    for (int j = AbnormalBufferSize - 1; j != i; j--)
                    {
                        entry->_max[j] = entry->_max[j - 1];
                    }
                    entry->_max[i] = distanceTime;
                    break;
                }
            }
        }
    }
    void CreateProfile();
    void EntryReset() {};
  private:
    enum enConfig
    {
        EnTryMaxSize = 30,
        AbnormalBufferSize = 3,

        MAX,
    };
    struct stProfileEntry
    {
        stProfileEntry()
            : _tag(nullptr), _totalTime(0), _totalCnt(0), _min{INT_MAX}, _max{0}, _avg(0.0f) {}

        const wchar_t *_tag;
        int64_t _totalTime;
        int64_t _totalCnt;
        int64_t _min[AbnormalBufferSize];
        int64_t _max[AbnormalBufferSize];

        float _avg;
    };
    stProfileEntry _entrys[EnTryMaxSize];
    std::stack<int32_t> _freeIndex;

    LARGE_INTEGER _QPC_frequency;
    
    std::wstring _fileName;
    FILE *hFile = nullptr;
};


struct stProfile
{
    stProfile(const wchar_t *tag)
        : _tag(tag), _startTime{0}
    {
#ifdef PROFILE
        Profile_Start(&_startTime);
#endif
    }
    ~stProfile()
    {
#ifdef PROFILE
        Profile_End(_tag, _startTime);
#endif
    }
    void Profile_Start(LARGE_INTEGER *out)
    {
        QueryPerformanceCounter(out);
    }
    void Profile_End(const wchar_t *tag, LARGE_INTEGER startTime)
    {
        LARGE_INTEGER currentTime;
        int64_t distance;

        // 미리 시간 측정
        QueryPerformanceCounter(&currentTime);
        distance = currentTime.QuadPart - startTime.QuadPart;

        stProfileManager::GetInstance()->UpdateEntry(tag, distance);
    }
    LARGE_INTEGER _startTime;
    const wchar_t *_tag;
};
