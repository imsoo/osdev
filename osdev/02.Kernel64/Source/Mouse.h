#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "Types.h"
#include "Synchronization.h"

#define MOUSE_MAXQUEUECOUNT 100

#define MOUSE_LBUTTONDOWN 0x01
#define MOUSE_RBUTTONDOWN 0x02
#define MOUSE_MBUTTONDOWN 0x03

#pragma pack(push, 1)

typedef struct kMousePacketStruct
{
  // Status and Flag
  BYTE bButtonStatusAndFlag;

  // X Movement
  BYTE bXMovement;

  // Y Movement
  BYTE bYMovement;
} MOUSEDATA;

#pragma pack(pop)

typedef struct kMouseManagerStruct
{
  // Spinlock
  SPINLOCK stSpinLock;

  // 0 ~ 2 Value, Mouse Packet is 3 Bytes
  int iByteCount;

  // Current Mouse Packet
  MOUSEDATA stCurrentData;
} MOUSEMANAGER;

BOOL kInitializeMouse(void);
BOOL kActivateMouse(void);
void kEnableMouseInterrupt(void);
BOOL kAccumulatedMouseDataAndPutQueue(BYTE bMouseData);
BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX, int* piRelativeY);
BOOL kIsMouseDataInOutputBuffer(void);

#endif // !__MOUSE_H__
