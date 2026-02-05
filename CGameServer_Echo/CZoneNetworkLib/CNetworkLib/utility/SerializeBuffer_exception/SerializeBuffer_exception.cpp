// SerializeBuffer_MT.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//
#include <strsafe.h>
#include "../Parser/Parser.h"
#include "../SerializeBuffer_exception/SerializeBuffer_exception.h"

static int g_mode = 0;

static const wchar_t *format[(BYTE)CMessage::en_Tag::MAX] =
    {
        L"\n%s\n",
        L"\n  %-15s  \n%s   \n",
        L"\n  %-15s  \n%s   \n",
        L"\n  %-15s  \n%s   \n",
        L"\n  %-15s  \n%s   \n",
        L"\n  %-15s  \n%s   \n",
};
static const wchar_t *Stringformat[(BYTE)CMessage::en_Tag::MAX] = 
{
    L"==================================================================================================================",
    L"Dummy가 보낸 인코딩 후 데이터  ",
    L"Server가 준 디코딩 후 데이터 ",
    L"Dummy가 보낸 인코딩 전 데이터  ",
    L"Server가 준 디코딩 전 데이터  ",
    L"dif Data",

};


CMessage::CMessage()
    : ownerID(GetCurrentThreadId())
{

    _end = _begin + _size;
    _frontPtr = _begin ;
    _rearPtr = _frontPtr;
}

CMessage::~CMessage()
{
    _size = en_BufferSize::bufferSize;

    _frontPtr = _begin;
    _rearPtr = _begin;
    _end = _begin + _size;

    _interlockedexchange64(&iUseCnt, 1);
}

void CMessage::InitMessage()
{
    _size = en_BufferSize::bufferSize;

    _frontPtr = _begin;
    _rearPtr = _begin;
    _end = _begin + _size;

    _interlockedexchange64(&iUseCnt, 1);
}

void CMessage::EnCoding( )
{
    // 원본과 동일한 타입/의미 유지
    SerializeBufferSize len;
    BYTE RK;
    BYTE total = 0;

    BYTE P = 0;
    BYTE E = 0;

    char *local_Front = _begin + offsetof(stHeader, byCheckSum);
    len = SerializeBufferSize(_rearPtr - local_Front);

    // (원본에는 len > _size 체크가 없었음) -> 동일성 위해 그대로 두는 게 안전
    // 필요하면 assert용으로만 추가하는 걸 추천

    // RK 생성: 원본과 동일한 방식 유지 (분포/범위까지 동일)
    RK = rand() % UCHAR_MAX;
    *reinterpret_cast<BYTE *>(_begin + offsetof(stHeader, byRandKey)) = RK;

    // 1) checksum 계산: 원본과 완전 동일 (1..len-1)
    //    (메모리 접근만 조금 더 단순하게)
    for (SerializeBufferSize i = 1; i < len; ++i)
        total = (BYTE)(total + (BYTE)local_Front[i]);

    local_Front[0] = (char)total;

    // 2) 암호화: 원본의 current=1..len, local_Front[current-1] => index 0..len-1
    for (SerializeBufferSize i = 0; i < len; ++i)
    {
        const SerializeBufferSize current = i + 1;

        const BYTE D1 = (BYTE)local_Front[i];

        // b = (P + RK + current)
        // BYTE로 wrap 타이밍을 고정해서 원본과 동일 보장
        const BYTE b = (BYTE)(P + (BYTE)(RK + (BYTE)current));

        P = (BYTE)(D1 ^ b);
        E = (BYTE)(P ^ (BYTE)(E + (BYTE)(K + (BYTE)current)));

        local_Front[i] = (char)E;
    }
}

bool CMessage::DeCoding( )
{
    uint8_t P1 = 0, E1 = 0;
    uint8_t RK = *(_begin + offsetof(stHeader, byRandKey));

    uint8_t *p = reinterpret_cast<uint8_t *>(_begin + offsetof(stHeader, byCheckSum));
    uint8_t *end = reinterpret_cast<uint8_t *>(_rearPtr);

    // 디코딩 대상 길이
    const ptrdiff_t lenSigned = end - p;
    if (lenSigned <= 0 || lenSigned > static_cast<ptrdiff_t>(_size))
        return false;

    const uint32_t len = static_cast<uint32_t>(lenSigned);

    // p[0]은 checksum, p[1..len-1]이 검증에 사용됨(기존 코드와 동일)
    uint32_t sum = 0;

    // i는 1부터 시작 (기존 current=1 의미)
    for (uint32_t i = 1; i < len; ++i)
    {
        const uint8_t x = p[i - 1];                    // E2 (암호화된 바이트)
        const uint8_t t = static_cast<uint8_t>(K + i); // (K + current)

        const uint8_t P2 = static_cast<uint8_t>(x ^ static_cast<uint8_t>(E1 + t));
        E1 = x;

        const uint8_t D2 = static_cast<uint8_t>(P2 ^ static_cast<uint8_t>(P1 + static_cast<uint8_t>(RK + i)));
        P1 = P2;

        p[i - 1] = D2;

        // checksum 누적: 기존 두 번째 루프는 local_Front[1..len-1]을 더함
        // 지금은 i-1 위치에 D2를 썼으니, i-1이 0이 아닌 시점부터 더하면 됨
        // i-1 == 0일 때는 checksum byte 자리라 누적하면 안 됨
        if (i >= 2)
            sum += D2;
    }

    // checksum 비교
    if (p[0] != static_cast<uint8_t>(sum))
        return false;

    return true;
}

