#include "TCP.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"

static TCPMANAGER gs_stTCPManager = { 0, };

static BYTE* pbStateString[] = {
  "TCP_CLOSED",
  "TCP_LISTEN",
  "TCP_SYN_SENT",
  "TCP_SYN_RECEIVED",
  "TCP_ESTABLISHED",
  "TCP_FIN_WAIT_1",
  "TCP_FIN_WAIT_2",
  "TCP_CLOSE_WAIT",
  "TCP_CLOSING",
  "TCP_LAST_ACK",
  "TCP_TIME_WAIT",
  "TCP_UNKNOWN",
};

long kTCP_Send(TCP_TCB* pstTCB, BYTE* pbBuf, WORD wLen, BYTE bFlag)
{
  volatile long qwRet = 0;
  TCP_REQUEST stRequest;
  stRequest.eCode = TCP_SEND;
  stRequest.eFlag = bFlag;
  stRequest.pbBuf = pbBuf;
  stRequest.wLen = wLen;
  stRequest.pqwRet = &qwRet;

  kTCP_PutRequestToTCB(pstTCB, &stRequest);

  while (qwRet == 0);
  return qwRet;
}

long kTCP_Recv(TCP_TCB* pstTCB, BYTE* pbBuf, WORD wLen, BYTE bFlag)
{
  volatile long qwRet = 0xFFFFFFFF;
  TCP_REQUEST stRequest;
  stRequest.eCode = TCP_RECEIVE;
  stRequest.eFlag = bFlag;
  stRequest.pbBuf = pbBuf;
  stRequest.wLen = wLen;
  stRequest.pqwRet = &qwRet;

  kTCP_PutRequestToTCB(pstTCB, &stRequest);

  while (qwRet == 0xFFFFFFFF);
  return qwRet;
}

BYTE kTCP_Close(TCP_TCB* pstTCB)
{
  volatile long qwRet = 0;
  TCP_REQUEST stRequest;
  stRequest.eCode = TCP_CLOSE;
  stRequest.pqwRet = &qwRet;

  kTCP_PutRequestToTCB(pstTCB, &stRequest);

  while (qwRet == 0);
  return qwRet;
}

TCP_TCB* kTCP_Open(WORD wLocalPort, QWORD qwForeignSocket, BYTE bFlag)
{
  return kTCP_CreateTCB(wLocalPort, qwForeignSocket, bFlag);
}

BYTE kTCP_Status(TCP_TCB* pstTCB)
{
  return kTCP_GetCurrentState(pstTCB);
}

void kTCP_ReturnRequest(TCP_REQUEST* pstRequest, long qwReturnValue)
{
  if (pstRequest->pqwRet != NULL)
    *(pstRequest->pqwRet) = qwReturnValue;
}

void kTCP_ProcessOption(TCP_TCB* pstTCB, TCP_HEADER* pstHeader)
{
  DWORD dwOptionLen;
  BYTE* pbOption;
  BYTE bKind, bLength, *pbValue;
  dwOptionLen = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_DATAOFFSET_SHIFT) * 4 - 20;
  if (dwOptionLen > 0) {
    pbOption = (BYTE*)pstHeader + 20;
    while (dwOptionLen > 0) {
      bKind = pbOption[0];
      bLength = pbOption[1];
      pbValue = pbOption + bLength - 2;

      switch (bKind)
      {
      case TCP_OPTION_EOL:
        return;

      case TCP_OPTION_NO:
        pbOption++;
        dwOptionLen--;
        break;

      case TCP_OPTION_MSS:
        pstTCB->dwMSS = ntohs(*((WORD*)pbValue));
        pbOption += bLength;
        dwOptionLen -= bLength;
        break;
        
      default:
        pbOption += bLength;
        dwOptionLen -= bLength;
        break;
      }
    }
  }
}

