#include "TCP.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"


static TCPMANAGER gs_stTCPManager = { 0, };

void kTCP_Task(void)
{
  TCP_TCB* pstTCB = NULL;
  TCP_HEADER* pstHeader;
  BYTE* pbIPPayload;
  QWORD qwSocketPair;

  FRAME stFrame;
  BYTE* pbTCPPayload;

  // 초기화
  if (kTCP_Initialize() == FALSE)
    return;

  while (1)
  {
    // 큐 확인
    if (kTCP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_UP:
      kPrintf("TCP | Receive TCP Segment \n");

      // 세그먼트 헤더 분리
      kDecapuslationSegment(&stFrame, &pstHeader, &pbIPPayload);

      // 소켓 정보 생성 (Destination Address, Port, Local Port);

      // IP Header의 근원지 주소, TCP Header의 근원지 포트, 목적지 포트
      qwSocketPair = ((stFrame.qwDestAddress >> 32) << 32) |
        (pstHeader->wSourcePort << 16) | (pstHeader->wDestinationPort);

      // 소켓 검색
      pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), qwSocketPair);

      // 소켓이 없는 경우
      if (pstTCB == NULL) {
        // ICMP Destination Unreachable Message (PORT UNREACHABLE)
        kFreeFrame(&stFrame);
        break;
      }

      // 포트 번호에 따라 멀티 플렉싱
      kTCP_PutFrameToTCB(pstTCB, &stFrame);
      break;  /* End of case FRAME_UP: */

    case FRAME_DOWN:
      kPrintf("TCP | Send TCP Segment \n");

      // IP로 전송
      gs_stTCPManager.pfDownIP(stFrame);
      break;  /* End of case FRAME_DOWN: */
    }
  }
}

