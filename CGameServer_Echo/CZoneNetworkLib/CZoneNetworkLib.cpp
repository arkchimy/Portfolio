// CZoneNetworkLib.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//
#include "CZoneNetworkLib.h"
#include "../enZoneType.h"

// TODO: 라이브러리 함수의 예제입니다.
void fnCZoneNetworkLib()
{
    
    enum class enZoneType : ZoneKeyType
    {
        LoginZone = 0,
        EchoZone
    };
        // IZone을 상속받아서 기능을 구현해둔다.
        class clsLoginZone : public IZone
        {
          public:
            // 이 함수를 컨텐츠 개발자가 구현해야함.
            virtual void OnEnterWorld(ull SessionID, SOCKADDR_IN &addr, void *pPlayer = nullptr){};
            virtual void OnRecv(ull SessionId, struct CMessage *msg) {};
            virtual void OnUpdate() {};
            virtual void OnLeaveWorld(ull SessiondId){};
            virtual void OnDisConnect(ull SessionID){};
        };

        class clsContentsZone : public IZone
        {
          public:
            // 이 함수를 컨텐츠 개발자가 구현해야함.
            virtual void OnEnterWorld(ull SessionID, SOCKADDR_IN &addr, void *pPlayer = nullptr) {};
            virtual void OnRecv(ull SessionId, struct CMessage *msg) {};
            virtual void OnUpdate() {};
            virtual void OnLeaveWorld(ull SessiondId){};
            virtual void OnDisConnect(ull SessionID){};
        };
    

    // CZoneServer을 상속받는 Server 구현.
    class CTestServer : public CZoneServer
    {
      public:
        CTestServer(int iEncording )
            : CZoneServer(iEncording)
        {
            //LoginZone은 하나만 등록해야하고.
            RegisterLoginZone<clsLoginZone>(L"LoginServer", 4000, (ZoneKeyType)enZoneType::LoginZone);

            // 동일한 방식으로 상속받아 구현한 class를 여기에 등록한다.
            RegisterZone<clsContentsZone>(L"Contents이름", 20, (ZoneKeyType)enZoneType::EchoZone);
        }
    };
    //  TestServer생성시에 LoginZone을 넘겨주게 설계.
    CTestServer *server = new CTestServer(true);


}

//  IOCP 에서 알려주는 용도
bool CZoneServer::OnAccept(ull SessionID, SOCKADDR_IN &addr)
{

    clsSession& session = sessions_vec[SessionID >> 47];

    session._addr = addr;
    session.m_zoneSet = _LoginZone;
    _LoginZone->Push(SessionID);

    SetEvent(_LoginZone->_hEvent);

    CSystemLog::GetInstance()->Log(L"Session_Log", en_LOG_LEVEL::DEBUG_Mode, L"OnAccept %20s  : %lld ",
                                   L" SessionID ", SessionID);
    return true;
}

//  IOCP 에서 알려주는 용도
void CZoneServer::OnRecv(ull SessionID, CMessage *msg)
{
    clsSession &session = sessions_vec[SessionID >> 47];
    session.m_ZoneBuffer.Push(msg);
    InterlockedIncrement(&_RecvTotalCnt);
}

//  IOCP 에서 알려주는 용도
void CZoneServer::OnRelease(ull SessionID)
{
    clsSession &session = sessions_vec[SessionID >> 47];

    _interlockedbittestandreset64((LONG64*)&session.m_ReleaseAndDBReQuest, 63);

}

void CZoneServer::RequeseMoveZone(ull SessionID, ZoneKeyType targetZone, ZoneKeyType lastZone, void *pPlayer)
{
    //  이 단계에서 session이 Release될수 있을까?
    // RequeseMoveZone 의 호출은 속해있는 Zone 내부에서 호출한 것.
    // 동기적으로 이루어지므로 해제될 가능성 Zero

    clsSession &session = sessions_vec[SessionID >> 47];
    int targetIdx;
    short userMin;

    // 매우 치명적인 상황.
    if (SessionID != session.m_SeqID)
        __debugbreak();

    session.pPlayer = pPlayer;
    if (targetZone == 0)
    {
        session.m_zoneSet = _LoginZone;
        return;
    }
    //Echo 라면
    else if (targetZone == 1)
    {
        userMin = _EchoMaxUser;
    }

    {
        auto iter = _zoneKeyMap.find(targetZone);
        if (iter == _zoneKeyMap.end())
        {
            //_zoneMap 에 등록이 되지않은 상황. 말이 안 됨.
            __debugbreak();
        }
        // 여기부터는 LoginZone만 접근. Lock없어도 됨.
        std::vector<ZoneSet *>& vec = iter->second;
        targetIdx = 0;

        for (int i = 0; i < vec.size(); i++)
        {
            short currentCount = vec[i]->GetCurrentSessionCount();
            if (userMin > currentCount)
            {
                userMin = currentCount;
                targetIdx = i;
            }
        }
        if (userMin == _EchoMaxUser)
        {
            RegisterZone<class clsEchoZone>(L"EchoThread", 20, (ZoneKeyType)enZoneType::EchoZone);//희망
        }

    }
}