BOOL kTCP_ProcessSegment(TCP_TCB* pstTCB, TCP_HEADER* pstHeader, BYTE* pbPayload, WORD wPayloadLen)
{
  int i, j, k, l;
  BYTE vbTemp[TCP_MAXIMUM_SEGMENT_SIZE];
  BYTE vbOption[128] = { 0x02, 0x04, 0x05, 0x64 };
  TCP_HEADER stHeader = { 0, };
  BOOL bACK, bPSH, bRST, bSYN, bFIN;
  DWORD dwHeaderLen, dwSegmentSEQ, dwSegmentACK, wSegmentWindow;
  DWORD dwDestAddress;

  // 기본 헤더 생성
  stHeader.wSourcePort = htons(pstTCB->stLink.qwID & 0xFFFF);
  stHeader.wDestinationPort = htons((pstTCB->qwTempSocket >> 16) & 0xFFFF);
  stHeader.wWindow = htons(TCP_WINDOW_DEFAULT_SIZE);
  stHeader.pbOption = vbOption;
  dwDestAddress = (pstTCB->qwTempSocket >> 32);

  bACK = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_ACK_SHIFT) & 0x01;
  bPSH = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_PSH_SHIFT) & 0x01;
  bRST = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_RST_SHIFT) & 0x01;
  bSYN = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_SYN_SHIFT) & 0x01;
  bFIN = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_FIN_SHIFT) & 0x01;

  dwSegmentSEQ = ntohl(pstHeader->dwSequenceNumber);
  dwSegmentACK = ntohl(pstHeader->dwAcknowledgmentNumber);
  wSegmentWindow = ntohs(pstHeader->wWindow);

  //kPrintf("TCP | SEQ : %p, ACK : %p, SEG.LEN : %d | A[%d] P[%d] R[%d] S[%d] F[%d]\n", dwSegmentSEQ, dwSegmentACK, wPayloadLen, bACK, bPSH, bRST, bSYN, bFIN);
  //kPrintf("TCP | RCV.NXT : %p, \n", pstTCB->dwRecvNXT);

  switch (kTCP_GetCurrentState(pstTCB))
  {
  case TCP_CLOSED:
    // 수신 세크먼트가 FIN을 포함하는 경우 무시
    if (bFIN == TRUE)
      break;

    // RST가 꺼져 있는 경우 송신측으로 RST 세그먼트 전송
    if (bRST == FALSE) {
      bRST = TRUE;

      // ACK가 꺼져 있는 경우 ACK로 송신
      if (bACK == FALSE) {
        bACK = TRUE;
        dwSegmentACK = dwSegmentSEQ + wPayloadLen;
        dwSegmentSEQ = 0;
      }
      else {
        bACK = FALSE;
        dwSegmentSEQ = dwSegmentACK;
        dwSegmentACK = 0;
      }
      // RST Segment 송신
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
    }

    break; /* TCP_CLOSED */

  case TCP_LISTEN:
    // FIN이나 RST을 포함하는 경우 : 무시
    if ((bFIN == TRUE) || (bRST == TRUE))
      break;

    // ACK을 포함하는 경우 : RST Segment 송신
    if (bACK == TRUE) {
      // RST Segment 송신
      bACK = FALSE;
      bRST = TRUE;
      dwSegmentSEQ = dwSegmentACK;
      dwSegmentACK = 0;

      // RST Segment 송신
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // SYN을 포함하는 경우 : SYN 세그먼트 전송, SYN_RECEIVED 상태로 전이
    if (bSYN == TRUE) {
      // RCV.NXT=SEG.SEQ+1, IRS=SEG.SEQ로 설정
      pstTCB->dwRecvNXT = dwSegmentSEQ + 1;
      pstTCB->dwIRS = dwSegmentSEQ;

      // 송신 윈도우 초기화
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->pbSendWindow = (BYTE*)kAllocateMemory(pstTCB->dwSendWND);
      if (pstTCB->pbSendWindow == NULL) {
        kPrintf("kTCP_Machine | Send Window Allocate Fail\n");
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      kInitializeQueue(&(pstTCB->stSendQueue), pstTCB->pbSendWindow, pstTCB->dwSendWND, sizeof(BYTE));

      // --- CRITCAL SECTION BEGIN ---
      kLock(&(gs_stTCPManager.stLock));

      // ISS 결정
      pstTCB->dwISS = kTCP_GetISS();

      // 소켓 업데이트
      pstTCB->stLink.qwID = pstTCB->qwTempSocket;

      kUnlock(&(gs_stTCPManager.stLock));
      // --- CRITCAL SECTION END ---

      // SYN Segment 송신 (SEQ : ISS, CTL : SYN, ACK)
      bSYN = bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwISS;
      dwSegmentACK = pstTCB->dwRecvNXT;

      stHeader.pbOption = vbOption;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 6, bACK, 0, 0, bSYN, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 24, NULL, 0, dwDestAddress);

      // SND.UNA=ISS, SND.NXT=ISS+1로 설정
      pstTCB->dwSendUNA = pstTCB->dwISS;
      pstTCB->dwSendNXT = pstTCB->dwISS + 1;

      // SYN_RECEIVED 상태로 전이 
      kTCP_StateTransition(pstTCB, TCP_SYN_RECEIVED);
      break;
    }

    break; /* TCP_LISTEN */

  case TCP_SYN_SENT:
    // 수신 세크먼트가 FIN을 포함하는 경우 무시
    if (bFIN == TRUE)
      break;

    // ACK 포함하는 경우 ACK 번호 확인
    if (bACK == TRUE) {
      // ACK가 범위를 벗어나는 경우 (SEG.ACK <= ISS, SND.NXT < SEG.ACK)
      // RST 세그먼트 전송
      if ((dwSegmentACK <= pstTCB->dwISS) || (dwSegmentACK > pstTCB->dwSendNXT)) {
        // RST Segment 송신
        bACK = FALSE;
        bRST = TRUE;
        dwSegmentSEQ = dwSegmentACK;
        dwSegmentACK = 0;

        // RST Segment 송신
        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
        break;
      }
    }

    // ACK가 없거나 정상 인 경우
    // RST Segment 수신 시
    if (bRST == TRUE) {
      if (bACK == TRUE) { // 정상 ACK : CLOSED 상태로 전이
        // "error: connection reset"
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      // No ACK 인 경우 Segment 버림
      break;
    }

    // SYN 확인
    if (bSYN == TRUE) {
      // 옵션 처리 (MSS 가져옴)
      kTCP_ProcessOption(pstTCB, pstHeader);

      // RCV.NXT=SEG.SEQ+1, IRS=SEG.SEQ로 설정
      pstTCB->dwRecvNXT = dwSegmentSEQ + 1;
      pstTCB->dwIRS = dwSegmentSEQ;
      pstTCB->dwSendWL1 = pstTCB->dwIRS;
      pstTCB->dwSendWL2 = dwSegmentACK;

      // 송신 윈도우 초기화
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->pbSendWindow = (BYTE*)kAllocateMemory(pstTCB->dwSendWND);
      if (pstTCB->pbSendWindow == NULL) {
        kPrintf("kTCP_Machine | Send Window Allocate Fail\n");
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      kInitializeQueue(&(pstTCB->stSendQueue), pstTCB->pbSendWindow, pstTCB->dwSendWND, sizeof(BYTE));

      // ACK 확인
      if (bACK == TRUE) {
        // SND.UNA 갱신
        pstTCB->dwSendUNA = dwSegmentACK;
        // 삭제 가능 재전송 프레임 삭제
        kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
      }

      // SND.UNA > ISS (전송한 SYN Segment가 응답을 받은 경우)
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 전송, ESTABLISHED 전이
      if (pstTCB->dwSendUNA > pstTCB->dwISS) {
        bACK = TRUE;
        dwSegmentSEQ = pstTCB->dwSendNXT;
        dwSegmentACK = pstTCB->dwRecvNXT;

        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

        // ESTABLISHED 상태로 전이
        kTCP_StateTransition(pstTCB, TCP_ESTABLISHED);
        break;
      }

      // 전송한 SYN Segment 응답을 받지 못하고, 상대방의 SYN Segment 만을 받은 경우
      // <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK> 전송
      bSYN = bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwISS;
      dwSegmentACK = pstTCB->dwRecvNXT;

      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, bSYN, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 수신 Data 처리 : 수신 윈도우가 여유있는 만큼 수신
      j = MIN(wPayloadLen, pstTCB->dwRecvWND);
      for (i = 0; i < j; i++) {
        kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
      }
      // 수신 윈도우 크기 조정
      pstTCB->dwRecvWND = pstTCB->dwRecvWND - j;

      // SYN_RECEIVED 상태로 전이
      kTCP_StateTransition(pstTCB, TCP_SYN_RECEIVED);
    }
    // SYN, RST 모두 아닌 Segment : 버림
    break; /* TCP_SYN_SENT */

  case TCP_SYN_RECEIVED:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우
    if (bRST == TRUE) {
      // Passive Open을 통해 시작된 경우 : Listen State로 전이
      if (kTCP_GetPreviousState(pstTCB) == TCP_LISTEN) {
        kTCP_StateTransition(pstTCB, TCP_LISTEN);
      }
      // Active Open으로 통해 시작된 경우 : Closed 상태로 전이 후 종료
      else if (kTCP_GetPreviousState(pstTCB) == TCP_SYN_SENT) {
        // 모든 요청, 전송 자원 초기화
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      break;
    }

    // SYN Segment 인 경우
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT (SYN에 대한 ACK 수신) : ESTABLISHED 상태로 전이
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      // SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
      kTCP_StateTransition(pstTCB, TCP_ESTABLISHED);
    }
    // 잘못된 응답 : RST Segment 송신
    else {
      bRST = TRUE;
      dwSegmentSEQ = dwSegmentACK;
      dwSegmentACK = 0;
      // RST Segment 송신
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
    }

    // FIN Bit가 켜져 있는 경우
    if (bFIN == TRUE) {
      // 모든 Receive 요청 : "connection closing"
      // RCV.NXT = FIN, ACK 송신
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 수신 Segment를 PUSH 처리
      // CLOSE-WAIT 상태로 전이
      kTCP_StateTransition(pstTCB, TCP_CLOSE_WAIT);
      break;
    }
    break; /* TCP_SYN_RECEIVED */

  case TCP_ESTABLISHED:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우
    if (bRST == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (아직 전송하지 않은 SEQ에 대한 ACK)
    else if (pstTCB->dwSendNXT < dwSegmentACK) {
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }
    // SEG.ACK < SND.UNA (중복 ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window 갱신
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) 인 경우
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // 송신 윈도우 갱신
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    // kPrintf("Segment Arrive : %p\n", dwSegmentSEQ - pstTCB->dwIRS);

    // Segment Data가 존재하는 경우 수신 처리
    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : 개선
        // 수신 Data 처리 : 수신 윈도우가 여유있는 만큼 수신
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // 수신 윈도우 크기 조정
        pstTCB->dwRecvWND = pstTCB->dwRecvWND - j;
        pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + j;

        // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
        bACK = TRUE;
        dwSegmentSEQ = pstTCB->dwSendNXT;
        dwSegmentACK = pstTCB->dwRecvNXT;
        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      }
      else {
        return FALSE;
      }
    }
    // FIN Bit가 켜져 있는 경우
    if (bFIN == TRUE) {
      // 모든 Receive 요청 : "connection closing"
      // RCV.NXT = FIN, ACK 송신
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 수신 Segment를 PUSH 처리
      // CLOSE-WAIT 상태로 전이
      kTCP_StateTransition(pstTCB, TCP_CLOSE_WAIT);
    }

    break; /* TCP_ESTABLISHED */

  case TCP_FIN_WAIT_1:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우 : TCP_CLOSED
    if (bRST == TRUE) {
      // 모든 요청에 "reset" 응답
      // 전송 자원 초기화
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우 : TCP_CLOSED
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (아직 전송하지 않은 SEQ에 대한 ACK)
    else if (pstTCB->dwSendNXT < dwSegmentACK) {
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }
    // SEG.ACK < SND.UNA (중복 ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window 갱신
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) 인 경우
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // 송신 윈도우 갱신
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    // 송신한 FIN에 대한 응답 확인 : FIN_WAIT_2 상태로 전이 후 세그먼트 재처리
    // (마지막 Segment 까지 응답 받은 경우)
    if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
      kTCP_StateTransition(pstTCB, TCP_FIN_WAIT_2);
      kTCP_ProcessSegment(pstTCB, pstHeader, pbPayload, wPayloadLen);
      break;
    }

    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : 개선
        // 수신 Data 처리 : 수신 윈도우가 여유있는 만큼 수신
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // 수신 윈도우 크기 조정
        pstTCB->dwRecvWND = pstTCB->dwRecvWND - j;
        pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + j;

        // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
        bACK = TRUE;
        dwSegmentSEQ = pstTCB->dwSendNXT;
        dwSegmentACK = pstTCB->dwRecvNXT;
        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      }
      else {
        return FALSE;
      }
    }

    // FIN Bit가 켜져 있는 경우
    if (bFIN == TRUE) {
      // 모든 Receive 요청 : "connection closing"
      // RCV.NXT = FIN, ACK 송신
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 수신 Segment를 PUSH 처리
      // 송신한 FIN에 대한 응답 확인 : TIME_WAIT 상태로 전이 (타이머 시작)
      // (마지막 Segment 까지 응답 받은 경우)
      if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
        pstTCB->qwTimeWaitTime = kGetTickCount() + (TCP_MSL_DEFAULT_SECOND * 1000);
        kTCP_StateTransition(pstTCB, TCP_TIME_WAIT);
        break;
      }
      // 송신한 FIN에 대한 응답 미확인 : CLOSING 상태로 전이
      else {
        kTCP_StateTransition(pstTCB, TCP_CLOSING);
        break;
      }
    }
    break; /* TCP_FIN_WAIT_1 */

  case TCP_FIN_WAIT_2:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우 : TCP_CLOSED
    if (bRST == TRUE) {
      // 모든 요청에 "reset" 응답
      // 전송 자원 초기화
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우 : TCP_CLOSED
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (아직 전송하지 않은 SEQ에 대한 ACK)
    else if (pstTCB->dwSendNXT < dwSegmentACK) {
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }
    // SEG.ACK < SND.UNA (중복 ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window 갱신
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) 인 경우
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // 송신 윈도우 갱신
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : 개선
        // 수신 Data 처리 : 수신 윈도우가 여유있는 만큼 수신
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // 수신 윈도우 크기 조정
        pstTCB->dwRecvWND = pstTCB->dwRecvWND - j;
        pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + j;

        // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
        bACK = TRUE;
        dwSegmentSEQ = pstTCB->dwSendNXT;
        dwSegmentACK = pstTCB->dwRecvNXT;
        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      }
      else {
        return FALSE;
      }
    }

    // FIN Bit가 켜져 있는 경우 : TIME_WAIT 상태로 전이
    if (bFIN == TRUE) {
      // 모든 Receive 요청 : "connection closing"
      // RCV.NXT = FIN, ACK 송신
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 수신 Segment를 PUSH 처리

      // 타이머 시작 후 TIME_WAIT 상태로 전이
      pstTCB->qwTimeWaitTime = kGetTickCount() + (TCP_MSL_DEFAULT_SECOND * 1000);
      kTCP_StateTransition(pstTCB, TCP_TIME_WAIT);
    }
    break; /* TCP_FIN_WAIT_2 */

  case TCP_CLOSE_WAIT:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우 : TCP_CLOSED
    if (bRST == TRUE) {
      // 모든 요청에 "reset" 응답
      // 전송 자원 초기화
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우 : TCP_CLOSED
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (아직 전송하지 않은 SEQ에 대한 ACK)
    else if (pstTCB->dwSendNXT < dwSegmentACK) {
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }
    // SEG.ACK < SND.UNA (중복 ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window 갱신
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) 인 경우
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // 송신 윈도우 갱신
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : 개선
        // 수신 Data 처리 : 수신 윈도우가 여유있는 만큼 수신
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // 수신 윈도우 크기 조정
        pstTCB->dwRecvWND = pstTCB->dwRecvWND - j;
        pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + j;

        // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
        bACK = TRUE;
        dwSegmentSEQ = pstTCB->dwSendNXT;
        dwSegmentACK = pstTCB->dwRecvNXT;
        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      }
      else {
        return FALSE;
      }
    }
    // FIN Bit가 켜져 있는 경우 : PASS
    if (bFIN == TRUE) {
      break;
    }

    break; /* TCP_CLOSE_WAIT */

  case TCP_CLOSING:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우 : TCP_CLOSED
    if (bRST == TRUE) {
      // 모든 요청에 "reset" 응답
      // 전송 자원 초기화
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우 : TCP_CLOSED
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (아직 전송하지 않은 SEQ에 대한 ACK)
    else if (pstTCB->dwSendNXT < dwSegmentACK) {
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }
    // SEG.ACK < SND.UNA (중복 ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window 갱신
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) 인 경우
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // 송신 윈도우 갱신
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    // 송신한 FIN에 대한 응답 확인 : TIME_WAIT 상태로 전이 후 세그먼트 재처리
    // (마지막 Segment 까지 응답 받은 경우)
    if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
      // 타이머 시작 후 TIME_WAIT 상태로 전이
      pstTCB->qwTimeWaitTime = kGetTickCount() + (TCP_MSL_DEFAULT_SECOND * 1000);
      kTCP_StateTransition(pstTCB, TCP_TIME_WAIT);
      break;
    }

    // FIN Bit가 켜져 있는 경우 : PASS
    if (bFIN == TRUE) {
      break;
    }
    break; /* TCP_CLOSING */

  case TCP_LAST_ACK:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우 : TCP_CLOSED
    if (bRST == TRUE) {
      // 모든 요청에 "reset" 응답
      // 전송 자원 초기화
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우 : TCP_CLOSED
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // ACK 처리
    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK 로 설정, 재전송 큐 갱신
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }

    // FIN에 대한 ACK를 수신한 경우 : TCP_CLOSED
    // (마지막 Segment 까지 응답 받은 경우)
    if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // FIN Bit가 켜져 있는 경우 : PASS
    if (bFIN == TRUE) {
      break;
    }

    break; /* TCP_LAST_ACK */

  case TCP_TIME_WAIT:
    // 수신 Segment 순서번호 확인
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // 수신 Segment가 올바르지 않은 경우 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment 인 경우 : TCP_CLOSED
    if (bRST == TRUE) {
      // 모든 요청에 "reset" 응답
      // 전송 자원 초기화
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment 인 경우 : TCP_CLOSED
    if (bSYN == TRUE) {
      // 모든 요청, 전송 자원 초기화
      // Closed 상태로 전이 후 종료
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit가 꺼져 있는 경우 : Segment 버림
    if (bACK == FALSE) {
      break;
    }

    // FIN Bit가 켜져 있는 경우 : ACK 전송, 2 MSL Timeout 재 시작
    if (bFIN == TRUE) {
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 2 MSL Timeout 재 시작
      pstTCB->qwTimeWaitTime = kGetTickCount() + (2 * TCP_MSL_DEFAULT_SECOND * 1000);
      break;
    }
    break; /* TCP_TIME_WAIT */

  default:
    break;
  } /* switch (pstTCB->eState) */

  return TRUE;
}

