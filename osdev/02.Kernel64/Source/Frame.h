#ifndef __FRAME_H__
#define __FRAME_H__

#include "Types.h"

#define FRAME_MAX_SIZE          1514
#define FRAME_QUEUE_MAX_COUNT   1024

typedef BOOL(*UpFunction)(FRAME);
typedef BOOL(*DownFunction)(FRAME);
typedef BOOL(*SideOutFunction)(FRAME);

typedef enum kFrameDirection
{
  FRAME_IN = 0,
  FRAME_OUT = 1,
  FRAME_UP = 2,
  FRAME_DOWN = 3
} FRAME_DIRECTION;

typedef enum kFrameType
{
  FRAME_ARP = 0,
  FRAME_ICMP = 1,
  FRAME_IP = 4,
  FRAME_TCP = 6,
  FRAME_DHCP = 5,
  FRAME_DNS = 6,
  FRAME_UDP = 17,
  FRAME_PASS = 99
} FRAME_TYPE;

#pragma pack(push, 1)

typedef struct kFrame
{
  BYTE* pbBuf;
  BYTE* pbCur;
  WORD  wLen;
  FRAME_TYPE bType;
  QWORD qwDestAddress;
  DWORD dwDestPort;
  DWORD dwRetransmitCount;
  FRAME_DIRECTION eDirection;
} FRAME;

typedef struct kReFrame
{
  QWORD qwTime;
  FRAME stFrame;
} RE_FRAME;

#pragma pack(pop)

// function

BOOL kAllocateFrame(FRAME* pstFrame);
BOOL kAllocateReFrame(RE_FRAME* pstReFrame, const FRAME* pstOriFrame);
BOOL kAllocateBiggerFrame(FRAME* pstFrame);
void kFreeFrame(FRAME* pstFrame);

void kEncapuslationFrame(FRAME* pstFrame, BYTE* pbHeader, DWORD dwHeaderSize,
  BYTE* pbPayload, DWORD dwPayloadSize);

void kDecapuslationFrame(FRAME* pstFrame, BYTE** ppbHeader, DWORD dwHeaderSize,
  BYTE** ppbPayload);

void kPrintFrame(FRAME* pstFrame);


BOOL kStub_UpDirectionPoint(FRAME stFrame);
BOOL kStub_DownDirectionPoint(FRAME stFrame);

#endif // !__FRAME_H__
