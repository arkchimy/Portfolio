#include "clsSession.h"

#include "../utility/CSystemLog/CSystemLog.h"
#include "../utility/CTlsObjectPool/CTlsObjectPool.h"
#include "../utility/SerializeBuffer_exception/SerializeBuffer_exception.h"

#include <timeapi.h>

#include "../../CZoneNetworkLib.h"
#include "../CNetworkLib.h"

ZoneSet::ZoneSet(IZone *zone, const wchar_t *ThreadName, int deltaTime, CZoneServer *server, HANDLE hEvent)
    : m_zone(zone), _deltaTime(deltaTime), _hEvent(hEvent), _bOn(true), _server(server)
{
    m_zone->_server = _server;
    if (_hEvent == INVALID_HANDLE_VALUE)
        m_Thread = WinThread(&ZoneSet::ZoneThread, this);
    else
        m_Thread = WinThread(&ZoneSet::ZoneTimerThread, this);

    SetThreadDescription(m_Thread.native_handle(), ThreadName);
}
void ZoneSet::ZoneThread()
{
    DWORD currentTime = timeGetTime();
    DWORD TargetTime = currentTime;
    CMessage *msg;
    ull SessionId;
    timeBeginPeriod(1);
    while (_bOn == true)
    {

        TargetTime += _deltaTime;

        // Zone자체의 Q에서 빼기.
        while (q.Pop(SessionId))
        {
            clsSession *session = _server->GetSession(SessionId);

            // session에 남아있는 msg 처분.
            while (session->m_ZoneBuffer.Pop(msg))
            {
                stTlsObjectPool<CMessage>::Release(msg);
            }
            sessions.push_back(session);
            m_zone->OnEnterWorld(SessionId, session->_addr, session->pPlayer);
        }
        for (clsSession *session : sessions)
        {
            bool bChkSum = true;
            while (session->m_ZoneBuffer.Pop(msg))
            {
                if (_server->GetisEncode())
                {
                    {
                        Profiler profile(L"DeCoding");
                        bChkSum = msg->DeCoding();
                    }
                    if (bChkSum == false)
                    {
                        // Attack : 조작된 패킷으로 checkSum이 다름.
                        session->m_blive =  0;
                        stTlsObjectPool<CMessage>::Release(msg);
                        return;
                    }
                }
                msg->_frontPtr = msg->_frontPtr + _server->GetheaderSize();
                m_zone->OnRecv(session->m_SeqID, msg);
            }
        }
        m_zone->OnUpdate();

        for (auto iter = sessions.begin(); iter != sessions.end();)
        {
            clsSession *session = *iter;
            // DB요청도  IOCP에서도 연결끊김이 확인되면 호출
            if (session->m_ReleaseAndDBReQuest == 0)
            {
                ZoneSet *targetZone;
                ull SessionID = session->m_SeqID;

                targetZone = session->m_zoneSet;

                m_zone->OnDisConnect(SessionID);

                // ReleaseSession(*session);
                iter = sessions.erase(iter);

                targetZone->Push(SessionID);
                continue;
            }
            else if (session->m_zoneSet != this)
            {
                // MoveZone이 호출됬다면 this 와 다름.
                if (session->m_ReleaseAndDBReQuest == (ull)1 << 63)
                {
                    // DB 처리가 끝났다면
                    ull SessionID;
                    ZoneSet *targetZone;
     

                    SessionID = session->m_SeqID;
                    targetZone = session->m_zoneSet;

                    iter = sessions.erase(iter);
                    m_zone->OnLeaveWorld(SessionID);
                    targetZone->Push(SessionID);

                    continue;
                }
            }

            iter++;
        }

        currentTime = timeGetTime();

        if (currentTime < TargetTime)
        {
            Sleep(TargetTime - currentTime);
        }
    }
    timeEndPeriod(1);
}

void ZoneSet::ZoneTimerThread()
{

    CMessage *msg;

    timeBeginPeriod(1);

    while (_bOn == true)
    {
        WaitForSingleObject(_hEvent, _deltaTime);
        // Zone자체의 Q에서 빼기.
        ull SessionId;
        while (q.Pop(SessionId))
        {
            clsSession *session = _server->GetSession(SessionId);
            // 다른 Zone에서 LoginZone으로 돌아온경우 폐기
            if (session->pPlayer != nullptr)
            {
                // 폐기
                while (session->m_ZoneBuffer.Pop(msg))
                {
                    stTlsObjectPool<CMessage>::Release(msg);
                }
            }
            sessions.push_back(session);
            m_zone->OnEnterWorld(SessionId, session->_addr, session->pPlayer);
        }
        for (clsSession *session : sessions)
        {
            bool bChkSum = true;
            while (session->m_ZoneBuffer.Pop(msg))
            {
                if (_server->GetisEncode())
                {
                    {
                        Profiler profile(L"DeCoding");
                        bChkSum = msg->DeCoding();
                    }
                    if (bChkSum == false)
                    {
                        // Attack : 조작된 패킷으로 checkSum이 다름.
                        session->m_blive = 0;
                        stTlsObjectPool<CMessage>::Release(msg);
                        return;
                    }
                }
                msg->_frontPtr = msg->_frontPtr + _server->GetheaderSize();
                m_zone->OnRecv(session->m_SeqID, msg);

            }
        }
        m_zone->OnUpdate();


        for (auto iter = sessions.begin(); iter != sessions.end();)
        {
            clsSession *session = *iter;


            // DB요청도  IOCP에서도 연결끊김이 확인되면 호출

            if (session->m_zoneSet != this)
            {
                // MoveZone이 호출됬다면 this 와 다름.
                if (session->m_ReleaseAndDBReQuest == (ull)1 << 63)
                {
                    // DB 처리가 끝났다면
                    ull SessionID;
                    ZoneSet *targetZone;

                    SessionID = session->m_SeqID;
                    targetZone = session->m_zoneSet;

                    iter = sessions.erase(iter);
                    m_zone->OnLeaveWorld(SessionID);

                    targetZone->Push(SessionID);

                    continue;
                }
            }
            if (session->m_ReleaseAndDBReQuest == 0)
            {
                ull SessionID = session->m_SeqID;

                m_zone->OnDisConnect(SessionID);

                ReleaseSession(*session);
                iter = sessions.erase(iter);

                continue;
            }

            iter++;
        }
    }

    timeEndPeriod(1);
}

void ZoneSet::ReleaseSession(clsSession &session)
{
    session.Release();
    closesocket(session.m_sock);
    _server->PushSessionStack(session.m_SeqID);
}
