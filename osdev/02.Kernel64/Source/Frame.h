#ifndef __FRAME_H__
#define __FRAME_H__

#include "Types.h"

#define FRAME_MAX_SIZE          1514
#define FRAME_QUEUE_MAX_COUNT   100

typedef BOOL(*UpFunction)(FRAME);
typedef BOOL(*DownFunction)(FRAME);
typedef BOOL(*SideOutFunction)(FRAME);

typedef enum kFrameDirection
{
  FRAME_IN = 0,
  FRAME_OUT = 1
} FRAME_DIRECTION;

typedef enum kFrameType
{
  FRAME_ARP = 0,
  FRAME_IP = 1,
  FRAME_ICMP = 2,
  FRAME_UDP = 3,
  FRAME_TCP = 4,
  FRAME_DHCP = 5
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
  FRAME_DIRECTION eDirection;
} FRAME;

#pragma pack(pop)

// function

BOOL kAllocateFrame(FRAME* pstFrame);
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