SSIZE_T CMessage::PutData(PVOID src, SerializeBufferSize size)
{
    char *r = _rearPtr;
    if (r + size > _end)
        throw MessageException(MessageException::NotEnoughSpace, "Buffer OverFlow\n");
    memcpy(r, src, size);
    _rearPtr += size;
    return _rearPtr - r;
}

SSIZE_T CMessage::GetData(PVOID desc, SerializeBufferSize size)
{
    char *f = _frontPtr;
    if (f + size > _rearPtr)
    {
        throw MessageException(MessageException::HasNotData, "buffer has not Data\n");
    }
    memcpy(desc, f, size);
    _frontPtr += size;
    return _frontPtr - f;
}

// 지금 까지의 모든 데이터를 새로 할당받은 메모리에 복사후 그대로 진행해야 함.
BOOL CMessage::ReSize()
{
    // 직렬화 버퍼는 넣고 뺴고는 하나의 쓰레드에서 할 것으로 예상이 된다.
    SerializeBufferSize UseSize;

    _size = en_BufferSize::MaxSize;

    // TODO : 복사 범위 생각해보기.
    //    f     =>    r 인 경우
    // case : _frontPtr < _rearPtr  옮길 데이터가 없는 상황.
    //    r       f   인 경우 데이터를 옮겨야 함.
    if (_frontPtr > _rearPtr)
    {
        UseSize = SerializeBufferSize(_rearPtr - _frontPtr);
        memcpy(_end, _begin, UseSize);
    }
    else
        UseSize = SerializeBufferSize(_frontPtr - _rearPtr);

    _end = _begin + _size;
    _frontPtr = _begin;
    _rearPtr = _begin + UseSize;
    printf("ReSize\n");
    return TRUE;
}

void CMessage::Peek(char *out, SerializeBufferSize size)
{
    char *f = _frontPtr;
    if (f + size > _rearPtr)
        throw MessageException(MessageException::HasNotData, "buffer has not Data\n");
    memcpy(out, f, size);
}


void CMessage::HexLog(en_Tag tag , const wchar_t * filename)
{
    int current = 0;

    wchar_t *hexBuffer = (wchar_t*)malloc(MaxSize * 3 + 4); // 최대 바이트와 띄어쓰기, 널문자 까지 포함.
    wchar_t *printBuffer = (wchar_t *)malloc(MaxSize * 4);
    if (hexBuffer == nullptr)
        __debugbreak();
    if (printBuffer == nullptr)
        __debugbreak();

    wchar_t wordSet[] = L"0123456789ABCDEF";
    BYTE data;

    for (; &_begin[current] != _end; current++)
    {
        data = _begin[current];

        hexBuffer[3 * current + 0] = wordSet[data >> 4];
        hexBuffer[3 * current + 1] = wordSet[data & 0xF];
        hexBuffer[3 * current + 2] = L' ';
    }
    hexBuffer[3 * current + 0] = L'\0';

    FILE *file;
    file = nullptr;
    while (file == nullptr)
    {
        _wfopen_s(&file, filename, L"a+ ,ccs=UTF-16LE");
    }
    if ((BYTE)tag == 0)
    {
        StringCchPrintfW(printBuffer, MaxSize * 4 / sizeof(wchar_t), format[(BYTE)tag], Stringformat[(BYTE)tag]);
    }
    else
        StringCchPrintfW(printBuffer, MaxSize * 4 / sizeof(wchar_t), format[(BYTE)tag], Stringformat[(BYTE)tag], hexBuffer);
    fwrite(printBuffer, 2, wcslen(printBuffer), file);
    current = 0;
    for (; &_begin[current] != _end; current++)
    {
        hexBuffer[3 * current + 0] = L' ';
        hexBuffer[3 * current + 1] = L' ';
        hexBuffer[3 * current + 2] = L' ';
    }
    hexBuffer[3 * current + 0] = L'\n';
    hexBuffer[3 * current + 1] = L'\0';

    hexBuffer[3 * (_frontPtr - _begin)] = L'F';
    hexBuffer[3 * (_rearPtr - _begin)] = L'R';
    if ((BYTE)tag != 0)
    {
        fwrite(hexBuffer, 2, wcslen(hexBuffer), file);
    }
    fclose(file);

    free(hexBuffer);
    free(printBuffer);
}

