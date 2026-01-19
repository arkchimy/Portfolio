#pragma once
#include "CZoneNetworkLib/CZoneNetworkLib.h"
#include <unordered_map>
#include "enZoneType.h"

/*
* 
*     // 해당 Hash 는 다른 Zone에서도 접근함.
    std::unordered_map<ull, stPlayer *> SessionID_hash;
    // 중복 로그인 주의

    std::unordered_map<INT64, stPlayer *> Account_hash;

    이 부분을 Server 함수를 통해 전달하여 Lock을 걸고 접근하는 방식을 제거.
*/
class clsLoginZone : public IZone
{
    enum enMsgType : std::uint8_t
    {
        LoginPacket,
        HeartBeatPacket,
        Max,
    };

  public:
    // 이 함수를 컨텐츠 개발자가 구현해야함.
    virtual void OnEnterWorld(ull SessionID, SOCKADDR_IN &addr,void* pPlayer = nullptr);
    virtual void OnRecv(ull SessionID, struct CMessage *msg);
    virtual void OnUpdate();

    // 연결 끊김이 아닌. Zone을 옮길때 호출하는 함수. 
    virtual void OnLeaveWorld(ull SessionID);
    // 연결 끊김을 의미하는 함수.
    virtual void OnDisConnect(ull SessionID);


    bool PacketProc(ull SessionID, CMessage *msg);

    void REQ_LOGIN(ull SessionID, CMessage *msg, INT64 AccountNo, WCHAR *SessionKey, WORD wType = en_PACKET_CS_GAME_REQ_LOGIN, BYTE bBroadCast = false, std::vector<ull> *pIDVector = nullptr, size_t wVectorLen = 0);

    // TODO : private로 바꾸기
    // 인증된 Session 
    std::unordered_map<ull, stPlayer *> SessionID_hash;
    std::unordered_map<INT64, stPlayer *> Account_hash;

    // LoginZone에 연결된 미 인증 Session  인증이 될 경우 해당 해시에서 제거된다.
    std::unordered_map<ull, stPlayer *> prePlayer_hash;

    // 해당 Player 포인터를 다른존에 어떻게 넘겨줄 것인가.
    CObjectPool<stPlayer> player_pool;

    private:
    DWORD _sessionTimeoutMs = 10000;   // 10초
    
  public:
    ull _msgTypeCntArr[Max]{0,};
    ull totalPacketCnt = 0;


    // 모니터링 정보

    ull Auth_SessionCnt = 0;
    ull User_SessionCnt = 0;
    
    // Frame 시 ++하기 1초의 구별은 모니터링 쓰레드에서 구현
    ull LoginThreadFPS = 0;

    ull AcceptTps = 0;
    ull _UpdateFrame = 0;
};