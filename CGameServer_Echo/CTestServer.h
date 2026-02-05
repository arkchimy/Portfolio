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
        RegisterLoginZone<clsLoginZone>(L"LoginServer", 5000, (ZoneKeyType)enZoneType::LoginZone);

        Parser parser;
        if (parser.LoadFile(L"Config.txt") == false)
            __debugbreak();
        parser.GetValue(L"EchoThreadCnt", _EchoThreadCnt);
        // 동일한 방식으로 상속받아 구현한 class를 여기에 등록한다.
        for (int i = 1; i <= _EchoThreadCnt; i++)
        {
            RegisterZone<clsEchoZone>(L"EchoThread", 20, i);
        }
        _MonitorThread = WinThread(&CTestServer::MonitorThread, this);
        
    }
    WinThread _MonitorThread;
};
