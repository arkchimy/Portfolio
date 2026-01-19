#include "CTestServer.h"
#include <Pdh.h>
#include <stdio.h>
#include "CZoneNetworkLib/CNetworkLib/utility/CCpuUsage/CCpuUsage.h"

#include "MonitorData.h"


#pragma comment(lib, "Pdh.lib")

void CTestServer::MonitorThread()
{
    // 해당 스레드는 Regist후 호출된다.
    // 
    // 
    
    clsLoginZone *loginZone = static_cast<clsLoginZone*>( _zoneMap[ZoneKeyType(enZoneType::LoginZone)]->GetZone());
    clsEchoZone *echoZone = static_cast<clsEchoZone *>(_zoneMap[ZoneKeyType(enZoneType::EchoZone)]->GetZone());


    ull LoginFPS;
    ull EchoFPS;

    ull current_LoginThreadFrameCnt;
    ull current_EchoThreadFrameCnt;

    ull old_LoginThreadFrameCnt =0;
    ull old_EchoThreadFrameCnt = 0;
    //모니터링  변수

    size_t MaxSessions = sessions_vec.size();
    ull SessionCnt; // Network에서의 Session 수
    ull AuthCnt;    // 인증 대기 Session수
    ull UserCnt;    // 인증이 완료된 Session 수


    // 이전 프레임에서의 차이를 이용
    ull old_Accept = 0 ; // 이전 프레임에서 Accept를 한 카운트.
    ull old_RecvCnt = 0; //이전 프레임에서 OnRecv를 한 카운트.

    ull current_Accept; // 이전 프레임에서 OnRecv를 한 카운트.
    ull current_RecvCnt; //이전 프레임에서 OnRecv를 한 카운트.


    ull AcceptTps;
    ull RecvTps;

    ull LoginTps;
    ull ResLoginTps;

    ull EchoTps;
    ull HeartTps;

    //
    ull old_GameServer_msgTypeCntArr[2]{0,};
    ull old_GameContents_msgTypeCntArr[2]{0,};

    ull current_GameServer_msgTypeCntArr[2];
    ull current_GameContents_msgTypeCntArr[2];


    

    timeBeginPeriod(1);

    DWORD currentTime;
    DWORD nextTime; // 내가 목표로하는 이상적인 시간.


    char ProfilerFormat[2][30] = {
        "Profiler_Mode : Off\n",
        "Profiler_Mode : On\n"};

    // PDH 쿼리 핸들 생성
    PDH_HQUERY hQuery;
    PdhOpenQuery(NULL, NULL, &hQuery);

    PDH_HCOUNTER Process_PrivateByte;
    PdhAddCounter(hQuery, L"\\Process(CGameServer_Echo)\\Private Bytes", NULL, &Process_PrivateByte);

    PDH_HCOUNTER Process_NonpagedByte;
    PdhAddCounter(hQuery, L"\\Process(CGameServer_Echo)\\Pool Nonpaged Bytes", NULL, &Process_NonpagedByte);

    PDH_HCOUNTER Available_Byte;
    PdhAddCounter(hQuery, L"\\Memory\\Available MBytes", NULL, &Available_Byte);

    PDH_HCOUNTER Nonpaged_Byte;
    PdhAddCounter(hQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &Nonpaged_Byte);


    PDH_HCOUNTER hTcp4Retrans;

    PdhAddCounter(hQuery, L"\\TCPv4\\Segments Retransmitted/sec", 0, &hTcp4Retrans);


    PdhCollectQueryData(hQuery);
    CCpuUsage CPUTime;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const double logical = (double)si.dwNumberOfProcessors;

    {
        PDH_FMT_COUNTERVALUE Process_PrivateByteVal;
        PDH_FMT_COUNTERVALUE Process_Nonpaged_ByteVal;
        PDH_FMT_COUNTERVALUE Available_Byte_ByteVal;
        PDH_FMT_COUNTERVALUE Nonpaged_Byte_ByteVal;

        PDH_FMT_COUNTERVALUE recvVal[3];
        PDH_FMT_COUNTERVALUE sentVal[3];
        PDH_FMT_COUNTERVALUE totalVal[3];
        PDH_FMT_COUNTERVALUE vTcp4Retr, vTcp4Sent, vTcp4Recv;

        currentTime = timeGetTime();
        nextTime; // 내가 목표로하는 이상적인 시간.
        nextTime = currentTime;
        LONG64 TotalTPS = 0;

        //while (1)
        //{
        //    nextTime += 1000;

        //    printf(" %-25s : %10lld\n", "PacketPool", stTlsObjectPool<CMessage>::instance.m_TotalCount);
        //    currentTime = timeGetTime();
        //    if (nextTime > currentTime)
        //        Sleep(nextTime - currentTime);
        //}
        
        //lock을 획득하고 모든 Zone을 돔.


        while (1)
        {
            nextTime += 1000;

            {
                // ull SessionCnt; // Network에서의 Session 수
                // ull AuthCnt;    // 인증 대기 Session수
                // ull UserCnt;    // 인증이 완료된 Session 수

                //// 이전 프레임에서의 차이를 이용
                // ull old_Accept;  // 이전 프레임에서 Accept를 한 카운트.
                // ull old_RecvCnt; // 이전 프레임에서 OnRecv를 한 카운트.

                //ull current_Accept;  // 현재 프레임에서 Accept를 한 카운트.
                //ull current_RecvCnt; // 현재 프레임에서 OnRecv를 한 카운트.

            }

            SessionCnt  =   GetSessionCount();
            current_RecvCnt = _RecvTotalCnt; 

            //login Thread 정보셋팅
            {
                current_LoginThreadFrameCnt = loginZone->_UpdateFrame;

                AuthCnt = loginZone->Auth_SessionCnt;
                UserCnt = loginZone->User_SessionCnt;

                current_Accept = loginZone->AcceptTps;

                //Login요청 패킷
                current_GameServer_msgTypeCntArr[0] = loginZone->_msgTypeCntArr[0];
            }

            //Echo Thread 정보
            {
                current_EchoThreadFrameCnt = echoZone->_UpdateFrame;

                current_GameServer_msgTypeCntArr[1] = echoZone->_msgTypeCntArr[0];

                current_GameContents_msgTypeCntArr[0] = echoZone->_msgTypeCntArr[1]; // 에코라 Recv,Send 동일
                current_GameContents_msgTypeCntArr[1] = echoZone->_msgTypeCntArr[2]; // 하트비트
            }
  



            
            // 계산 영역
            {
                AcceptTps = current_Accept - old_Accept;
                RecvTps = current_RecvCnt - old_RecvCnt;

                LoginTps = current_GameServer_msgTypeCntArr[0] - old_GameServer_msgTypeCntArr[0];
                ResLoginTps = current_GameServer_msgTypeCntArr[1] - old_GameServer_msgTypeCntArr[1];


                EchoTps = current_GameContents_msgTypeCntArr[0] - old_GameContents_msgTypeCntArr[0];
                HeartTps = current_GameContents_msgTypeCntArr[1] - old_GameContents_msgTypeCntArr[1];


                // 전 프레임으로 초기화
                old_Accept = current_Accept;
                old_RecvCnt = current_RecvCnt;

                old_GameServer_msgTypeCntArr[0] = current_GameServer_msgTypeCntArr[0];
                old_GameServer_msgTypeCntArr[1] = current_GameServer_msgTypeCntArr[1];


                old_GameContents_msgTypeCntArr[0] = current_GameContents_msgTypeCntArr[0];
                old_GameContents_msgTypeCntArr[1] = current_GameContents_msgTypeCntArr[1];


                // Thread TPS
                LoginFPS = current_LoginThreadFrameCnt - old_LoginThreadFrameCnt;
                EchoFPS = current_EchoThreadFrameCnt - old_EchoThreadFrameCnt;

                old_LoginThreadFrameCnt = current_LoginThreadFrameCnt;
                old_EchoThreadFrameCnt = current_EchoThreadFrameCnt;

               
            }

            printf(" ============================================ Config ============================================ \n");

            printf("%-25s : %10d  %-25s : %10lld\n", "WorkerThread Cnt", m_WorkThreadCnt, "SessionNum", GetSessionCount());

            printf("%-25s : %10d  %-25s : %10d \n", "ZeroCopy", bZeroCopy, "Nodelay", bNoDelay);
            printf("%-25s : %10zu \n", "MaxSessions", MaxSessions);

            printf(" \n========================================= Server Runtime Status ====================================== \n");

            printf(" %-25s : %10llu  \n", "Total Accept", getTotalAccept());
            printf(" %-25s : %10lld  \n", "MyCustomMessage_Cnt", getNetworkMsgCount());
            printf(" %-25s : %10lld  %-25s : %10lld\n", "PrePlayer Count", AuthCnt,"Player Count", UserCnt);

            printf(" %-25s : %10lld\n", "PacketPool", stTlsObjectPool<CMessage>::instance.m_TotalCount);
            printf(" %-25s : %10lld\n", "Total iDisconnectCount", iDisCounnectCount);

            printf(" %100s \n", ProfilerFormat[Profiler::bOn]);

            printf(" ============================================ Contents Thread TPS ========================================== \n");

            printf(" Accept TPS           : %lld\n", AcceptTps);
            printf(" Recv TPS           : %lld\n",   RecvTps);
            printf(" Send TPS           : %lld\n", EchoTps + ResLoginTps + HeartTps);

            printf(" ============================================ Contents Thread FPS ========================================== \n");
            printf(" LoginFPS           : %lld\n", LoginFPS);
            printf(" EchoFPS           : %lld\n", EchoFPS);

            printf(" ============================================ Packet TPS ========================================== \n");
            printf(" en_PACKET_CS_GAME_REQ_LOGIN          : %lld \n", LoginTps);
            printf(" en_PACKET_CS_GAME_RES_LOGIN          : %lld \n", ResLoginTps);

            printf(" en_PACKET_CS_GAME_REQ_ECHO          : %lld \n",   EchoTps);
            printf(" en_PACKET_CS_GAME_REQ_HEARTBEAT          : %lld \n",   HeartTps);

            {
                // 1초마다 갱신
                PdhCollectQueryData(hQuery);

                CPUTime.UpdateCpuTime();

                wprintf(L" ============================================ CPU Useage ============================================ \n");

                wprintf(L" [ Total ]T:%03.2f U : %03.2f  K : %03.2f \t", CPUTime.ProcessorTotal(), CPUTime.ProcessorKernel(), CPUTime.ProcessorUser());
                wprintf(L" [ Process ] T:%03.2f U : %03.2f  K : %03.2f   \n", CPUTime.ProcessTotal(), CPUTime.ProcessKernel(), CPUTime.ProcessUser());
                wprintf(L"====================================================================================================\n");

                // 갱신 데이터 얻음
                // PDH_FMT_COUNTERVALUE counterVal;

                PdhGetFormattedCounterValue(Process_PrivateByte, PDH_FMT_LARGE, NULL, &Process_PrivateByteVal);
                wprintf(L"Process_PrivateByte : %lld Byte\n", Process_PrivateByteVal.largeValue);

                PdhGetFormattedCounterValue(Process_NonpagedByte, PDH_FMT_LARGE, NULL, &Process_Nonpaged_ByteVal);
                wprintf(L"Process_Nonpaged_Byte :  %lld Byte\n", Process_Nonpaged_ByteVal.largeValue);

                PdhGetFormattedCounterValue(Available_Byte, PDH_FMT_LARGE, NULL, &Available_Byte_ByteVal);
                wprintf(L"Available_Byte :  %lld Byte\n", Available_Byte_ByteVal.largeValue);

                PdhGetFormattedCounterValue(Nonpaged_Byte, PDH_FMT_LARGE, NULL, &Nonpaged_Byte_ByteVal);
                wprintf(L"Nonpaged_Byte_ByteVal : %lld Byte\n", Nonpaged_Byte_ByteVal.largeValue);
            }

            currentTime = timeGetTime();

            time_t currenttt;
            time(&currenttt);
            // MonitorData
            {
                // InterlockedExchange((DWORD *)&g_MonitorData[enMonitorType::TimeStamp], currenttt);
                int totalCountentSize = 0;


                g_MonitorData[enMonitorType::TimeStamp] = (int)currenttt;
                g_MonitorData[enMonitorType::On] = bOn;
                g_MonitorData[enMonitorType::Cpu] = (int)CPUTime.ProcessTotal();
                g_MonitorData[enMonitorType::Memory] = (int)(Process_PrivateByteVal.largeValue / 1024 / 1024);
                g_MonitorData[enMonitorType::SessionCnt] = (int)GetSessionCount();
                g_MonitorData[enMonitorType::UserCnt] = (int)UserCnt;
                g_MonitorData[enMonitorType::AcceptTPS] = (int)AcceptTPS;
                g_MonitorData[enMonitorType::RecvTPS] = (int)RecvTps;
                g_MonitorData[enMonitorType::SendTPS] = (int)TotalTPS;
                g_MonitorData[enMonitorType::DB_WRITE_TPS] = (int)0;
                g_MonitorData[enMonitorType::DB_WRITE_MSG] = (int)0;
                g_MonitorData[enMonitorType::AUTH_THREAD_FPS] = (int)LoginFPS;
                g_MonitorData[enMonitorType::GAME_THREAD_FPS] = (int)EchoFPS;
                g_MonitorData[enMonitorType::PACKET_POOL] = (int)stTlsObjectPool<CMessage>::instance.m_TotalCount;

                SetEvent(g_hMonitorEvent);
            }

            if (nextTime > currentTime)
                Sleep(nextTime - currentTime);
        }
        
    }
}