void kTCP_Task(void)
{
  TCP_TCB* pstTCB = NULL;
  TCP_HEADER* pstHeader;
  BYTE* pbIPPayload;
  QWORD qwSocketPair;
  DWORD dwForeignSocket;
  DWORD dwLocalSocket;

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
      // kPrintf("TCP | Receive TCP Segment %d\n", i++);

      // 세그먼트 헤더 분리
      pstHeader = (TCP_HEADER*)stFrame.pbCur;

      // 소켓 정보 생성 (Destination Address, Port, Local Port);
      // IP Header의 근원지 주소, TCP Header의 근원지 포트, 목적지 포트
      dwForeignSocket = ntohs(pstHeader->wSourcePort);
      dwLocalSocket = ntohs(pstHeader->wDestinationPort);
      qwSocketPair = ((stFrame.qwDestAddress & 0xFFFFFFFF00000000) | (dwForeignSocket << 16) | dwLocalSocket);

      // --- CRITCAL SECTION BEGIN ---
      kLock(&(gs_stTCPManager.stLock));

      // 연결된 소켓 검색
      pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), qwSocketPair);
      // 연결된 소켓이 없는 경우
      if (pstTCB == NULL) {
        // 열린 소켓 검색
        pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), dwLocalSocket);
      }

      // 연결된 소켓, 열린 소켓 모두 없는 경우
      if (pstTCB == NULL) {
        kFreeFrame(&stFrame);
      }
      // 포트 번호에 따라 멀티 플렉싱
      else {
        kTCP_PutFrameToTCB(pstTCB, &stFrame);
      }

      kUnlock(&(gs_stTCPManager.stLock));
      // --- CRITCAL SECTION END ---

      break;  /* End of case FRAME_UP: */

    case FRAME_DOWN:
      // kPrintf("TCP | Send TCP Segment \n");

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

  // 밀리 세컨드 차이
  qwDiff = qwNow - gs_stTCPManager.qwISSTime;

  // 4 마이크로 세컨드마다 1 증가
  dwISS = (gs_stTCPManager.dwISS + (qwDiff * 250)) % 4294967296;
  gs_stTCPManager.dwISS = dwISS;

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

