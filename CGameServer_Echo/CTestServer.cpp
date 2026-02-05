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
    
    clsLoginZone *loginZone = static_cast<clsLoginZone*>( _LoginZone->GetZone());

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


    ull currentAcceptTps;
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


    
    std::vector<ull> current_GameServer_EchomsgCnt;
    current_GameServer_EchomsgCnt.reserve(100);
    std::vector<ull> old_GameServer_EchomsgCnt;
    old_GameServer_EchomsgCnt.reserve(100);

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


    // CPU 전체 사용률
    PDH_HCOUNTER CpuTotal;
    PdhAddCounter(hQuery,
                  L"\\Processor(_Total)\\% Processor Time",
                  0, &CpuTotal);

    // 논페이지 메모리 (Bytes)
    PDH_HCOUNTER NonPagedBytes;
    PdhAddCounter(hQuery,
                  L"\\Memory\\Pool Nonpaged Bytes",
                  0, &NonPagedBytes);

    // 사용 가능 메모리 (MB)
    PDH_HCOUNTER AvailableMemoryMB;
    PdhAddCounter(hQuery,
                  L"\\Memory\\Available MBytes",
                  0, &AvailableMemoryMB);

    // 네트워크 수신량 (Bytes/sec)
    PDH_HCOUNTER NetworkRecv;
    PdhAddCounter(hQuery,
                  L"\\Network Interface(*)\\Bytes Received/sec",
                  0, &NetworkRecv);

    // 네트워크 송신량 (Bytes/sec)
    PDH_HCOUNTER NetworkSend;
    PdhAddCounter(hQuery,
                  L"\\Network Interface(*)\\Bytes Sent/sec",
                  0, &NetworkSend);


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
            current_EchoThreadFrameCnt = 0;
            current_GameServer_msgTypeCntArr[1] = 0;
            current_GameContents_msgTypeCntArr[0] = 0;
            current_GameContents_msgTypeCntArr[1] = 0;

            auto& EchoVec = _zoneKeyMap[(ZoneKeyType)enZoneType::EchoZone];

            for (int i = 0; i < EchoVec.size(); i++)
            {

                clsEchoZone *zone = static_cast<clsEchoZone*>(EchoVec[i]->GetZone());

                current_EchoThreadFrameCnt += zone->_UpdateFrame;
                current_GameServer_msgTypeCntArr[1] += zone->_msgTypeCntArr[0];

                current_GameServer_EchomsgCnt[i] = zone->_msgTypeCntArr[1];
                current_GameContents_msgTypeCntArr[0] += current_GameServer_EchomsgCnt[i];   // 에코라 Recv,Send 동일
                current_GameContents_msgTypeCntArr[1] += zone->_msgTypeCntArr[2];          // 하트비트
            }
            



            
            // 계산 영역
            {
                currentAcceptTps = current_Accept - old_Accept;
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
            printf(" %-25s : %10lld\n", "s_ActiveNode", stTlsObjectPool<CMessage>::s_ActiveNode);

            printf(" %-25s : %10lld\n", "Total iDisconnectCount", iDisCounnectCount);

            printf(" %100s \n", ProfilerFormat[Profiler::bOn]);

            printf(" ============================================ Contents Thread TPS ========================================== \n");
            printf(" EchoZone ThreadCnt :  %zu \n", _zoneKeyMap[1].size());
            printf(" Accept TPS           : %lld\n", currentAcceptTps);
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
                // 갱신 데이터 얻음
                // PDH_FMT_COUNTERVALUE counterVal;

                PdhGetFormattedCounterValue(Process_PrivateByte, PDH_FMT_LARGE, NULL, &Process_PrivateByteVal);
                PdhGetFormattedCounterValue(Process_NonpagedByte, PDH_FMT_LARGE, NULL, &Process_Nonpaged_ByteVal);

                PdhGetFormattedCounterValue(Available_Byte, PDH_FMT_LARGE, NULL, &Available_Byte_ByteVal);

                PdhGetFormattedCounterValue(Nonpaged_Byte, PDH_FMT_LARGE, NULL, &Nonpaged_Byte_ByteVal);


            }
            //   송신 데이터
            time_t currenttt;
            time(&currenttt);

            {
                // CPU (%)
                PDH_FMT_COUNTERVALUE cpuVal;
                PdhGetFormattedCounterValue(
                    CpuTotal,
                    PDH_FMT_DOUBLE,
                    nullptr,
                    &cpuVal);

                double cpuPercent = cpuVal.doubleValue;

                // 논페이지 메모리 (MB 변환)
                PDH_FMT_COUNTERVALUE nonpagedVal;
                PdhGetFormattedCounterValue(
                    NonPagedBytes,
                    PDH_FMT_LARGE,
                    nullptr,
                    &nonpagedVal);

                double nonpagedMB =
                    nonpagedVal.largeValue / (1024.0 * 1024.0);

                // 사용 가능 메모리 (이미 MB)
                PDH_FMT_COUNTERVALUE availMemVal;
                PdhGetFormattedCounterValue(
                    AvailableMemoryMB,
                    PDH_FMT_LARGE,
                    nullptr,
                    &availMemVal);

                LONG64 availableMB = availMemVal.largeValue;

                // 네트워크 수신 (KB/sec)
                PDH_FMT_COUNTERVALUE netRecvVal;
                PdhGetFormattedCounterValue(
                    NetworkRecv,
                    PDH_FMT_LARGE,
                    nullptr,
                    &netRecvVal);

                double netRecvKB =
                    netRecvVal.largeValue / 1024.0;

                // 네트워크 송신 (KB/sec)
                PDH_FMT_COUNTERVALUE netSendVal;
                PdhGetFormattedCounterValue(
                    NetworkSend,
                    PDH_FMT_LARGE,
                    nullptr,
                    &netSendVal);

                double netSendKB =
                    netSendVal.largeValue / 1024.0;

                g_MonitorTotalData[(BYTE)enMonitorTotal::TimeStamp] = (int)currenttt;
                    (int)(cpuPercent ); // 소수 제거하고 싶으면
                g_MonitorTotalData[(BYTE)enMonitorTotal::CPU_TOTAL] =
                    (int)(cpuPercent ); // 소수 제거하고 싶으면

                g_MonitorTotalData[(BYTE)enMonitorTotal::NONPAGED_MEMORY] =
                    (int)nonpagedMB;

                g_MonitorTotalData[(BYTE)enMonitorTotal::AVAILABLE_MEMORY] =
                    (int)availableMB;

                g_MonitorTotalData[(BYTE)enMonitorTotal::NETWORK_RECV] =
                    (int)netRecvKB;

                g_MonitorTotalData[(BYTE)enMonitorTotal::NETWORK_SEND] =
                    (int)netSendKB;
            }
        


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
                g_MonitorData[enMonitorType::AcceptTPS] = (int)currentAcceptTps;
                g_MonitorData[enMonitorType::RecvTPS] = (int)RecvTps;
                g_MonitorData[enMonitorType::SendTPS] = (int)TotalTPS;
                g_MonitorData[enMonitorType::DB_WRITE_TPS] = (int)0;
                g_MonitorData[enMonitorType::DB_WRITE_MSG] = (int)0;
                g_MonitorData[enMonitorType::AUTH_THREAD_FPS] = (int)LoginFPS;
                g_MonitorData[enMonitorType::GAME_THREAD_FPS] = (int)EchoFPS;
                g_MonitorData[enMonitorType::PACKET_POOL] = (int)stTlsObjectPool<CMessage>::instance.m_TotalCount;

                SetEvent(g_hMonitorEvent);
            }
            currentTime = timeGetTime();

            if (nextTime > currentTime)
                Sleep(nextTime - currentTime);
        }
        
    }
}
