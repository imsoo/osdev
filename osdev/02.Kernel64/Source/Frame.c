#include "Frame.h"
#include "DynamicMemory.h"
#include "Console.h"
#include "Utility.h"

BOOL kAllocateFrame(FRAME *pstFrame)
{
  pstFrame->pbBuf = kAllocateMemory(FRAME_MAX_SIZE);
  if (pstFrame->pbBuf == NULL) {
    kPrintf("kAllocateFrame Fail");
    return FALSE;
  }
  pstFrame->pbCur = pstFrame->pbBuf + FRAME_MAX_SIZE;
  pstFrame->qwDestAddress = pstFrame->wLen = pstFrame->eDirection = 0;
  return TRUE;
}

BOOL kAllocateBiggerFrame(FRAME* pstFrame)
{
  pstFrame->pbBuf = kAllocateMemory(8192 * 8);
  if (pstFrame->pbBuf == NULL) {
    kPrintf("kAllocateFrame Fail");
    return FALSE;
  }
  pstFrame->pbCur = pstFrame->pbBuf + (8192 * 8);
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

void kEncapuslationFrame(FRAME* pstFrame, BYTE* pbHeader, DWORD dwHeaderSize,
  BYTE* pbPayload, DWORD dwPayloadSize)
{
  pstFrame->wLen += dwPayloadSize + dwHeaderSize;

  if (dwPayloadSize != 0) {
    pstFrame->pbCur = pstFrame->pbCur - dwPayloadSize;
    kMemCpy(pstFrame->pbCur, pbPayload, dwPayloadSize);
  }

  pstFrame->pbCur = pstFrame->pbCur - dwHeaderSize;
  kMemCpy(pstFrame->pbCur, pbHeader, dwHeaderSize);
}

void kDecapuslationFrame(FRAME* pstFrame, BYTE** ppbHeader, DWORD dwHeaderSize,
  BYTE** ppbPayload)
{
  *ppbHeader = pstFrame->pbCur;

  pstFrame->wLen -= dwHeaderSize;
  pstFrame->pbCur += dwHeaderSize;

  if (ppbPayload != NULL)
    *ppbPayload = pstFrame->pbCur;
}

void kPrintFrame(FRAME* pstFrame)
{
  int i, j;
  BYTE* pbBuf = NULL;

  if (pstFrame->pbBuf == NULL)
    return;

  pbBuf = pstFrame->pbCur;

  for (i = 0; i < 40; i++) {
    kPrintf("%x\t", pbBuf[i]);

    if (i % 8 == 7)
      kPrintf("\n");
  }
  kPrintf("\n");
}

BOOL kStub_UpDirectionPoint(FRAME stFrame)
{
  kPrintf("kStub_UpDirectionPoint\n");
  kFreeFrame(&stFrame);
}

BOOL kStub_DownDirectionPoint(FRAME stFrame)
{
  kPrintf("kStub_DownDirectionPoint\n");
}