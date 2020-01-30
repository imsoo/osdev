#include "ICMP.h"
#include "IP.h"
#include "DynamicMemory.h"
#include "Utility.h"

static ICMPMANAGER gs_stICMPManager = { 0, };

void kICMP_Task(void)
{
  int i;
  FRAME stFrame;

  // 초기화
  if (kICMP_Initialize() == FALSE)
    return;

  while (1)
  {
    if (kGetQueue(&(gs_stICMPManager.stFrameQueue), &stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      kPrintf("ICMP | Send ICMP Packet\n");

      gs_stICMPManager.pfSideOut(stFrame);
      break;
    case FRAME_IN:
      kPrintf("ICMP | Recevie ICMP Packet\n");
      kPrintFrame(&stFrame);
      break;
    }
  }
}

BOOL kICMP_Initialize(void)
{
  // Allocate Frame Queue
  gs_stICMPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stICMPManager.pstFrameBuffer == NULL) {
    kPrintf("kIP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stICMPManager.stFrameQueue), gs_stICMPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 레이어 설정
  gs_stICMPManager.pfSideOut = kIP_SideInPoint;

  return TRUE;
}

BOOL kICMP_SideInPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kPutQueue(&(gs_stICMPManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

void kICMP_SendEchoTest(void)
{
  ICMP_HEADER stICMPHeader = { 0, };
  FRAME stFrame;
  WORD wDataLen = 1000;
  BYTE vbTestData[32] = { 
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
  'h', 'i'};
  BYTE vbTestAddress[4] = { 10, 0, 2, 2 };

  stICMPHeader.bType = ICMP_TYPE_ECHO;
  stICMPHeader.bCode = ICMP_CODE_DEFAULT;

  stICMPHeader.stRRHeader.wIdentifier = htons(0x0001);
  stICMPHeader.stRRHeader.wSequenceNumber = htons(0x0014);

  kAllocateFrame(&stFrame);
  stFrame.wLen += (sizeof(ICMP_HEADER) + wDataLen);
  stFrame.pbCur = stFrame.pbBuf + FRAME_MAX_SIZE - stFrame.wLen;

  stFrame.bType = FRAME_ICMP;
  stFrame.qwDestAddress = kAddressArrayToNumber(vbTestAddress, 4);
  stFrame.eDirection = FRAME_OUT;

  // 체크섬 계산
  stICMPHeader.wChecksum = htons(kICMP_CalcChecksum(&stICMPHeader, vbTestData, wDataLen));

  // 헤더 복사
  kMemCpy(stFrame.pbCur, &stICMPHeader, sizeof(ICMP_HEADER));

  // 데이터 복사
  kMemCpy(stFrame.pbCur + sizeof(ICMP_HEADER), &vbTestData, wDataLen);

  stFrame.qwDestAddress = kAddressArrayToNumber(vbTestAddress, 4);
  kPutQueue(&(gs_stICMPManager.stFrameQueue), &stFrame);
}

WORD kICMP_CalcChecksum(ICMP_HEADER* pstHeader, void* pvData, WORD wDataLen)
{
  int i;
  DWORD dwSum = 0;
  WORD* pwP = pstHeader;

  for (int i = 0; i < sizeof(ICMP_HEADER) / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  pwP = pvData;
  for (int i = 0; i < wDataLen / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  // 마지막 한바이트 남은 경우
  if (((wDataLen / 2) * 2) != wDataLen) {
    dwSum += ntohs((pwP[wDataLen / 2]) & 0xFF00);
  }

  if (dwSum > 0xFFFF) {
    dwSum = ((dwSum >> 16) + (dwSum & 0xFFFF));
  }

  return ~(dwSum & 0xFFFF) & 0xFFFF;
}