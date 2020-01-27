#include "IP.h"
#include "Utility.h"
#include "Ethernet.h"

static IPMANAGER gs_stIPManager = { 0, };

void kIP_Task(void)
{
  int i;
  FRAME stFrame;

  // 초기화
  if (kIP_Initialize() == FALSE)
    return;

  while (1)
  {
    if (kGetQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      gs_stIPManager.pfUp(stFrame);
      break;
    case FRAME_IN:
      gs_stIPManager.pfDown(stFrame);
      break;
    }
  }
}

BOOL kIP_UpDirectionPoint(FRAME stFrame)
{
  if (kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

static BOOL kIP_Initialize(void)
{
  // Allocate Frame Queue
  gs_stIPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stIPManager.pstFrameBuffer == NULL) {
    kPrintf("kIP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // 레이어 설정
  gs_stIPManager.pfDown = kEthernet_DownDirectionPoint;
  gs_stIPManager.pfUp = kStub_UpDirectionPoint;

  // Inint Frame Queue
  kInitializeQueue(&(gs_stIPManager.stFrameQueue), gs_stIPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  return TRUE;
}