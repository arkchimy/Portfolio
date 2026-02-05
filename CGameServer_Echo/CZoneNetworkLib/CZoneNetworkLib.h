#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#include "CNetworkLib/CNetworkLib.h"

/*
* 
* 해당 구조의 핵심
*  MoveZone을 하여 Zone을 옮길때 Enter쪽에서는 SessionID밖에 알지못한다.
*  Player를 가져오는 방법.
* 
*  중복 로그인의 처리방안. 
*  Player의 반환시기
* 
* 
*/
//Zone을 구분할 유일한 Key값을 저장하는 자료형타입.
using ZoneKeyType = uint8_t;
enum enZoneMsgType : uint8_t
{
    EnterZone,
    LeaveZone,
};
class CZoneServer : public CLanServer
{

  protected:
    CZoneServer(int iEncoding)
        : CLanServer(iEncoding)
    {
    }

    virtual ~CZoneServer()
    {
        // TODO : ZoneSet가 들고있는 Zone에 대한 할당해제는 ZoneSet이 해줌
        //        
        std::lock_guard<SharedMutex> lock(_zoneMutex);
        for (std::pair<const ZoneKeyType, ZoneSet *> &element : _zoneMap)
        {
            delete element.second;
        }
        
    }
  private:
      //  사실  session을 직접받는것이 성능이 좋으나
      // networklib를 최소한으로 건드려서 그대로 복붙희망.
    virtual bool OnAccept(ull SessionID, SOCKADDR_IN &addr) final;
    virtual void OnRecv(ull SessionID, struct CMessage *msg) final;
    virtual void OnRelease(ull SessionID) final ;

  public:
    template <typename T>
    void RegisterLoginZone(const wchar_t *ThreadName, int deltaTime, ZoneKeyType key);

    template <typename T>
    void RegisterZone(const wchar_t *ThreadName, int deltaTime, ZoneKeyType key);

    void RequeseMoveZone(ull SessionID, ZoneKeyType targetZone,void* pPlayer);


    protected:
    ZoneSet *_LoginZone = nullptr;

    // OnRecv를 통해 해당 이벤트를 SetEvent 시켜야하므로 서버의OnRecv에서 접근할 수 있어야함.
    HANDLE _hLoginEvent = INVALID_HANDLE_VALUE;

    ull _bLoginZoneChk = false;
    ull _RecvTotalCnt = 0;

  public:

    //  동기화 객체의 필요성 : 여러쓰레드에서 Regist하면
    // _zoneMap의 iterator가 틀어질 수도있음.
    SharedMutex _zoneMutex;
    std::map<ZoneKeyType, ZoneSet *> _zoneMap;
    short _EchoThreadCnt = 0;
        // DB연동서버
    enum
    {
        IP_LEN = 16,
        DBName_LEN = 16,
        schema_LEN = 16,
        ID_LEN = 16,
        Password_LEN = 16,
    };
    char RedisIpAddress[IP_LEN]{
        0,
    }; // port 6379
};

template <typename T>
inline void CZoneServer::RegisterLoginZone(const wchar_t *ThreadName, int deltaTime, ZoneKeyType key)
{
    // 이벤트 방식과 Frame 방식 을 고려해야할듯.
    // ContentsLogic의 경우는 프레임이 존재하므로 겸사겸사 하트비트가능.
    // LoginLogic의 경우 프레임으로 돌 이유가 없음. 이벤트가 필요.

    if (InterlockedCompareExchange(&_bLoginZoneChk, 1, 0) != 0)
    {
        // LoginZone 2개 이상 등록한 경우
        __debugbreak();
    }
    static_assert(std::is_base_of_v<IZone, T>, " Regiset Login T is not from IZone ");
    static_assert(std::is_default_constructible_v<T>, " Regiset Login  T need default constructor ");

    T *LoginZone = new T();
    _hLoginEvent = CreateEvent(nullptr, 0, 0, nullptr);

    _LoginZone = new ZoneSet(LoginZone, ThreadName, deltaTime, this , _hLoginEvent);

    {
        std::lock_guard<SharedMutex> lock(_zoneMutex);
        if (_zoneMap.find(key) != _zoneMap.end())
        {
            // zone 의 중복 등록  무시할 수도있지만, 수정하도록 유도
            __debugbreak();
        }
        _zoneMap.insert({key, _LoginZone});
    }
}

template <typename T>
inline void CZoneServer::RegisterZone(const wchar_t *ThreadName, int deltaTime, ZoneKeyType key)
{
    // 단 T의 타입은 IZone의 자식 클래스여야함.
    static_assert(std::is_base_of_v<IZone, T>, " T is not from IZone ");
    static_assert(std::is_default_constructible_v<T>,
        " T need default constructor ");

    T *newZone = new T();
    ZoneSet *zoneSet = new ZoneSet(newZone, ThreadName,  deltaTime,this);
    // 해당 Server가 관리하는 zoneSet목록

    {
        std::lock_guard<SharedMutex> lock(_zoneMutex);
        if (_zoneMap.find(key) != _zoneMap.end())
        {
            // zone 의 중복 등록  무시할 수도있지만, 수정하도록 유도
            __debugbreak();
        }
        _zoneMap.insert({key, zoneSet});
    }

}
