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
        Parser parser;
        if (parser.LoadFile(L"Config.txt") == false)
            __debugbreak();
        parser.GetValue(L"LoginDeltaTime", _LoginDeltaTime);
        parser.GetValue(L"EchoDeltaTime", _EchoDeltaTime);

        // LoginZone은 하나만 등록해야하고.
        ReQuestCreateZone((ZoneKeyType)enZoneType::LoginZone);
        ReQuestCreateZone((ZoneKeyType)enZoneType::EchoZone);
        // Zone생성 후에 MonitorThread를 생성한다.
        _MonitorThread = WinThread(&CTestServer::MonitorThread, this);
    }
    virtual void ReQuestCreateZone(ZoneKeyType key) override
    {
        std::lock_guard<SharedMutex> lock(_zoneKeyMapLock);
        switch (key)
        {
        case (ZoneKeyType)enZoneType::LoginZone:
            RegisterLoginZone<clsLoginZone>(L"LoginServer", _LoginDeltaTime, (ZoneKeyType)enZoneType::LoginZone, true);
            return;
        case (ZoneKeyType)enZoneType::EchoZone:
            RegisterZone<clsEchoZone>(L"EchoThread", _EchoDeltaTime, (ZoneKeyType)enZoneType::EchoZone, false);
            return;
        }
        __debugbreak();
    }
    int _LoginDeltaTime;
    int _EchoDeltaTime;
    WinThread _MonitorThread;
};
