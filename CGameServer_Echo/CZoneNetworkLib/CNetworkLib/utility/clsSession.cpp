#include "clsSession.h"

#include "../utility/CSystemLog/CSystemLog.h"
#include "../utility/CTlsObjectPool/CTlsObjectPool.h"
#include "../utility/SerializeBuffer_exception/SerializeBuffer_exception.h"

#include <timeapi.h>

#include "../../CZoneNetworkLib.h"
#include "../CNetworkLib.h"

ZoneSet::ZoneSet(IZone *zone, const wchar_t *ThreadName, int deltaTime, CZoneServer *server, bool bEventUse)
    : m_zone(zone), _deltaTime(deltaTime), _hEvent(INVALID_HANDLE_VALUE), _bOn(true), _server(server), _bEventUse(bEventUse)
{
    m_zone->_server = _server;
    _hEvent = CreateEvent(nullptr, 0, 0, nullptr);

    m_Thread = WinThread(&ZoneSet::ZoneThread, this);

    SetThreadDescription(m_Thread.native_handle(), ThreadName);
}
void ZoneSet::ZoneThread()
{
    DWORD currentTime = timeGetTime();
    DWORD TargetTime = currentTime + _deltaTime;
    CMessage *msg;
    ull SessionId;
    bool bChkSum;

    DWORD retval;
    timeBeginPeriod(1);
    while (_bOn == true)
    {
        
        if (currentTime < TargetTime)
        {
            retval = WaitForSingleObject(_hEvent, TargetTime - currentTime);
            if (WAIT_TIMEOUT == retval)
                TargetTime += _deltaTime;
        }
        else
            TargetTime += _deltaTime;

        // Zone자체의 Q에서 빼기.
        do
        {
            while (q.Pop(SessionId))
            {
                clsSession *session = _server->GetSession(SessionId);

                sessions.push_back(session);
                m_zone->OnEnterWorld(SessionId, session->_addr, session->pPlayer);
            }
            for (clsSession *session : sessions)
            {
                bChkSum = true;
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
                if (session->m_ReleaseAndDBReQuest == 0)
                {
                    ZoneSet *targetZone;
                    ull SessionID = session->m_SeqID;

                    targetZone = session->m_zoneSet;

                    m_zone->OnDisConnect(SessionID);
                    iter = sessions.erase(iter);
                    InterlockedDecrement64(&sessionCount);
                    if (this != _server->GetLoginZoneSet())
                    {
                        targetZone->Push(SessionID);
                        SetEvent(targetZone->_hEvent);
                    }
                    else
                    {
                        // LoginZone의 경우 반환
                        ReleaseSession(*session);
                    }
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

                        // session에 남아있는 msg 처분.
                        while (session->m_ZoneBuffer.Pop(msg))
                        {
                             stTlsObjectPool<CMessage>::Release(msg);
                        }

                        iter = sessions.erase(iter);
                        m_zone->OnLeaveWorld(SessionID);
                        targetZone->Push(SessionID);
                        SetEvent(targetZone->_hEvent);
                        InterlockedDecrement64(&sessionCount);
                        continue;
                    }
                }

                iter++;
            }
        } while (q.m_size != 0);

        currentTime = timeGetTime();
    }
    timeEndPeriod(1);
}

void ZoneSet::ReleaseSession(clsSession &session)
{
    session.Release();
    closesocket(session.m_sock);
    _server->PushSessionStack(session.m_SeqID);
}
