#pragma once
#include <iostream>
#include <windows.h>

namespace ProfileConfig
{
    constexpr uint8_t PROFILE_SIZE = 50;
    constexpr uint8_t Min_BufferLen = 2;
    constexpr uint8_t Max_BufferLen = 2;

}

struct stProfileManager
{
    struct stProfileEntry
    {
        stProfileEntry()
            : _bUse(false), _tag(nullptr), _totalTime(0), _called(0), _aver(0.0f)
        {
        }
        bool _bUse;
        float _aver;

        const wchar_t *_tag;
        LARGE_INTEGER _startTime{};

        int64_t _totalTime;
        int64_t _min[ProfileConfig::Min_BufferLen]{};
        int64_t _max[ProfileConfig::Max_BufferLen]{};
        int64_t _called;

    };
    void FindEntry(const wchar_t *tag, stProfileEntry** out) 
    {
        stProfileEntry *empty = nullptr;
        for (int i = 0; i < ProfileConfig::PROFILE_SIZE; i++)
        {
            if (entrys[i]._tag == tag)
            {
                *out = &entrys[i];
                return;
            }
            if (empty == nullptr && entrys[i]._bUse == false)
            {
                empty = &entrys[i];
            }
        }
        if (empty == nullptr)
            return;
        *out = empty;
        empty->_bUse = true;
    }

  private:
    stProfileEntry entrys[ProfileConfig::PROFILE_SIZE];

};


#ifdef PROFILE
#define PRO_BEGIN(Tag) \
    do                 \
    {                  \
                       \
    } while (0)

#define PRO_END(Tag) \
    do               \
    {                \
                     \
    } while (0)
#else
#define PRO_BEGIN(Tag) \
    do                 \
    {                  \
                       \
    } while (0)

#define PRO_END(Tag) \
    do               \
    {                \
                     \
    } while (0)
#endif

struct stProfile
{
    stProfile(const wchar_t *tag)
        : entry(nullptr), _tag(tag)
    {
        PRO_BEGIN(_tag);
    }
    void Pro_begin()
    {
        intance.FindEntry(_tag, &entry);
        // °¡µæÂü
        if (entry == nullptr)
            __debugbreak();

    }
    void Pro_end()
    {
        intance.FindEntry(_tag, &entry);
    }
    ~stProfile()
    {
        PRO_END(_tag);
    }
    const wchar_t *_tag;
    stProfileManager::stProfileEntry *entry;
    inline static stProfileManager intance;
};
