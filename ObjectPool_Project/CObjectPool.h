#pragma once
#define WIN32_LEAN_MEAN
#include <Windows.h>

#include <list>

#include <cstddef> // offsetof
#include <cstdint> // uint32_t
#include <iostream>
#include <shared_mutex>

#pragma comment(lib, "winmm.lib")

/*
*	======================== Pool에서 주의할 점. ========================
*
        ■ 두 번이상 Release 되는 경우
        => 반환시에 _bActive를 확인하여 2번 반환되는 경우를 방지.

        ■ 반환된 Node가  나의 Pool에서 할당한 것이 아닌 경우.
        => OwnerPool의 Pointer를 들고 반환 시에 stNode에 저장된 OwnerPool과의 비교.

        ■ 반환되지 않은 Node가 존재하는 경우.
        => Pool의 삭제 시  "ActiveCnt == 0 " 체크.

        ■ 반환과 할당 과정에서 요소가 소실되는 경우.
        => Pool의 삭제 시 반환된 Node를 Delete 후 "AllocCnt == 0 " 체크.

        ■ 반환을 하지않는 요소에대한 추적.
        => 할당을 할때마다 Lock을 걸고 ActicveNodes List 를 작성.
                => Touch 함수를 만들고 해당 객체에 대해 접근할떄 호출. __Line__ 정보를 남기기
                        => cap을 걸어두고, 도달 시에 가장 오래된 순으로 정렬


*/
#ifdef _POOLTRACE
#define POOL_TOUCH(type, p) reinterpret_cast<decltype(type)::stNode *>(((char *)(p) - offsetof(decltype(type)::stNode, _data)))->Touch(__FILE__, __LINE__)
#else
#define POOL_TOUCH(type, p)
#endif
template <typename T>
class CObjectPool final
{
  private:
    enum : uint64_t
    {
        GuardValue = 0xfdfdfdfdfdfdfdfd,
    };

  public:
    struct stNode final
    {
        explicit stNode(CObjectPool *ownerPool)
            :
#ifdef _POOLTRACE
              _bActive(false), _frontGuard(GuardValue), _backGuard(GuardValue), _file(nullptr), _line(0), _lastTime(0),
#endif
              _next(nullptr), _ownerPool(ownerPool)
        {
        }
        stNode(const stNode &other) = delete;
        stNode(stNode &&other) = delete;

        stNode &operator=(const stNode &other) = delete;
        stNode &operator=(stNode &&other) = delete;

        bool operator<(const stNode &other)
        {
            return this->_lastTime < other._lastTime;
        }

        void Touch(const char *file, int line)
        {
#ifdef _POOLTRACE
            _file = file;
            _line = line;
            _lastTime = timeGetTime();
#endif
        }

#ifdef _POOLTRACE
        uint64_t _frontGuard;
        T _data{};
        uint64_t _backGuard;
        stNode *_next;
        const char *_file;
        int _line;
        int32_t _lastTime;
        bool _bActive;
#else
        T _data{};
        stNode *_next;
#endif // _POOLTRACE
        CObjectPool *_ownerPool;
    };

  public:
    CObjectPool()
        : _AllocNodeCnt(0), _ActiveNodeCnt(0), _capacity(INT_MAX)
    {
#ifdef _POOLTRACE
        InitializeSRWLock(&_srw_lock);
#endif
    }
    ~CObjectPool()
    {
        stNode *oldTop;
        if (_ActiveNodeCnt != 0)
            __debugbreak();

        while (_top != &_dummy)
        {
            oldTop = _top;
            _top = oldTop->_next;
            delete oldTop;
            _AllocNodeCnt--;
        }

        if (_AllocNodeCnt != 0)
            __debugbreak();
    }

  public:
    void *Alloc();
    void Release(void *ptr);
    void SetCapacity(uint32_t capacity) { _capacity = capacity; }
#ifdef _POOLTRACE
    void CatchLeak();
#endif // DEBUG

    uint64_t GetActiveNodeCnt() const
    {
        return _ActiveNodeCnt;
    }

    __int64 GetAllocNodeCnt() const
    {
        return _AllocNodeCnt;
    }

  private:
    // 해당 ObjectPool 에서 할당한 Node수
    uint64_t _AllocNodeCnt;
    // 아직 반환되지않은 Node의 Cnt
    uint64_t _ActiveNodeCnt;

    stNode _dummy{this};
    stNode *_top = &_dummy;
    uint32_t _capacity;

#ifdef _POOLTRACE
    std::list<stNode *> _ActiveNodes;
    SRWLOCK _srw_lock;
#endif
};

template <typename T>
inline void *CObjectPool<T>::Alloc()
{
    stNode *retNode;

    do
    {

        if (_top == &_dummy)
        {
            uint64_t local_AllocNodeCnt;
            retNode = new stNode(this);
            local_AllocNodeCnt = _interlockedincrement64((long long *)&_AllocNodeCnt);
#ifdef _POOLTRACE
            if (_capacity == _AllocNodeCnt)
            {
                CatchLeak();
            }
#endif
            break;

        }
        else
        {
            retNode = _top;
        }
    } while (InterlockedCompareExchange((unsigned long long *)&_top, (unsigned long long )_top->_next,
                                        (unsigned long long)retNode) != (unsigned long long )retNode);
    _interlockedincrement64((long long*)&_ActiveNodeCnt);
#ifdef _POOLTRACE
    retNode->_bActive = true;
    AcquireSRWLockExclusive(&_srw_lock);
    _ActiveNodes.push_back(retNode);
    ReleaseSRWLockExclusive(&_srw_lock);
    retNode->_lastTime = INT_MAX;

#endif
    return (char *)retNode + offsetof(stNode, _data);
}

template <typename T>
inline void CObjectPool<T>::Release(void *ptr)
{
    void *Ptr = (char *)ptr - offsetof(stNode, _data);
    stNode *retNode = static_cast<stNode *>(Ptr);
    stNode *oldTop;

#ifdef _POOLTRACE
    // 할당한 Pool이 아닐 경우
    if (retNode->_ownerPool != this)
        __debugbreak();
    // 2번 Release
    if (retNode->_bActive == false)
        __debugbreak();
    // 버퍼 오버런
    if (retNode->_frontGuard != GuardValue)
        __debugbreak();
    if (retNode->_backGuard != GuardValue)
        __debugbreak();
#endif

#ifdef _POOLTRACE
    retNode->_bActive = false;
    AcquireSRWLockExclusive(&_srw_lock);
    for (auto iter = _ActiveNodes.begin(); iter != _ActiveNodes.end();)
    {
        if (*iter == retNode)
        {
            _ActiveNodes.erase(iter);
            break;
        }
        iter++;
    }
    ReleaseSRWLockExclusive(&_srw_lock);
#endif
    do
    {
        oldTop = _top;
        retNode->_next = oldTop;

    } while (InterlockedCompareExchange((unsigned long long *)&_top, (unsigned long long )retNode,
                                        (unsigned long long)oldTop) != (unsigned long long)oldTop);
    _InterlockedDecrement64((long long*)&_ActiveNodeCnt);
}
#ifdef _POOLTRACE
template <typename T>
inline void CObjectPool<T>::CatchLeak()
{
    AcquireSRWLockExclusive(&_srw_lock);
    _ActiveNodes.sort([](const CObjectPool<T>::stNode *a, const CObjectPool<T>::stNode *b)
                      { return a->_lastTime < b->_lastTime; });
    __debugbreak();
    ReleaseSRWLockExclusive(&_srw_lock);
}
#endif // DEBUG