DWORD kTCP_GetISS(void)
{
  QWORD qwNow = kGetTickCount();
  QWORD qwDiff;
  QWORD dwISS;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stTCPManager.stLock));

  // 밀리 세컨드 차이
  qwDiff = qwNow - gs_stTCPManager.qwISSTime;

  // 4 마이크로 세컨드마다 1 증가
  dwISS = (gs_stTCPManager.dwISS + (qwDiff * 250)) % 4294967296;
  gs_stTCPManager.dwISS = dwISS;

  kUnlock(&(gs_stTCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return dwISS;
}

BOOL kTCP_Initialize(void)
{
  // 뮤텍스 초기화
  kInitializeMutex(&(gs_stTCPManager.stLock));

  // 순서 번호 초기화
  gs_stTCPManager.qwISSTime = kGetTickCount();
  gs_stTCPManager.dwISS = 0;

  // Allocate Frame Queue
  gs_stTCPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stTCPManager.pstFrameBuffer == NULL) {
    kPrintf("kTCP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stTCPManager.stFrameQueue), gs_stTCPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // TCB 리스트 초기화
  kInitializeList(&(gs_stTCPManager.stTCBList));
  gs_stTCPManager.pstCurrentTCB = NULL;

  // 레이어 설정
  gs_stTCPManager.pfDownIP = kIP_DownDirectionPoint;

  return TRUE;
}

BYTE kTCP_PutRequestToTCB(const TCP_TCB* pstTCB, const TCP_REQUEST* pstRequest)
{
  BYTE bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  bResult = kPutQueue(&(pstTCB->stRequestQueue), pstRequest);

  kUnlock(&(pstTCB->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BYTE kTCP_GetRequestFromTCB(const TCP_TCB* pstTCB, TCP_REQUEST* pstRequest)
{
  BYTE bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  bResult = kGetQueue(&(pstTCB->stRequestQueue), pstRequest);

  kUnlock(&(pstTCB->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BYTE kTCP_PutFrameToTCB(const TCP_TCB* pstTCB, const FRAME* pstFrame)
{
  BYTE bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  bResult = kPutQueue(&(pstTCB->stFrameQueue), pstFrame);

  kUnlock(&(pstTCB->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BYTE kTCP_GetFrameFromTCB(const TCP_TCB* pstTCB, FRAME* pstFrame)
{
  BYTE bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  bResult = kGetQueue(&(pstTCB->stFrameQueue), pstFrame);

  kUnlock(&(pstTCB->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

TCP_TCB* kTCP_CreateTCB(WORD wLocalPort, QWORD qwForeignSocket, TCP_FLAG eFlag)
{
  TCP_REQUEST stRequest;
  TCP_TCB* pstTCB = NULL;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stTCPManager.stLock));

  // TCB 리스트 검색
  QWORD qwSocketPair = (qwForeignSocket << 16) | wLocalPort;
  pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), qwSocketPair);

  // 소켓 쌍에 할당된 TCB 가 이미 존재하는 경우.
  if (pstTCB != NULL) {
    // TODO : 처리 코드 추가
    kPrintf("kTCP_CreateTCB | TCB already exists\n");
    return NULL;
  }

  // TCB 할당
  pstTCB = (TCP_TCB*)kAllocateMemory(sizeof(TCP_TCB));
  if (pstTCB == NULL) {
    kPrintf("kTCP_CreateTCB | TCB Allocate Fail\n");
    return NULL;
  }

  // TCB 초기화

  // 요청 큐 할당
  pstTCB->pstRequestBuffer = (TCP_REQUEST*)kAllocateMemory(TCP_REQUEST_QUEUE_MAX_COUNT * sizeof(TCP_REQUEST));
  if (pstTCB->pstRequestBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | RequestBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    return NULL;
  }
  // 요청 큐 초기화
  kInitializeQueue(&(pstTCB->stRequestQueue), pstTCB->pstRequestBuffer, TCP_REQUEST_QUEUE_MAX_COUNT, sizeof(TCP_REQUEST));

  // 프레임 큐 할당
  pstTCB->pstFrameBuffer = (TCP_REQUEST*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (pstTCB->pstFrameBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | FrameBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    kFreeMemory(pstTCB->pstRequestBuffer);
    return NULL;
  }
  // 프레임 큐 초기화
  kInitializeQueue(&(pstTCB->stFrameQueue), pstTCB->pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 뮤텍스 초기화
  kInitializeMutex(&(pstTCB->stLock));

  // TCB 정보 초기화
  pstTCB->stLink.qwID = qwSocketPair;
  pstTCB->eState = TCP_CLOSED;

  // TCB 리스트에 추가
  gs_stTCPManager.pstCurrentTCB = pstTCB;
  kAddListToHeader(&(gs_stTCPManager.stTCBList), pstTCB);

  // Request 삽입
  stRequest.eCode = TCP_OPEN;
  stRequest.eFlag = eFlag;
  kTCP_PutRequestToTCB(pstTCB, &stRequest);
  
  // TCB 담당 상태 머신 생성
  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_HIGH, 0, 0,
    (QWORD)kTCP_Machine, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kTCP_Machine Fail\n");
  }

  // 머신 생성 완료
  while (gs_stTCPManager.pstCurrentTCB != NULL);

  kUnlock(&(gs_stTCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return pstTCB;
}

BYTE kTCP_InitTCB(TCP_TCB* pstTCB)
{
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  pstTCB->dwISS = kTCP_GetISS();
  pstTCB->dwSendUNA = pstTCB->dwISS;
  pstTCB->dwSendNXT = pstTCB->dwISS;

  //  수신 윈도우 할당
  pstTCB->dwRecvWND = TCP_WINDOW_DEFAULT_SIZE;
  pstTCB->pbRecvWindow = pstTCB->dwRecvWND;
  if (pstTCB->pbRecvWindow == NULL) {
    kUnlock(&(pstTCB->stLock));
    // --- CRITCAL SECTION END ---
  }

  kUnlock(&(pstTCB->stLock));
  // --- CRITCAL SECTION END ---
}

BYTE* kTCP_FreeTCB(TCP_TCB* pstTCB)
{

}

void kTCP_Machine(void)
{
  DWORD dwDestAddress;
  QWORD qwTime;
  FRAME stFrame;
  TCP_REQUEST stRequest;
  TCP_HEADER stHeader = { 0, };
  BOOL bRequest = FALSE;
  BOOL bFrame = FALSE;
  // TCB 가져오기
  TCP_TCB* pstTCB = gs_stTCPManager.pstCurrentTCB;
  if (pstTCB == NULL)
    return;
  gs_stTCPManager.pstCurrentTCB = NULL;

  // 기본 헤더 생성
  stHeader.wSourcePort = htons(pstTCB->stLink.qwID & 0xFFFF);
  stHeader.wDestinationPort = htons((pstTCB->stLink.qwID >> 16) & 0xFFFF);
  dwDestAddress = (pstTCB->stLink.qwID >> 32);

  kPrintf("kTCP_Machine\n");

  qwTime = kGetTickCount();
  while (1)
  {
    if ((kGetTickCount() - qwTime) >= 5000) {
      qwTime = kGetTickCount();
      kPrintf("TCP_Machine Socket : %x | State : %d\n", pstTCB->stLink.qwID, pstTCB->eState);
    }

    // 요청 처리
    // 큐 확인
    bRequest = kTCP_GetRequestFromTCB(pstTCB, &stRequest);

    switch (pstTCB->eState)
    {
    case TCP_CLOSED:
      // 요청이 있는 경우
      if (bRequest) {
        switch (stRequest.eCode)
        {
          // Open Call 처리
        case TCP_OPEN:
          // TCB 초기화
          kTCP_InitTCB(pstTCB);

          if (stRequest.eFlag == TCP_PASSIVE) {
            pstTCB->eState = TCP_LISTEN;
          }
          else if (stRequest.eFlag == TCP_ACTIVE) {
            // SYN Segment 송신
            stHeader.dwSequenceNumber = htons(pstTCB->dwISS);
            stHeader.wDataOffsetAndFlags =
              htons((5 << TCP_FLAGS_DATAOFFSET_SHIFT) | (1 << TCP_FLAGS_SYN_SHIFT));
            stHeader.wWindow = htons(pstTCB->dwRecvWND);

            kTCP_SendSegment(&stHeader, 20, NULL, 0, dwDestAddress);

            pstTCB->eState = TCP_SYN_SENT;
          }
          break;
          // 나머지 : Error
        default:
          break;
        }
      }

      break;  /* TCP_CLOSED */

    case TCP_LISTEN:
      break;  /* TCP_LISTEN */

    case TCP_SYN_SENT:
      break;  /* TCP_SYN_SENT */

    default:
      break;
    }
  }
}

BOOL kTCP_SendSegment(TCP_HEADER* pstHeader, WORD wHeaderSize, BYTE* pbPayload, WORD wPayloadLen, DWORD dwDestAddress)
{
  FRAME stFrame;
  IPv4Pseudo_Header stPseudoHeader = { 0, };

  if (kAllocateFrame(&stFrame) == FALSE)
    return FALSE;
  
  // 체크섬 계산
  kNumberToAddressArray(stPseudoHeader.vbSourceIPAddress, kIP_GetIPAddress(NULL), 4);
  kNumberToAddressArray(stPseudoHeader.vbDestinationIPAddress, dwDestAddress, 4);
  stPseudoHeader.bProtocol = IP_PROTOCOL_TCP;
  stPseudoHeader.wLength = htons(wHeaderSize + wPayloadLen);

  pstHeader->wChecksum = htons(kTCP_CalcChecksum(&stPseudoHeader, pstHeader, pbPayload, wPayloadLen));

  kEncapuslationSegment(&stFrame, pstHeader, wHeaderSize, pbPayload, wPayloadLen);

  stFrame.bType = FRAME_TCP;
  stFrame.eDirection = FRAME_DOWN;
  stFrame.qwDestAddress = ((0xFFFFFFFFUL << 32) | dwDestAddress);

  kTCP_PutFrameToFrameQueue(&stFrame);
}

WORD kTCP_CalcChecksum(IPv4Pseudo_Header* pstPseudo_Header, TCP_HEADER* pstHeader, BYTE* pbPayload, WORD wPayloadLen)
{
  int i;
  DWORD dwSum = 0;
  DWORD dwLen = 0;
  WORD* pwP;

  // 체크섬 초기화
  pstHeader->wChecksum = 0;

  // IPv4 Pseduo Header
  pwP = pstPseudo_Header;
  for (int i = 0; i < sizeof(IPv4Pseudo_Header) / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  // TCP Header
  pwP = pstHeader;
  dwLen = ntohs((pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_DATAOFFSET_SHIFT);
  for (int i = 0; i < 20 / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  // Option
  pwP = pstHeader->pbOption;
  if (dwLen > 20) {
    for (int i = 0; i < (dwLen - 20) / 2; i++)
    {
      dwSum += ntohs(pwP[i]);
    }
  }

  // Payload
  pwP = pbPayload;
  for (int i = 0; i < wPayloadLen / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  // 마지막 한바이트 남은 경우
  if (((wPayloadLen / 2) * 2) != wPayloadLen) {
    dwSum += ntohs((pwP[wPayloadLen / 2]) & 0xFF00);
  }

  if (dwSum > 0xFFFF) {
    dwSum = ((dwSum >> 16) + (dwSum & 0xFFFF));
  }

  return ~(dwSum & 0xFFFF) & 0xFFFF;
}

BOOL kTCP_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_UP;
  if (kTCP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kTCP_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stTCPManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stTCPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stTCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kTCP_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stTCPManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stTCPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stTCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

void kEncapuslationSegment(FRAME* pstFrame, BYTE* pbHeader, DWORD dwHeaderSize,
  BYTE* pbPayload, DWORD dwPayloadSize)
{
   pstFrame->wLen += dwPayloadSize + dwHeaderSize;

  if (dwPayloadSize != 0) {
    pstFrame->pbCur = pstFrame->pbCur - dwPayloadSize;
    kMemCpy(pstFrame->pbCur, pbPayload, dwPayloadSize);
  }

  pstFrame->pbCur = pstFrame->pbCur - dwHeaderSize;
  kMemCpy(pstFrame->pbCur, pbHeader, dwHeaderSize);

  // 옵션이 존재하는 경우 복사
  if (dwHeaderSize > sizeof(TCP_HEADER)) {
    kMemCpy(pstFrame->pbCur + sizeof(TCP_HEADER), 
      ((TCP_HEADER*)pbHeader)->pbOption, 
      dwHeaderSize - sizeof(TCP_HEADER));
  }
}

void kDecapuslationSegment(FRAME* pstFrame, BYTE** ppbHeader, BYTE** ppbPayload)
{
  TCP_HEADER* pstHeader;
  DWORD dwHeaderSize;

  *ppbHeader = pstFrame->pbCur;
  pstHeader = pstFrame->pbCur;
  dwHeaderSize = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_DATAOFFSET_SHIFT) * 8;

  // 옵션있는 경우
  if (dwHeaderSize > sizeof(TCP_HEADER))
    ((TCP_HEADER*)*ppbHeader)->pbOption = pstFrame->pbCur + sizeof(TCP_HEADER);
  else 
    ((TCP_HEADER*)*ppbHeader)->pbOption = NULL;

  pstFrame->wLen -= dwHeaderSize;
  pstFrame->pbCur += dwHeaderSize;

  if (ppbPayload != NULL)
    *ppbPayload = pstFrame->pbCur;
}