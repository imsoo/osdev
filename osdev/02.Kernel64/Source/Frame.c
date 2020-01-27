#include "Frame.h"
#include "DynamicMemory.h"
#include "Console.h"

BOOL kAllocateFrame(FRAME *pstFrame)
{
  pstFrame->pbBuf = kAllocateMemory(FRAME_MAX_SIZE);
  if (pstFrame->pbBuf == NULL) {
    kPrintf("kAllocateFrame Fail");
    return FALSE;
  }
  pstFrame->pbCur = pstFrame->pbBuf;
  pstFrame->qwDestAddress = pstFrame->wLen = pstFrame->eDirection = 0;
  return TRUE;
}

void kFreeFrame(FRAME *pstFrame)
{
  if (pstFrame->pbBuf == NULL)
    return;

  kFreeMemory(pstFrame->pbBuf);
  pstFrame->pbBuf = NULL;
}

void kPrintFrame(FRAME* pstFrame)
{
  int i, j;
  BYTE* pbBuf = NULL;

  if (pstFrame->pbBuf == NULL)
    return;

  pbBuf = pstFrame->pbCur;

  for (i = 0; i < pstFrame->wLen; i++) {
    kPrintf("%x\t", pbBuf[i]);

    if (i % 8 == 7)
      kPrintf("\n");
  }
  kPrintf("\n");
}

BOOL kStub_UpDirectionPoint(FRAME stFrame)
{
  kPrintf("kStub_UpDirectionPoint\n");
  kPrintFrame(&stFrame);
  kFreeFrame(&stFrame);
}

BOOL kStub_DownDirectionPoint(FRAME stFrame)
{
  kPrintf("kStub_DownDirectionPoint\n");
}