#pragma once
#define WIN32_LEAN_AND_MEAN

#include <cstdint>

#include <list>
#include <stack>
#include <string>

#include <Windows.h>


class CProfileRegistry final
{
  private:
    CProfileRegistry()
    {
        InitializeSRWLock(&_srwManager);
    }
    ~CProfileRegistry() {}
  public:
    static CProfileRegistry &GetInstance()
    {
        static CProfileRegistry instance;
        return instance;
    }
    void CreateProfile(const wchar_t *fileName);
    void ResetEntry();
    void RegistProfiler(class CProfileManager *manager) noexcept
    {
        _Acquires_exclusive_lock_(&_srwManager);
        _manager_list.push_back(manager);
        _Releases_exclusive_lock_(&_srwManager);
    }

  private:
    SRWLOCK _srwManager;
    std::list<class CProfileManager *> _manager_list;
};


class CProfileManager
{
  public:
    CProfileManager();

  public:
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
    void CreateProfile(const wchar_t *fileName);
    void ResetEntry();

  private:
    enum enConfig : int64_t
    {
        EnTryMaxSize = 30,
        AbnormalBufferSize = 3,

        MAX,
    };
    struct stProfileEntry
    {
        stProfileEntry()
            : _tag(nullptr), _totalTime(0), _totalCnt(0), _max{0}, _avg(0.0) 
        {
            for (int i = 0; i < AbnormalBufferSize; i++)
                _min[i] = INT_MAX;
        }

        const wchar_t *_tag;
        int64_t _totalTime;
        int64_t _totalCnt;
        int64_t _min[AbnormalBufferSize];
        int64_t _max[AbnormalBufferSize];

        double _avg;
        void InitEntry()
        {
            // tag는 유지
            _totalTime = 0;
            _totalCnt = 0;

            for (int i = 0; i < AbnormalBufferSize; i++)
            {
                _min[i] = INT_MAX;
                _max[i] = 0;
            }
        }
    };
    stProfileEntry _entrys[EnTryMaxSize];
    std::stack<int32_t> _freeIndex;

    LARGE_INTEGER _QPC_frequency;
    DWORD _ThreadID;
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
    void Profile_End(const wchar_t *tag, LARGE_INTEGER startTime);

    LARGE_INTEGER _startTime;
    const wchar_t *_tag;
};
