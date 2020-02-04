#include "DNS.h"
#include "UDP.h"
#include "IP.h"
#include "Utility.h"

static DNSMANAGER gs_stDNSManager = { 0, };

WORD kDNS_AddRR(DNS_QUERYRR* pstRR, BYTE* pbBuf, WORD wIndex)
{
  int i;
  BYTE* pbTemp;
  WORD wLen = 0;

  // Name (Var)
  // www.google.com -> 
  // 03 77 77 77 (www)
  // 06 67 6f 6f 67 6c 65 (google)
  // 03 63 6f 6d (com)
  // 00 (NULL)
  pbTemp = pstRR->pbName;
  while (1) {
    // 도메인 문자열을 탐색하여 구분점(.)을 찾음.
    if ((*pbTemp == '.') ||
      (*pbTemp == 0))
    {
      // 문자 수 기록
      pbBuf[wIndex++] = wLen;

      // 문자열 복사
      for (i = wLen; i > 0; i--) {
        pbBuf[wIndex++] = *(pbTemp - i);
      }
      // 카운팅 초기화
      wLen = 0;

      // 마지막에 NULL 문자 추가
      if (*pbTemp == 0) {
        pbBuf[wIndex++] = 0;
        break;
      }
    }
    // 구분점 앞에 위치한 문자 수를 카운팅 함.
    else {
      wLen += 1;
    }
    pbTemp++;
  }

  // Type(2), Class(2)
  kMemCpy(pbBuf + wIndex + wLen, &pstRR->wType, 2 + 2);
  return  wIndex + 2 + 2;
}

WORD kDNS_GetRR(DNS_HEADER* pstHeader, BYTE* pbPayload)
{
  int i;
  DNS_QUERYRR* pstQueryRR;
  DNS_ANSWERRR* pstAnswerRR;
  BYTE* pbTemp;
  WORD wOffset;
  // Query
  pstQueryRR = pbPayload;
  kPrintf("Aliases : %s\n", (BYTE*)pstQueryRR);

  // Answer
  pstAnswerRR = pbPayload + kStrLen((BYTE*)pstQueryRR) + 1 + 4;

  kPrintf("Address : ");
  for (i = 0; i < ntohs(pstHeader->wAnswer); i++) {
    if ((ntohs(pstAnswerRR->wType) == DNS_RR_TYPE_A)) {
      kPrintIPAddress(&(pstAnswerRR->pbRData));
      kPrintf("\n");
    }
    pstAnswerRR = ((BYTE*)&(pstAnswerRR->pbRData)) + ntohs(pstAnswerRR->wRDLength);
  }
}

void kDNS_SendDNSQuery(const char* pcName)
{
  FRAME stFrame;
  DNS_HEADER stDNSHeader;
  DNS_QUERYRR stDNSQueryRR;
  BYTE vbPayload[512];
  WORD wIndex = 0;

  // 헤더 설정
  stDNSHeader.wTransactionID = htons(kRandom() % 0xFFFFFFFF);
  stDNSHeader.wFlags = htons(
    DNS_FLAGS_QUERYRESPOSE_QUERY << DNS_FLAGS_QUERYRESPOSE_OFFSET |
    DNS_FLAGS_OPCODE_QUERY << DNS_FLAGS_OPCODE_OFFSET |
    1 << DNS_FLAGS_RECURSIONDESIRED_OFFSET
  );

  stDNSHeader.wQuestion = htons(0x0001);
  stDNSHeader.wAnswer = 0x0000;
  stDNSHeader.wAuthority = 0x0000;
  stDNSHeader.wAdditional = 0x0000;

  // 쿼리 설정
  stDNSQueryRR.pbName = pcName;
  stDNSQueryRR.wType = htons(DNS_RR_TYPE_A);
  stDNSQueryRR.wClass = htons(DNS_RR_CLASS_IN);
  wIndex = kDNS_AddRR(&stDNSQueryRR, vbPayload, wIndex);

  // 캡슐화
  kAllocateFrame(&stFrame);
  kEncapuslationFrame(&stFrame, &stDNSHeader, sizeof(DNS_HEADER), vbPayload, wIndex);

  // IP로 전송
  stFrame.bType = FRAME_DNS;
  // Source : My IP, Dest : DNS Server IP
  stFrame.qwDestAddress = ((QWORD)kIP_GetIPAddress(NULL) << 32) | kAddressArrayToNumber(gs_stDNSManager.vbDNSAddress, 4);
  stFrame.dwDestPort = ((kRandom() & 0xFFFF) << 16) | UDP_PORT_DNS;
  stFrame.eDirection = FRAME_DOWN;

  kDNS_PutFrameToFrameQueue(&stFrame);
}

BOOL kDNS_SetDNSAddress(BYTE* pbAddress)
{
  kMemCpy(gs_stDNSManager.vbDNSAddress, pbAddress, 4);
  return TRUE;
}

void kDNS_Task(void)
{
  DNS_HEADER* pstDNSHeader;
  BYTE* pbDNSPayload;

  FRAME stFrame;

  // 초기화
  if (kDNS_Initialize() == FALSE)
    return;

  while (1)
  {
    if (kDNS_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_DOWN:
      kPrintf("DNS | Send DNS Message\n");
      gs_stDNSManager.pfDownUDP(stFrame);
      break;
    case FRAME_UP:
      kPrintf("DNS | Recevie DNS Message\n");

      kDecapuslationFrame(&stFrame, &pstDNSHeader, sizeof(DNS_HEADER), &pbDNSPayload);

      kDNS_GetRR(pstDNSHeader, pbDNSPayload);

      kFreeFrame(&stFrame);
      break;
    }
  }
}

BOOL kDNS_Initialize(void)
{
  // 뮤텍스 초기화
  kInitializeMutex(&(gs_stDNSManager.stLock));

  // Allocate Frame Queue
  gs_stDNSManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stDNSManager.pstFrameBuffer == NULL) {
    kPrintf("kDNS_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stDNSManager.stFrameQueue), gs_stDNSManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 레이어 설정
  gs_stDNSManager.pfDownUDP = kUDP_DownDirectionPoint;

  return TRUE;
}

BOOL kDNS_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_UP;
  if (kDNS_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kDNS_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stDNSManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kDNS_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stDNSManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}