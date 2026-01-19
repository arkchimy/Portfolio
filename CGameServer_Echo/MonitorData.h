#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// 채팅서버 ChatServer 실행 여부 ON / OFF
// 채팅서버 ChatServer CPU 사용률
// 채팅서버 ChatServer 메모리 사용 MByte
// 채팅서버 세션 수 (컨넥션 수)
// 채팅서버 인증성공 사용자 수 (실제 접속자)
// 채팅서버 UPDATE 스레드 초당 처리 횟수
// 채팅서버 패킷풀 사용량
// 채팅서버 UPDATE MSG 풀 사용량


enum enMonitorType : BYTE
{
    TimeStamp = 0,
	On ,
	Cpu ,
	Memory,
    SessionCnt,
    AUTHCnt, // 인증대기 
    UserCnt, // 인증완료
    AcceptTPS, 
    RecvTPS,
    SendTPS,
    DB_WRITE_TPS,
    DB_WRITE_MSG,
    // 로그인 쓰레드 초당 루프 횟수.
    AUTH_THREAD_FPS, 
    GAME_THREAD_FPS,
    PACKET_POOL,
    Max,
};
enum class enMonitorTotal : BYTE
{
    TimeStamp = 0,
    CPU_TOTAL = 40,
    NONPAGED_MEMORY,
    AVAILABLE_MEMORY,
    NETWORK_RECV,
    NETWORK_SEND,
    Max,
};
extern int g_MonitorData[enMonitorType::Max];
extern int g_MonitorTotalData[(BYTE)enMonitorTotal::Max];

extern HANDLE g_hMonitorEvent;

extern void ClientFunc();