BYTE kTCP_GetRequestFromTCB(const TCP_TCB* pstTCB, TCP_REQUEST* pstRequest, BOOL bPop)
{
  BYTE bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  if (bPop == TRUE)
    bResult = kGetQueue(&(pstTCB->stRequestQueue), pstRequest);
  else 
    bResult = kFrontQueue(&(pstTCB->stRequestQueue), pstRequest);

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
  pstTCB->pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (pstTCB->pstFrameBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | FrameBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    kFreeMemory(pstTCB->pstRequestBuffer);
    return NULL;
  }
  // 프레임 큐 초기화
  kInitializeQueue(&(pstTCB->stFrameQueue), pstTCB->pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 재전송 프레임 큐 할당
  pstTCB->pstRetransmitFrameBuffer = (RE_FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(RE_FRAME));
  if (pstTCB->pstRetransmitFrameBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | RetransmitFrameBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    kFreeMemory(pstTCB->pstRequestBuffer);
    kFreeMemory(pstTCB->pstFrameBuffer);
    return NULL;
  }
  // 재전송 프레임 큐 초기화
  kInitializeQueue(&(pstTCB->stRetransmitFrameQueue), pstTCB->pstRetransmitFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(RE_FRAME));

  //  수신 윈도우 할당
  pstTCB->dwRecvWND = TCP_WINDOW_DEFAULT_SIZE;
  pstTCB->pbRecvWindow = (BYTE*)kAllocateMemory(pstTCB->dwRecvWND);
  if (pstTCB->pbRecvWindow == NULL) {
    kPrintf("kTCP_CreateTCB | RecvWindow Allocate Fail\n");
    kFreeMemory(pstTCB);
    kFreeMemory(pstTCB->pstRequestBuffer);
    kFreeMemory(pstTCB->pstFrameBuffer);
    kFreeMemory(pstTCB->pstRetransmitFrameBuffer);
    return NULL;
  }
  kInitializeQueue(&(pstTCB->stRecvQueue), pstTCB->pbRecvWindow, pstTCB->dwRecvWND, sizeof(BYTE));

  // 뮤텍스 초기화
  kInitializeMutex(&(pstTCB->stLock));

  // TCB 정보 초기화
  pstTCB->stLink.qwID = qwSocketPair;
  pstTCB->eState = TCP_CLOSED;
  pstTCB->dwISS = kTCP_GetISS();  // 초기 순서번호
  pstTCB->dwSendWL1 = pstTCB->dwSendWL2 = pstTCB->dwSendUNA = pstTCB->dwSendNXT = pstTCB->dwISS;
  pstTCB->pbSendWindow = NULL;

  // TCB 리스트에 추가
  gs_stTCPManager.pstCurrentTCB = pstTCB;
  kAddListToHeader(&(gs_stTCPManager.stTCBList), pstTCB);

  // Request 삽입
  stRequest.eCode = TCP_OPEN;
  stRequest.eFlag = eFlag;
  stRequest.pqwRet = NULL;
  kTCP_PutRequestToTCB(pstTCB, &stRequest);
  
  // TCB 담당 상태 머신 생성
  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_HIGHEST, 0, 0,
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

BYTE kTCP_DeleteTCB(TCP_TCB* pstTCB)
{
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stTCPManager.stLock));

  // 송신 윈도우 해제
  if (pstTCB->pbSendWindow != NULL) {
    kFreeMemory(pstTCB->pbSendWindow);
  }
  // 수신 윈도우 해제
  if (pstTCB->pbRecvWindow != NULL) {
    kFreeMemory(pstTCB->pbRecvWindow);
  }
  // 요청 처리 큐 해제
  kFreeMemory(pstTCB->pstRequestBuffer);
  // 프레임 큐 해제
  kFreeMemory(pstTCB->pstFrameBuffer);
  // 재전송 큐 해제
  kFreeMemory(pstTCB->pstRetransmitFrameBuffer);

  // 소켓 반납
  kRemoveList(&(gs_stTCPManager.stTCBList), pstTCB->stLink.qwID);

  kFreeMemory(pstTCB);

  kUnlock(&(gs_stTCPManager.stLock));
  // --- CRITCAL SECTION END ---

  // kPrintf("Delete TCB !!!!\n");
}

