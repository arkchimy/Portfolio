#include "clsLoginZone.h"

#include <cpp_redis/cpp_redis>

#pragma comment(lib, "cpp_redis.lib")
#pragma comment(lib, "tacopie.lib")
#pragma comment(lib, "ws2_32.lib")

thread_local cpp_redis::client *client;
thread_local bool bRedisOnce ;

void clsLoginZone::OnEnterWorld(ull SessionID, SOCKADDR_IN &addr, void *pPlayer)
{
    stPlayer *player;

    // 한번 만.
    if (bRedisOnce == false)
    {
        bRedisOnce = true;
        client = new cpp_redis::client();

        client->connect(_server->RedisIpAddress, 6379);
    }


    if (pPlayer != nullptr)
    {
        // 다른 Zone에 존재하다가 LoginZone으로 삭제를 위해 넘어온 경우.
        auto prePlayerIter = prePlayer_hash.find(SessionID);// 없어야함.
        auto sessionIDIter = SessionID_hash.find(SessionID); // 있어야함.

        // 이미 있었다면 문제임
        if (prePlayerIter != prePlayer_hash.end() || sessionIDIter == SessionID_hash.end())
        {
            __debugbreak();
        }

        player = static_cast<stPlayer *>(pPlayer);
        prePlayer_hash.insert({SessionID, player});

        return;
    }


    {
  
        auto prePlayerIter = prePlayer_hash.find(SessionID);
        auto sessionIDIter = SessionID_hash.find(SessionID);

        // 이미 있었다면 문제임
        if (prePlayerIter != prePlayer_hash.end() || sessionIDIter != SessionID_hash.end())
        {
            __debugbreak();
        }
        player = (stPlayer*)player_pool.Alloc();
        prePlayer_hash.insert({SessionID, player});

    }
}

void clsLoginZone::OnRecv(ull SessionID, CMessage *msg)
{
    // 최근에온 메세지에 대한 시간체크 때문에 Player를 찾아야 함.
    auto iter = prePlayer_hash.find(SessionID);
    if (iter == prePlayer_hash.end())
    {
        // Session마다 msg큐가 빌때까지 돌고있기에. 
        stTlsObjectPool<CMessage>::Release(msg);
        _server->Disconnect(SessionID);

        CSystemLog::GetInstance()->Log(L"OnDisConnect", en_LOG_LEVEL::ERROR_Mode, L"LoginZone_DisConnect %20s SessionID : %lld ",
                                       L"prePlayer_hash Not Found", SessionID);
        return;
    }
    if (PacketProc(SessionID, msg) == false)
    {
        stTlsObjectPool<CMessage>::Release(msg);
        _server->Disconnect(SessionID);
        CSystemLog::GetInstance()->Log(L"OnDisConnect", en_LOG_LEVEL::ERROR_Mode, L"LoginZone_DisConnect %20s SessionID : %lld ",
                                       L" PacketProc false return ", SessionID);
        return;
    }
    iter->second->_lastRecvTime = timeGetTime();

    totalPacketCnt++;
}

void clsLoginZone::OnUpdate()
{
    DWORD currentTime = timeGetTime();
    DWORD distance;

    //TODO : 하트비트 현재 하고있지않음.
    /*for (auto &iter : prePlayer_hash)
    {
        stPlayer *player = iter.second;
        distance = currentTime - player->_lastRecvTime;
        if (distance >= _sessionTimeoutMs)
        {
            _server->Disconnect(player->_SessionID);
        }
    }*/
}

void clsLoginZone::OnLeaveWorld(ull SessionID)
{
    //LoginZone의 연결을 의미하는 prePlayer_Hash에서만 제거.
    auto iter = prePlayer_hash.find(SessionID);
    if (iter == prePlayer_hash.end())
    {
        // 없을 수가 없음.
        __debugbreak();
    }
    prePlayer_hash.erase(iter);
}

void clsLoginZone::OnDisConnect(ull SessionID)
{
    stPlayer *player;
    INT64 AccountNo;

    auto iter = prePlayer_hash.find(SessionID);
    //CSystemLog::GetInstance()->Log(L"OnDisConnect_NoError", en_LOG_LEVEL::ERROR_Mode,
    //                               L"LoginZone_DisConnect SessionID : %lld AccountNo : %lld", SessionID , iter->second->_AccountNo);

    if (iter == prePlayer_hash.end())
    {
        // 없으면 안 됨.
        __debugbreak();
    }
    player = iter->second;
    AccountNo = player->_AccountNo;

    // 중복제거로인한 다른 sessionID가 들어가있을수 있음.
    if (player->_SessionID == SessionID)
        prePlayer_hash.erase(iter);

    {
        auto iter = SessionID_hash.find(SessionID);
        // Login인증 패킷이 오기전에 연결이 끊기는 경우가 존재.
        if (iter != SessionID_hash.end())
        {
            SessionID_hash.erase(iter);
            auto iter = Account_hash.find(AccountNo);
            // 중복 로그인이라면 있던 player를 끊음.
            // 때문에 iter가 갖고있는 player의 sessionID가 새로 들어온 녀석일 수 있음.
            if (iter != Account_hash.end())
            {
                if( SessionID == iter->second->_SessionID)
                    Account_hash.erase(iter);
            }
            else
            {
                //SessionID_hash는 추가되었는데 Account_hash에 남는경우는
                // 말도안되는 상황.
                __debugbreak();
            }
        }

        // Player반환은 여기서.
        player_pool.Release(player);
    }
}

