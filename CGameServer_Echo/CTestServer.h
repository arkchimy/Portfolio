#pragma once
#include "./CZoneNetworkLib/CZoneNetworkLib.h"

#include "clsLoginZone.h"
#include "clsEchoZone.h"

class CTestServer : public CZoneServer
{
    friend class Stub;
    friend class Proxy;

  public:
    void MonitorThread();

    CTestServer(int iEncording)
        : CZoneServer(iEncording)
    {
        // LoginZone은 하나만 등록해야하고.
        RegisterLoginZone<clsLoginZone>(L"LoginServer", 1000, (ZoneKeyType)enZoneType::LoginZone,true);
        // 동일한 방식으로 상속받아 구현한 class를 여기에 등록한다.
        ReQuestCreateZone((ZoneKeyType)enZoneType::EchoZone);
        // Zone생성 후에 MonitorThread를 생성한다.
        _MonitorThread = WinThread(&CTestServer::MonitorThread, this);
        
    }
    virtual void ReQuestCreateZone(ZoneKeyType key) override
    {
        switch (key)
        {
        case (ZoneKeyType)enZoneType::EchoZone:
            RegisterZone<clsEchoZone>(L"EchoThread", 20, (ZoneKeyType)enZoneType::EchoZone,false);
            return;
        }
        __debugbreak();
    }
    WinThread _MonitorThread;
};
