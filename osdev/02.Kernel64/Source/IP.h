#ifndef __IP_H__
#define __IP_H__

#include "Types.h"
#include "Queue.h"
#include "Frame.h"

#pragma pack(push, 1)

typedef struct kIPManager
{
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUp;
  DownFunction pfDown;
} IPMANAGER;

#pragma pack(pop)

// function
void kIP_Task(void);
BOOL kIP_UpDirectionPoint(FRAME stFrame);
BOOL kIP_DownDirectionPoint(FRAME stFrame);


static BOOL kIP_Initialize(void);

#endif // !__IP_H__