bool clsLoginZone::PacketProc(ull SessionID, CMessage *msg)
{
    WORD wType;
    try
    {
        *msg >> wType;
    }
    catch (const MessageException &e)
    {
        switch (e.type())
        {
        case MessageException::ErrorType::HasNotData:
        {
            static bool bOn = false;
            if (bOn == false)
            {
                bOn = true;
                CSystemLog::GetInstance()->Log(L"Attack", en_LOG_LEVEL::ERROR_Mode,
                                               L"%-20s %20s %05d  ",
                                               L" msg >> Data  Faild ",
                                               L"wType", wType);
            }
            break;
        }
        case MessageException::ErrorType::NotEnoughSpace:
        {
            static bool bOn = false;
            if (bOn == false)
            {
                bOn = true;
                CSystemLog::GetInstance()->Log(L"Attack", en_LOG_LEVEL::ERROR_Mode,
                                               L"%-20s %20s %05d  ",
                                               L"NotEnoughSpace  : ",
                                               L"wType", wType);
            }
        }
            msg->HexLog(CMessage::en_Tag::_ERROR, L"Attack.txt");
            break;
        }

        return false;
    }
    switch (wType)
    {
    case en_PACKET_CS_GAME_REQ_LOGIN:
    {
        try
        {
            INT64 AccountNo;
            WCHAR SessionKey[32];
            *msg >> AccountNo;
            msg->GetData(SessionKey, 64);
            REQ_LOGIN(SessionID, msg, AccountNo, SessionKey);
        }
        catch (const MessageException &e)
        {
            switch (e.type())
            {
            case MessageException::ErrorType::HasNotData:
                break;
            case MessageException::ErrorType::NotEnoughSpace:
                break;
            }
            return false;
        }
        _msgTypeCntArr[LoginPacket]++;
        break;
    }

    default:
        return false;
    }

    return true;
}

void clsLoginZone::REQ_LOGIN(ull SessionID, CMessage *msg, INT64 AccountNo, WCHAR *SessionKey, WORD wType, BYTE bBroadCast, std::vector<ull> *pIDVector, size_t wVectorLen)
{
    stPlayer *player;

    {
        // Login응답.
        // redis에서 읽기, 가져오고 token을 비교 같다면

        std::string key = std::to_string(AccountNo);
        auto future = client->get(key.c_str());
        client->sync_commit();
        cpp_redis::reply reply = future.get();

        if (reply.is_null())
        {
            //TODO : Token 현재 비교안함.
            // 
            //printf("AccountNo %lld not found in redis\n", AccountNo);
            //__debugbreak();
            //return;
        }
        else
        {
            std::string sessionKey = reply.as_string();

            char SessionKeyA[66];
            memcpy(SessionKeyA, SessionKey, 64);
            SessionKeyA[64] = '\0';
            SessionKeyA[65] = '\0';

            key = std::to_string(AccountNo);

            if (sessionKey.compare(SessionKeyA) != 0)
            {
                CSystemLog::GetInstance()->Log(L"ContentsLog", en_LOG_LEVEL::ERROR_Mode,
                                               L"%-20s %10s %12s %10s ",
                                               L"LoginError - hash is Not Equle: ", SessionKeyA,
                                               L"현재들어온ID:", sessionKey);

                stTlsObjectPool<CMessage>::Release(msg);
                _server->Disconnect(SessionID);

                CSystemLog::GetInstance()->Log(L"OnDisConnect", en_LOG_LEVEL::ERROR_Mode, L"LoginZone_DisConnect %20s SessionID : %lld ",
                                               L" REQ_LOGIN false return ", SessionID);
                return;
            }
        }
    }

    {
        // 요청 메세지로온 AccounNo가 연결된 Session 중에 
        // 현재 존재하는지 체크.
        auto iter = Account_hash.find(AccountNo);
        // 중복 로그인이라면 있던 player를 끊음.
        if (iter != Account_hash.end())
        {
            player = iter->second;

            // 이 구간에서 직접 Account_hash에 대한 부분을 지움.
            _server->Disconnect(player->_SessionID);
            
            Account_hash.erase(iter);
            CSystemLog::GetInstance()->Log(L"OnDisConnect", en_LOG_LEVEL::ERROR_Mode, L"LoginZone_DisConnect %20s SessionID : %lld AccountNo : %lld",
                                           player->_SessionID,AccountNo);

        }

        auto prePlayeriter = prePlayer_hash.find(SessionID);
        if (prePlayeriter == prePlayer_hash.end())
        {
            // LoginPack을 받으면 바로 제거후  Zone을 옮겨주므로 여기 올 수가 없음.
            // TODO : 공격이 아니라면 있을 수 없으므로 일단은 납둠.
            __debugbreak();
        }
        player = prePlayeriter->second;
        player->_AccountNo = AccountNo;

        {

            // 전환 완료 메세지가 없으므로
            auto iter = SessionID_hash.find(SessionID);
            if (iter != SessionID_hash.end())
            {
                // 이제 추가하는 것이라 있으면 안됨.
                __debugbreak();
            }
            //ZoneSession에서 제거 후 Account와 SessionID 추가.
            // prePlayer_hash.erase(prePlayeriter); <= 지우는 것은 Leave에서

            SessionID_hash.insert({SessionID, player});
            Account_hash.insert({AccountNo, player});

            _server->RequeseMoveZone(SessionID, (ZoneKeyType)enZoneType::EchoZone,player);
        }
    }
}
