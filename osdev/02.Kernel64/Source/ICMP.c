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
    if (kICMP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
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
  // 뮤텍스 초기화
  kInitializeMutex(&(gs_stICMPManager.stLock));

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
  if (kICMP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

void kICMP_SendEcho(DWORD dwDestinationAddress)
{
  ICMP_HEADER stICMPHeader = { 0, };
  FRAME stFrame;
  WORD wDataLen = 32;

  BYTE vbData[32] = { 
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
  'h', 'i'};

  stICMPHeader.bType = ICMP_TYPE_ECHO;
  stICMPHeader.bCode = ICMP_CODE_DEFAULT;

  stICMPHeader.stRRHeader.wIdentifier = htons(gs_stICMPManager.wIdentifier++);
  stICMPHeader.stRRHeader.wSequenceNumber = htons(ICMP_SEQUENCENUMBER_DEFAULT);

  // 체크섬 계산
  stICMPHeader.wChecksum = htons(kICMP_CalcChecksum(&stICMPHeader, vbData, wDataLen));

  // 캡슐화
  kAllocateFrame(&stFrame);
  kEncapuslationFrame(&stFrame, &stICMPHeader, sizeof(ICMP_HEADER), vbData, wDataLen);

  // IP로 전송
  stFrame.bType = FRAME_ICMP;
  stFrame.qwDestAddress = dwDestinationAddress;
  stFrame.eDirection = FRAME_OUT;

  kICMP_PutFrameToFrameQueue(&stFrame);
}

void kICMP_SendMessage(DWORD dwDestinationAddress, BYTE bType, BYTE bCode, IP_HEADER* pstIP_Header, BYTE* pbDatagram)
{
  ICMP_HEADER stICMPHeader = { 0, };
  FRAME stFrame;

  // IP 헤더, 64bits 원본 데이터그램
  BYTE vbData[sizeof(IP_HEADER) + 8];
  WORD wDataLen = sizeof(IP_HEADER) + 8;

  // IP 원본 헤더, 데이터 그램 8바이트 복사
  kMemCpy(vbData, pstIP_Header, sizeof(IP_HEADER));
  kMemCpy(vbData + sizeof(IP_HEADER), pbDatagram, 8);

  stICMPHeader.bType = bType;
  stICMPHeader.bCode = bCode;
  stICMPHeader.wChecksum = htons(kICMP_CalcChecksum(&stICMPHeader, vbData, wDataLen));

  // 캡슐화
  kAllocateFrame(&stFrame);
  kEncapuslationFrame(&stFrame, &stICMPHeader, sizeof(ICMP_HEADER), vbData, wDataLen);

  // IP로 전송
  stFrame.bType = FRAME_ICMP;
  stFrame.qwDestAddress = dwDestinationAddress;
  stFrame.eDirection = FRAME_OUT;

  kICMP_PutFrameToFrameQueue(&stFrame);
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

BOOL kICMP_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stICMPManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stICMPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stICMPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kICMP_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stICMPManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stICMPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stICMPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}