BYTE kTCP_PutRetransmitFrameToTCB(const TCP_TCB* pstTCB, RE_FRAME* pstFrame)
{
  BYTE bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(pstTCB->stLock));

  bResult = kPutQueue(&(pstTCB->stRetransmitFrameQueue), pstFrame);

  kUnlock(&(pstTCB->stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

void kTCP_ProcessRetransmitQueue(const TCP_TCB* pstTCB)
{
  BOOL bResult;
  RE_FRAME stReFrame, stReReFrame;
  QWORD qwTime = kGetTickCount();

  while (1) {
    // 재전송 큐 맨 앞 프레임 확인
    bResult = kFrontQueue(&(pstTCB->stRetransmitFrameQueue), &stReFrame);
    if (bResult == FALSE)
      return;

    // 재전송 시간 초과한 경우 : 큐에서 인출
    if (stReFrame.qwTime <= qwTime) {
      bResult = kGetQueue(&(pstTCB->stRetransmitFrameQueue), &stReFrame);
      // ACK를 받은 프레임이 아닌 경우 : 재전송
      // ACK 받은 경우 : PASS
      if (stReFrame.stFrame.bType != FRAME_PASS) {
        // 재전송 프레임의 재전송 프레임 생성
        kAllocateReFrame(&stReReFrame, &(stReFrame.stFrame));
        kTCP_PutRetransmitFrameToTCB(pstTCB, &stReReFrame);

        // 재전송 (TCP 메인 모듈로 전달)
        kTCP_PutFrameToFrameQueue(&(stReFrame.stFrame));
      }
    }
    // 재전송 시간 초과하지 않은 경우 : PASS
    else {
      return;
    }
  }
}

void kTCP_UpdateRetransmitQueue(const TCP_TCB* pstTCB, DWORD dwACK)
{
  int i;
  BOOL bResult;
  RE_FRAME stReFrame;
  TCP_HEADER* pstHeader;

  for (i = 0; i < kGetQueueSize(&(pstTCB->stRetransmitFrameQueue)); i++) {
    bResult = kGetQueue(&(pstTCB->stRetransmitFrameQueue), &stReFrame);

    // 헤더 확인
    pstHeader = (TCP_HEADER*)(stReFrame.stFrame.pbCur);

    // 순서번호를 확인하여 응답 완료된 세그먼트 : 큐에서 제거
    // 완료되지 않은 세그먼트 : 재전송 큐에 재삽입
    if (ntohl(pstHeader->dwSequenceNumber) > dwACK) {
      kTCP_PutRetransmitFrameToTCB(pstTCB, &stReFrame);
    }
  }
}

BOOL kTCP_IsNeedRetransmit(const TCP_HEADER* pstHeader)
{
  BOOL bSYN, bFIN;
  WORD wFlags = ntohs(pstHeader->wDataOffsetAndFlags);

  bSYN = (wFlags >> TCP_FLAGS_SYN_SHIFT) & 0x01;
  bFIN = (wFlags >> TCP_FLAGS_FIN_SHIFT) & 0x01;

  // ACK 만 설정되있는 경우 재전송 필요없음
  if ((bSYN == TRUE) || (bFIN == TRUE))
    return TRUE;
  return FALSE;
}

BOOL kTCP_IsDuplicateSegment(const TCP_TCB* pstTCB, DWORD dwSEGLEN, DWORD dwSEGSEQ)
{
  // 1. SEQ 번호를 확인하여 중복 Segment 제거
  // SEQ == 0
  if (dwSEGLEN == 0) {
    // RCV.WND == 0
    if (pstTCB->dwRecvWND == 0) {
      // Check SEG.SEQ == RCV.NXT
      if (dwSEGSEQ == pstTCB->dwRecvNXT)
        return TRUE;
    }
    // RCV.WND > 0
    else {
      // Check RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
      if ((pstTCB->dwRecvNXT <= dwSEGSEQ) &&
        (dwSEGSEQ < (pstTCB->dwRecvNXT + pstTCB->dwRecvWND)))
        return TRUE;
    }
  }
  // SEQ > 0
  else {
    // RCV.WND == 0
    if (pstTCB->dwRecvWND == 0) {
      // not acceptable
      return FALSE;
    }
    // RCV.WND > 0
    else {
      // Check RCV.NXT =< SEG.SEQ < RCV,NXT+RCV,WND
      // or RCV.NXT = < SEG.SEQ + SEG.LEN - 1 < RCV.NXT + RCV.WND
      if (
        ((pstTCB->dwRecvNXT <= dwSEGSEQ) && (dwSEGSEQ < (pstTCB->dwRecvNXT + pstTCB->dwRecvWND))) ||
        ((pstTCB->dwRecvNXT <= dwSEGSEQ + dwSEGLEN - 1) && ((dwSEGSEQ + dwSEGLEN - 1) < (pstTCB->dwRecvNXT + pstTCB->dwRecvWND)))
        )
        return TRUE;
    }
  }
  return FALSE;
}

TCP_STATE kTCP_GetPreviousState(const TCP_TCB* pstTCB)
{
  return (pstTCB->eState >> 16);
}

TCP_STATE kTCP_GetCurrentState(const TCP_TCB* pstTCB)
{
  return (pstTCB->eState & 0xFFFF);
}

void kTCP_StateTransition(TCP_TCB* pstTCB, TCP_STATE eToState)
{
  DWORD previousState = kTCP_GetPreviousState(pstTCB);
  pstTCB->eState = (previousState << 16) | eToState;
}

void kTCP_Machine(void)
{
  int i, j, k, l;
  DWORD dwDestAddress;
  QWORD qwTime;
  FRAME stFrame;
  TCP_REQUEST stRequest;

  TCP_HEADER stHeader = { 0, };
  TCP_HEADER* pstHeader;
  BYTE *pbPayload;
  BYTE vbTemp[TCP_MAXIMUM_SEGMENT_SIZE];
  BYTE vbOption[128] = { 0x02, 0x04, 0x05, 0x64 };

  BOOL bRequest, bFrame;
  BOOL bACK, bPSH, bRST, bSYN, bFIN;
  DWORD dwHeaderLen, dwSegmentLen, dwSegmentSEQ, dwSegmentACK, wSegmentWindow;
  // TCB 가져오기
  TCP_TCB* pstTCB = gs_stTCPManager.pstCurrentTCB;
  if (pstTCB == NULL)
    return;
  gs_stTCPManager.pstCurrentTCB = NULL;

  // 기본 헤더 생성
  stHeader.wSourcePort = htons(pstTCB->stLink.qwID & 0xFFFF);
  stHeader.wDestinationPort = htons((pstTCB->stLink.qwID >> 16) & 0xFFFF);
  stHeader.wWindow = htons(TCP_WINDOW_DEFAULT_SIZE);
  stHeader.pbOption = vbOption;
  dwDestAddress = (pstTCB->stLink.qwID >> 32);

  // kPrintf("kTCP_Machine\n");

  qwTime = kGetTickCount();
  while (1)
  {
    if ((kGetTickCount() - qwTime) >= 5000) {
      // kPrintf("TCP_Machine Socket : %q | State : %s\n", pstTCB->stLink.qwID, pbStateString[pstTCB->eState]);
      qwTime = kGetTickCount();
    }

    // TIME_WAIT 타이머 만료시 : CLOSE 상태로 전이
    if (kTCP_GetCurrentState(pstTCB) == TCP_TIME_WAIT) {
      if (kGetTickCount() >= pstTCB->qwTimeWaitTime) {
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
    }

    // 재전송 처리
    kTCP_ProcessRetransmitQueue(pstTCB);

    // 요청 처리
    bRequest = kTCP_GetRequestFromTCB(pstTCB, &stRequest, FALSE);
    // 요청이 있는 경우
    if (bRequest) {
      switch (kTCP_GetCurrentState(pstTCB))
      {
      case TCP_CLOSED:
        switch (stRequest.eCode)
        {
          // Open Call 처리
        case TCP_OPEN:
          if (stRequest.eFlag == TCP_PASSIVE) {
            // LISTEN 상태로 전이
            kTCP_StateTransition(pstTCB, TCP_LISTEN);
          }
          else if (stRequest.eFlag == TCP_ACTIVE) {
            // SYN Segment 송신 (SEQ : ISS, CTL : SYN)
            bSYN = TRUE;
            dwSegmentSEQ = pstTCB->dwISS;
            dwSegmentACK = 0;

            stHeader.pbOption = vbOption;
            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 6, 0, 0, 0, bSYN, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 24, NULL, 0, dwDestAddress);

            // SND.UNA=ISS, SND.NXT=ISS+1로 설정
            pstTCB->dwSendUNA = pstTCB->dwISS;
            pstTCB->dwSendNXT = pstTCB->dwISS + 1;

            // SYN_SENT 상태로 전이
            kTCP_StateTransition(pstTCB, TCP_SYN_SENT);
          }

          // TCB 반환
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, pstTCB);
          break;

          // 나머지 : Error
        default: /* return “error: connection does not exist” */
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_DOES_NOT_EXIST);
          break;
        }
        break;  /* TCP_CLOSED */

      case TCP_LISTEN:
        break;  /* TCP_LISTEN */

      case TCP_SYN_SENT:
        switch (stRequest.eCode)
        {
          // Open Call 처리
        case TCP_OPEN:  /* Return “error: connection already exists” */
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;

          // Send, Recv : 요청 처리 안함. 
        case TCP_SEND:
        case TCP_RECEIVE:
          break;
        }
        break;  /* TCP_SYN_SENT */

      case TCP_ESTABLISHED:
        switch (stRequest.eCode)
        {
        case TCP_SEND:
          // 윈도우 닫힌 경우 블락 : Keep Alive Segment 전송하여 윈도우 확인
          // TODO : 바보 윈도우 관련 
          if (pstTCB->dwSendWND == 0)
          {
            // Keep Alive (ACK) Segment 송신 (SEQ : SND.NXT - 1, ACK : RCV.NXT, CTL : ACK)
            bACK = TRUE;
            dwSegmentSEQ = pstTCB->dwSendNXT - 1;
            dwSegmentACK = pstTCB->dwRecvNXT;

            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
          }

          // 송신 윈도우에 삽입가능한 만큼 삽입
          for (i = 0; i < stRequest.wLen; i++) {
            if (kPutQueue(&(pstTCB->stSendQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // PUSH 플래그 설정된 경우 OR 
          // 임계치 만큼 데이터가 모인 경우 세그먼트로 만들어 전송
          // 세그먼트 크기는 MSS를 넘지 않음.
          dwSegmentLen = MIN(kGetQueueSize(&(pstTCB->stSendQueue)) - (pstTCB->dwSendNXT - pstTCB->dwSendUNA), pstTCB->dwMSS);
          if ((stRequest.eFlag == TCP_PUSH) || (dwSegmentLen >= TCP_MINIMUM_SEGMENT_SIZE)) {
            dwSegmentSEQ = pstTCB->dwSendNXT;
            dwSegmentACK = pstTCB->dwRecvNXT;

            // TODO : Deque 형태로 개선
            // 앞에 위치한 UNA 부분 큐에서 인출하여 다시 삽입 (맨뒤로 위치)
            k = kGetQueueSize(&(pstTCB->stSendQueue));
            for (j = 0; j < (pstTCB->dwSendNXT - pstTCB->dwSendUNA); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // 세그먼트로 만들 데이터 인출하여 임시버퍼에 저장 후 다시 삽입 (맨뒤로 위치)
            for (j = 0; j < dwSegmentLen; j++) {
              kGetQueue(&(pstTCB->stSendQueue), vbTemp + j);
              kPutQueue(&(pstTCB->stSendQueue), vbTemp + j);
            }

            // 세그먼트로 만들지 않은 데이터 인출 후 다시 삽입 (맨뒤로 위치)
            for (j = 0; j < (k - dwSegmentLen - (pstTCB->dwSendNXT - pstTCB->dwSendUNA)); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // SND.NXT 갱신
            pstTCB->dwSendNXT = pstTCB->dwSendNXT + dwSegmentLen;

            // 세그먼트 전송
            bPSH = bACK = TRUE;
            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, bPSH, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, vbTemp, dwSegmentLen, dwDestAddress);
          }

          // 송신 윈도우에 삽입한 만큼 반환
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, i);
          break; /* TCP_SEND */

        case TCP_RECEIVE:
          // 수신 가능한 만큼 유저 버퍼에 삽입
          for (i = 0; i < stRequest.wLen; i++) {
            if (kGetQueue(&(pstTCB->stRecvQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }
          // 1바이트 이상 유저 버퍼에 삽입 시 반환 OR NONBLOCK 요청인 경우
          if ((i > 0) || (stRequest.eFlag == TCP_NONBLOCK)) {
            // 수신 윈도우 크기 갱신
            pstTCB->dwRecvWND = pstTCB->dwRecvWND + i;

            // 유저 버퍼에 삽입한 만큼 반환
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, i);
          }
          break; /* TCP_RECEIVE */

        case TCP_CLOSE:
          // FIN Segment 송신
          bACK = bFIN = TRUE;
          dwSegmentSEQ = pstTCB->dwSendNXT;
          dwSegmentACK = pstTCB->dwRecvNXT;

          kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
          kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, bFIN);
          kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

          // SND.NXT= 1증가 설정
          pstTCB->dwSendNXT = pstTCB->dwSendNXT + 1;
          // TCP_FIN_WAIT_1 상태로 전이
          kTCP_StateTransition(pstTCB, TCP_FIN_WAIT_1);

          // OK 반환
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_OK);
          break; /* TCP_CLOSE */
        }

        break; /* TCP_ESTABLISHED */

      case TCP_FIN_WAIT_1:
      case TCP_FIN_WAIT_2:
        switch (stRequest.eCode)
        {
        case TCP_OPEN:
          //  Return “error: connection already exists”
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;
        case TCP_SEND:
        case TCP_CLOSE:
          //  return “error: connection closing”
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_CLOSING);
          break;

        case TCP_RECEIVE:
          // 수신 가능한 만큼 유저 버퍼에 삽입
          for (i = 0; i < stRequest.wLen; i++) {
            if (kGetQueue(&(pstTCB->stRecvQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // 1바이트 이상 유저 버퍼에 삽입 시 반환 OR NONBLOCK 요청인 경우
          if ((i > 0) || (stRequest.eFlag == TCP_NONBLOCK)) {
            // 수신 윈도우 크기 갱신
            pstTCB->dwRecvWND = pstTCB->dwRecvWND + i;

            // 유저 버퍼에 삽입한 만큼 반환
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, i);
          }

          break; /* TCP_RECEIVE */
        }

        break; /* TCP_FIN_WAIT_1 */

      case TCP_CLOSE_WAIT:
        switch (stRequest.eCode)
        {
        case TCP_OPEN:
          //  Return “error: connection already exists”
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;
        case TCP_SEND:
          // 윈도우 닫힌 경우 블락 : Keep Alive Segment 전송하여 윈도우 확인
          // TODO : 바보 윈도우 관련 
          if (pstTCB->dwSendWND == 0)
          {
            // Keep Alive (ACK) Segment 송신 (SEQ : SND.NXT - 1, ACK : RCV.NXT, CTL : ACK)
            bACK = TRUE;
            dwSegmentSEQ = pstTCB->dwSendNXT - 1;
            dwSegmentACK = pstTCB->dwRecvNXT;

            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
          }

          // 송신 윈도우에 삽입가능한 만큼 삽입
          for (i = 0; i < stRequest.wLen; i++) {
            if (kPutQueue(&(pstTCB->stSendQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // PUSH 플래그 설정된 경우 OR 
          // 임계치 만큼 데이터가 모인 경우 세그먼트로 만들어 전송
          // 세그먼트 크기는 MSS를 넘지 않음.
          dwSegmentLen = MIN(kGetQueueSize(&(pstTCB->stSendQueue)) - (pstTCB->dwSendNXT - pstTCB->dwSendUNA), pstTCB->dwMSS);
          if ((stRequest.eFlag == TCP_PUSH) || (dwSegmentLen >= TCP_MINIMUM_SEGMENT_SIZE)) {
            dwSegmentSEQ = pstTCB->dwSendNXT;
            dwSegmentACK = pstTCB->dwRecvNXT;


            // TODO : Deque 형태로 개선
            // 앞에 위치한 UNA 부분 큐에서 인출하여 다시 삽입 (맨뒤로 위치)
            k = kGetQueueSize(&(pstTCB->stSendQueue));
            for (j = 0; j < (pstTCB->dwSendNXT - pstTCB->dwSendUNA); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // 세그먼트로 만들 데이터 인출하여 임시버퍼에 저장 후 다시 삽입 (맨뒤로 위치)
            for (j = 0; j < dwSegmentLen; j++) {
              kGetQueue(&(pstTCB->stSendQueue), vbTemp + j);
              kPutQueue(&(pstTCB->stSendQueue), vbTemp + j);
            }

            // 세그먼트로 만들지 않은 데이터 인출 후 다시 삽입 (맨뒤로 위치)
            for (j = 0; j < (k - dwSegmentLen); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // SND.NXT 갱신
            pstTCB->dwSendNXT = pstTCB->dwSendNXT + dwSegmentLen;

            // 세그먼트 전송
            bPSH = bACK = TRUE;
            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, bPSH, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, vbTemp, dwSegmentLen, dwDestAddress);
          }

          // 송신 윈도우에 삽입한 만큼 반환
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, i);
          break; /* TCP_SEND */

        case TCP_RECEIVE:
          // 수신 가능한 만큼 유저 버퍼에 삽입
          for (i = 0; i < stRequest.wLen; i++) {
            if (kGetQueue(&(pstTCB->stRecvQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // 1바이트 이상 유저 버퍼에 삽입 시 반환
          if (i > 0) {
            // 수신 윈도우 크기 갱신
            pstTCB->dwRecvWND = pstTCB->dwRecvWND + i;

            // 유저 버퍼에 삽입한 만큼 반환
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, i);
          }
          else {
            //  return “error: connection closing”
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_CLOSING);
          }
          break; /* TCP_RECEIVE */

        case TCP_CLOSE:
          // FIN Segment 송신
          bACK = bFIN = TRUE;
          dwSegmentSEQ = pstTCB->dwSendNXT;
          dwSegmentACK = pstTCB->dwRecvNXT;

          kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
          kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, bFIN);
          kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

          // SND.NXT= 1증가 설정
          pstTCB->dwSendNXT = pstTCB->dwSendNXT + 1;
          // TCP_LAST_ACK 상태로 전이
          kTCP_StateTransition(pstTCB, TCP_LAST_ACK);

          // OK 반환
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_OK);
          break; /* TCP_CLOSE */
        }

        break; /* TCP_CLOSE_WAIT */

      case TCP_CLOSING:
      case TCP_LAST_ACK:
      case TCP_TIME_WAIT:
        switch (stRequest.eCode)
        {
        case TCP_OPEN:
          //  Return “error: connection already exists”
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;
        case TCP_SEND:
        case TCP_CLOSE:
        case TCP_RECEIVE:
          //  return “error: connection closing”
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_CLOSING);
          break;
        }

        break; /* TCP_TIME_WAIT */

      } /* switch (pstTCB->eState) */
    } /* if (bRequest) */

    // 수신 세그먼트 처리
    bFrame = kTCP_GetFrameFromTCB(pstTCB, &stFrame);
    if (bFrame) {
      // 수신 세그먼트 디캡슐화
      FRAME stBackupFrame = stFrame;
      dwSegmentLen = kDecapuslationSegment(&stFrame, &pstHeader, &pbPayload);
     
      pstTCB->qwTempSocket = ((stFrame.qwDestAddress & 0xFFFFFFFF00000000) | ((DWORD)(ntohs(pstHeader->wSourcePort)) << 16) | ntohs(pstHeader->wDestinationPort));

      // 세그먼트 처리 성공
      if (kTCP_ProcessSegment(pstTCB, pstHeader, pbPayload, dwSegmentLen) == TRUE)
        kFreeFrame(&stFrame);
      // 세그먼트 처리 지연
      else
        kTCP_PutFrameToTCB(pstTCB, &stBackupFrame);
    } /* if (bFrame) */

    // TCB 반납 
    if ((kTCP_GetCurrentState(pstTCB) == TCP_CLOSED) && (bRequest == FALSE) && (bFrame == FALSE)) {
      kTCP_DeleteTCB(pstTCB);
      return;
    }
  } /* while (1) */
}

void kTCP_SetHeaderSEQACK(TCP_HEADER* pstHeader, DWORD dwSEQ, DWORD dwACK)
{
  // RST Segment 송신
  pstHeader->dwSequenceNumber = htonl(dwSEQ);
  pstHeader->dwAcknowledgmentNumber = htonl(dwACK);
}

void kTCP_SetHeaderFlags(TCP_HEADER* pstHeader, BYTE bHeaderLen, BOOL bACK, BOOL bPSH, BOOL bRST, BOOL bSYN, BOOL bFIN)
{
  pstHeader->wDataOffsetAndFlags =
    htons((bHeaderLen << TCP_FLAGS_DATAOFFSET_SHIFT) |
      (bACK << TCP_FLAGS_ACK_SHIFT) |
      (bPSH << TCP_FLAGS_PSH_SHIFT) |
      (bRST << TCP_FLAGS_RST_SHIFT) |
      (bSYN << TCP_FLAGS_SYN_SHIFT) |
      (bFIN << TCP_FLAGS_FIN_SHIFT)
    );
}

BOOL kTCP_SendSegment(TCP_TCB* pstTCB, TCP_HEADER* pstHeader, WORD wHeaderSize, BYTE* pbPayload, WORD wPayloadLen, DWORD dwDestAddress)
{
  FRAME stFrame;
  RE_FRAME stReFrame;
  IPv4Pseudo_Header stPseudoHeader = { 0, };

  if (kAllocateFrame(&stFrame) == FALSE)
    return FALSE;
  
  // 체크섬 계산
  kNumberToAddressArray(stPseudoHeader.vbSourceIPAddress, kIP_GetIPAddress(NULL), 4);
  kNumberToAddressArray(stPseudoHeader.vbDestinationIPAddress, dwDestAddress, 4);
  stPseudoHeader.bProtocol = IP_PROTOCOL_TCP;
  stPseudoHeader.wLength = htons(wHeaderSize + wPayloadLen);

  pstHeader->wWindow = htons(pstTCB->dwRecvWND);
  pstHeader->wChecksum = htons(kTCP_CalcChecksum(&stPseudoHeader, pstHeader, pbPayload, wPayloadLen));

  kEncapuslationSegment(&stFrame, pstHeader, wHeaderSize, pbPayload, wPayloadLen);

  stFrame.bType = FRAME_TCP;
  stFrame.eDirection = FRAME_DOWN;
  stFrame.qwDestAddress = ((0xFFFFFFFFUL << 32) | dwDestAddress);

  // 필요시 재전송 큐에 삽입 (SYN, FIN, Data Segment)
  if ((wPayloadLen > 0) || (kTCP_IsNeedRetransmit(pstHeader) == TRUE)) {
    kAllocateReFrame(&stReFrame, &stFrame);
    kTCP_PutRetransmitFrameToTCB(pstTCB, &stReFrame);
  }

  kTCP_PutFrameToFrameQueue(&stFrame);
  return TRUE;
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
  dwLen = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_DATAOFFSET_SHIFT) * 4;
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
  if ((wPayloadLen != 0) && (((wPayloadLen / 2) * 2) != wPayloadLen)) {
    dwSum += ntohs((pwP[wPayloadLen / 2]) & 0x00FF);
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
  if (dwHeaderSize > 20) {
    kMemCpy(pstFrame->pbCur + 20, 
      ((TCP_HEADER*)pbHeader)->pbOption, 
      dwHeaderSize - 20);
  }
}

DWORD kDecapuslationSegment(FRAME* pstFrame, BYTE** ppbHeader, BYTE** ppbPayload)
{
  TCP_HEADER* pstHeader;
  DWORD dwHeaderSize;

  *ppbHeader = pstFrame->pbCur;
  pstHeader = pstFrame->pbCur;
  dwHeaderSize = (ntohs(pstHeader->wDataOffsetAndFlags) >> TCP_FLAGS_DATAOFFSET_SHIFT) * 4;

  pstFrame->wLen -= dwHeaderSize;
  pstFrame->pbCur += dwHeaderSize;

  if (ppbPayload != NULL)
    *ppbPayload = pstFrame->pbCur;

  // 세그먼트 데이터 길이 반환
  return pstFrame->wLen;
}