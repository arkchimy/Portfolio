#pragma once
#include <timeapi.h>

using ZoneKeyType = uint8_t;

#ifndef ONCE_enZoneType
    #define ONCE_enZoneType
    enum class enZoneType : ZoneKeyType
    {
        LoginZone = 0,
        EchoZone1,
        EchoZone2,
        EchoZone3,
        EchoZone4,
        EchoZone5,
    };
    struct stPlayer
    {
        INT64 _AccountNo;
        ull _SessionID;
        DWORD _lastRecvTime;

        SOCKADDR_IN _addr;
    };
#endif
