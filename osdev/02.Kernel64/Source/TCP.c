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

  // �⺻ ��� ����
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
    // ���� ��ũ��Ʈ�� FIN�� �����ϴ� ��� ����
    if (bFIN == TRUE)
      break;

    // RST�� ���� �ִ� ��� �۽������� RST ���׸�Ʈ ����
    if (bRST == FALSE) {
      bRST = TRUE;

      // ACK�� ���� �ִ� ��� ACK�� �۽�
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
      // RST Segment �۽�
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
    }

    break; /* TCP_CLOSED */

  case TCP_LISTEN:
    // FIN�̳� RST�� �����ϴ� ��� : ����
    if ((bFIN == TRUE) || (bRST == TRUE))
      break;

    // ACK�� �����ϴ� ��� : RST Segment �۽�
    if (bACK == TRUE) {
      // RST Segment �۽�
      bACK = FALSE;
      bRST = TRUE;
      dwSegmentSEQ = dwSegmentACK;
      dwSegmentACK = 0;

      // RST Segment �۽�
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // SYN�� �����ϴ� ��� : SYN ���׸�Ʈ ����, SYN_RECEIVED ���·� ����
    if (bSYN == TRUE) {
      // RCV.NXT=SEG.SEQ+1, IRS=SEG.SEQ�� ����
      pstTCB->dwRecvNXT = dwSegmentSEQ + 1;
      pstTCB->dwIRS = dwSegmentSEQ;

      // �۽� ������ �ʱ�ȭ
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->pbSendWindow = (BYTE*)kAllocateMemory(pstTCB->dwSendWND);
      if (pstTCB->pbSendWindow == NULL) {
        kPrintf("kTCP_Machine | Send Window Allocate Fail\n");
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      kInitializeQueue(&(pstTCB->stSendQueue), pstTCB->pbSendWindow, pstTCB->dwSendWND, sizeof(BYTE));

      // --- CRITCAL SECTION BEGIN ---
      kLock(&(gs_stTCPManager.stLock));

      // ISS ����
      pstTCB->dwISS = kTCP_GetISS();

      // ���� ������Ʈ
      pstTCB->stLink.qwID = pstTCB->qwTempSocket;

      kUnlock(&(gs_stTCPManager.stLock));
      // --- CRITCAL SECTION END ---

      // SYN Segment �۽� (SEQ : ISS, CTL : SYN, ACK)
      bSYN = bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwISS;
      dwSegmentACK = pstTCB->dwRecvNXT;

      stHeader.pbOption = vbOption;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 6, bACK, 0, 0, bSYN, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 24, NULL, 0, dwDestAddress);

      // SND.UNA=ISS, SND.NXT=ISS+1�� ����
      pstTCB->dwSendUNA = pstTCB->dwISS;
      pstTCB->dwSendNXT = pstTCB->dwISS + 1;

      // SYN_RECEIVED ���·� ���� 
      kTCP_StateTransition(pstTCB, TCP_SYN_RECEIVED);
      break;
    }

    break; /* TCP_LISTEN */

  case TCP_SYN_SENT:
    // ���� ��ũ��Ʈ�� FIN�� �����ϴ� ��� ����
    if (bFIN == TRUE)
      break;

    // ACK �����ϴ� ��� ACK ��ȣ Ȯ��
    if (bACK == TRUE) {
      // ACK�� ������ ����� ��� (SEG.ACK <= ISS, SND.NXT < SEG.ACK)
      // RST ���׸�Ʈ ����
      if ((dwSegmentACK <= pstTCB->dwISS) || (dwSegmentACK > pstTCB->dwSendNXT)) {
        // RST Segment �۽�
        bACK = FALSE;
        bRST = TRUE;
        dwSegmentSEQ = dwSegmentACK;
        dwSegmentACK = 0;

        // RST Segment �۽�
        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
        break;
      }
    }

    // ACK�� ���ų� ���� �� ���
    // RST Segment ���� ��
    if (bRST == TRUE) {
      if (bACK == TRUE) { // ���� ACK : CLOSED ���·� ����
        // "error: connection reset"
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      // No ACK �� ��� Segment ����
      break;
    }

    // SYN Ȯ��
    if (bSYN == TRUE) {
      // �ɼ� ó�� (MSS ������)
      kTCP_ProcessOption(pstTCB, pstHeader);

      // RCV.NXT=SEG.SEQ+1, IRS=SEG.SEQ�� ����
      pstTCB->dwRecvNXT = dwSegmentSEQ + 1;
      pstTCB->dwIRS = dwSegmentSEQ;
      pstTCB->dwSendWL1 = pstTCB->dwIRS;
      pstTCB->dwSendWL2 = dwSegmentACK;

      // �۽� ������ �ʱ�ȭ
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->pbSendWindow = (BYTE*)kAllocateMemory(pstTCB->dwSendWND);
      if (pstTCB->pbSendWindow == NULL) {
        kPrintf("kTCP_Machine | Send Window Allocate Fail\n");
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      kInitializeQueue(&(pstTCB->stSendQueue), pstTCB->pbSendWindow, pstTCB->dwSendWND, sizeof(BYTE));

      // ACK Ȯ��
      if (bACK == TRUE) {
        // SND.UNA ����
        pstTCB->dwSendUNA = dwSegmentACK;
        // ���� ���� ������ ������ ����
        kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
      }

      // SND.UNA > ISS (������ SYN Segment�� ������ ���� ���)
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> ����, ESTABLISHED ����
      if (pstTCB->dwSendUNA > pstTCB->dwISS) {
        bACK = TRUE;
        dwSegmentSEQ = pstTCB->dwSendNXT;
        dwSegmentACK = pstTCB->dwRecvNXT;

        kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
        kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
        kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

        // ESTABLISHED ���·� ����
        kTCP_StateTransition(pstTCB, TCP_ESTABLISHED);
        break;
      }

      // ������ SYN Segment ������ ���� ���ϰ�, ������ SYN Segment ���� ���� ���
      // <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK> ����
      bSYN = bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwISS;
      dwSegmentACK = pstTCB->dwRecvNXT;

      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, bSYN, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // ���� Data ó�� : ���� �����찡 �����ִ� ��ŭ ����
      j = MIN(wPayloadLen, pstTCB->dwRecvWND);
      for (i = 0; i < j; i++) {
        kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
      }
      // ���� ������ ũ�� ����
      pstTCB->dwRecvWND = pstTCB->dwRecvWND - j;

      // SYN_RECEIVED ���·� ����
      kTCP_StateTransition(pstTCB, TCP_SYN_RECEIVED);
    }
    // SYN, RST ��� �ƴ� Segment : ����
    break; /* TCP_SYN_SENT */

  case TCP_SYN_RECEIVED:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ���
    if (bRST == TRUE) {
      // Passive Open�� ���� ���۵� ��� : Listen State�� ����
      if (kTCP_GetPreviousState(pstTCB) == TCP_LISTEN) {
        kTCP_StateTransition(pstTCB, TCP_LISTEN);
      }
      // Active Open���� ���� ���۵� ��� : Closed ���·� ���� �� ����
      else if (kTCP_GetPreviousState(pstTCB) == TCP_SYN_SENT) {
        // ��� ��û, ���� �ڿ� �ʱ�ȭ
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
      break;
    }

    // SYN Segment �� ���
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT (SYN�� ���� ACK ����) : ESTABLISHED ���·� ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      // SND.UNA=SEG.ACK �� ����, ������ ť ����
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
      kTCP_StateTransition(pstTCB, TCP_ESTABLISHED);
    }
    // �߸��� ���� : RST Segment �۽�
    else {
      bRST = TRUE;
      dwSegmentSEQ = dwSegmentACK;
      dwSegmentACK = 0;
      // RST Segment �۽�
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, bRST, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
    }

    // FIN Bit�� ���� �ִ� ���
    if (bFIN == TRUE) {
      // ��� Receive ��û : "connection closing"
      // RCV.NXT = FIN, ACK �۽�
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // ���� Segment�� PUSH ó��
      // CLOSE-WAIT ���·� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSE_WAIT);
      break;
    }
    break; /* TCP_SYN_RECEIVED */

  case TCP_ESTABLISHED:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ���
    if (bRST == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ���
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK �� ����, ������ ť ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (���� �������� ���� SEQ�� ���� ACK)
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
    // SEG.ACK < SND.UNA (�ߺ� ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window ����
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) �� ���
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // �۽� ������ ����
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    // kPrintf("Segment Arrive : %p\n", dwSegmentSEQ - pstTCB->dwIRS);

    // Segment Data�� �����ϴ� ��� ���� ó��
    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : ����
        // ���� Data ó�� : ���� �����찡 �����ִ� ��ŭ ����
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // ���� ������ ũ�� ����
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
    // FIN Bit�� ���� �ִ� ���
    if (bFIN == TRUE) {
      // ��� Receive ��û : "connection closing"
      // RCV.NXT = FIN, ACK �۽�
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // ���� Segment�� PUSH ó��
      // CLOSE-WAIT ���·� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSE_WAIT);
    }

    break; /* TCP_ESTABLISHED */

  case TCP_FIN_WAIT_1:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ��� : TCP_CLOSED
    if (bRST == TRUE) {
      // ��� ��û�� "reset" ����
      // ���� �ڿ� �ʱ�ȭ
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ��� : TCP_CLOSED
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK �� ����, ������ ť ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (���� �������� ���� SEQ�� ���� ACK)
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
    // SEG.ACK < SND.UNA (�ߺ� ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window ����
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) �� ���
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // �۽� ������ ����
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    // �۽��� FIN�� ���� ���� Ȯ�� : FIN_WAIT_2 ���·� ���� �� ���׸�Ʈ ��ó��
    // (������ Segment ���� ���� ���� ���)
    if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
      kTCP_StateTransition(pstTCB, TCP_FIN_WAIT_2);
      kTCP_ProcessSegment(pstTCB, pstHeader, pbPayload, wPayloadLen);
      break;
    }

    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : ����
        // ���� Data ó�� : ���� �����찡 �����ִ� ��ŭ ����
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // ���� ������ ũ�� ����
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

    // FIN Bit�� ���� �ִ� ���
    if (bFIN == TRUE) {
      // ��� Receive ��û : "connection closing"
      // RCV.NXT = FIN, ACK �۽�
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // ���� Segment�� PUSH ó��
      // �۽��� FIN�� ���� ���� Ȯ�� : TIME_WAIT ���·� ���� (Ÿ�̸� ����)
      // (������ Segment ���� ���� ���� ���)
      if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
        pstTCB->qwTimeWaitTime = kGetTickCount() + (TCP_MSL_DEFAULT_SECOND * 1000);
        kTCP_StateTransition(pstTCB, TCP_TIME_WAIT);
        break;
      }
      // �۽��� FIN�� ���� ���� ��Ȯ�� : CLOSING ���·� ����
      else {
        kTCP_StateTransition(pstTCB, TCP_CLOSING);
        break;
      }
    }
    break; /* TCP_FIN_WAIT_1 */

  case TCP_FIN_WAIT_2:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ��� : TCP_CLOSED
    if (bRST == TRUE) {
      // ��� ��û�� "reset" ����
      // ���� �ڿ� �ʱ�ȭ
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ��� : TCP_CLOSED
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK �� ����, ������ ť ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (���� �������� ���� SEQ�� ���� ACK)
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
    // SEG.ACK < SND.UNA (�ߺ� ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window ����
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) �� ���
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // �۽� ������ ����
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : ����
        // ���� Data ó�� : ���� �����찡 �����ִ� ��ŭ ����
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // ���� ������ ũ�� ����
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

    // FIN Bit�� ���� �ִ� ��� : TIME_WAIT ���·� ����
    if (bFIN == TRUE) {
      // ��� Receive ��û : "connection closing"
      // RCV.NXT = FIN, ACK �۽�
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // ���� Segment�� PUSH ó��

      // Ÿ�̸� ���� �� TIME_WAIT ���·� ����
      pstTCB->qwTimeWaitTime = kGetTickCount() + (TCP_MSL_DEFAULT_SECOND * 1000);
      kTCP_StateTransition(pstTCB, TCP_TIME_WAIT);
    }
    break; /* TCP_FIN_WAIT_2 */

  case TCP_CLOSE_WAIT:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ��� : TCP_CLOSED
    if (bRST == TRUE) {
      // ��� ��û�� "reset" ����
      // ���� �ڿ� �ʱ�ȭ
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ��� : TCP_CLOSED
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK �� ����, ������ ť ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (���� �������� ���� SEQ�� ���� ACK)
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
    // SEG.ACK < SND.UNA (�ߺ� ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window ����
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) �� ���
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // �۽� ������ ����
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    if (wPayloadLen > 0) {
      if (dwSegmentSEQ == pstTCB->dwRecvNXT) {  // TODO : ����
        // ���� Data ó�� : ���� �����찡 �����ִ� ��ŭ ����
        j = MIN(wPayloadLen, pstTCB->dwRecvWND);
        for (i = 0; i < j; i++) {
          kPutQueue(&(pstTCB->stRecvQueue), pbPayload + i);
        }
        // ���� ������ ũ�� ����
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
    // FIN Bit�� ���� �ִ� ��� : PASS
    if (bFIN == TRUE) {
      break;
    }

    break; /* TCP_CLOSE_WAIT */

  case TCP_CLOSING:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ��� : TCP_CLOSED
    if (bRST == TRUE) {
      // ��� ��û�� "reset" ����
      // ���� �ڿ� �ʱ�ȭ
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ��� : TCP_CLOSED
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK �� ����, ������ ť ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }
    // SEG.ACK > SND.NXT (���� �������� ���� SEQ�� ���� ACK)
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
    // SEG.ACK < SND.UNA (�ߺ� ACK) : Pass
    else {

    }

    // SND.UNA < SEG.ACK =< SND.NXT : send window ����
    // SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK) �� ���
    if ((pstTCB->dwSendWL1 < dwSegmentSEQ) ||
      ((pstTCB->dwSendWL1 == dwSegmentSEQ) && (pstTCB->dwSendWL2 <= dwSegmentACK))) {
      // �۽� ������ ����
      for (i = 0; i < (dwSegmentACK - pstTCB->dwSendWL2); i++) {
        kGetQueue(&(pstTCB->stSendQueue), vbTemp + i);
      }

      // SND.WND <- SEG.WND, SND.WL1 <- SEG.SEQ, SND.WL2 <- SEG.ACK
      pstTCB->dwSendWND = wSegmentWindow;
      pstTCB->dwSendWL1 = dwSegmentSEQ;
      pstTCB->dwSendWL2 = dwSegmentACK;
    }

    // �۽��� FIN�� ���� ���� Ȯ�� : TIME_WAIT ���·� ���� �� ���׸�Ʈ ��ó��
    // (������ Segment ���� ���� ���� ���)
    if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
      // Ÿ�̸� ���� �� TIME_WAIT ���·� ����
      pstTCB->qwTimeWaitTime = kGetTickCount() + (TCP_MSL_DEFAULT_SECOND * 1000);
      kTCP_StateTransition(pstTCB, TCP_TIME_WAIT);
      break;
    }

    // FIN Bit�� ���� �ִ� ��� : PASS
    if (bFIN == TRUE) {
      break;
    }
    break; /* TCP_CLOSING */

  case TCP_LAST_ACK:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ��� : TCP_CLOSED
    if (bRST == TRUE) {
      // ��� ��û�� "reset" ����
      // ���� �ڿ� �ʱ�ȭ
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ��� : TCP_CLOSED
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // ACK ó��
    // SND.UNA < SEG.ACK =< SND.NXT : SND.UNA=SEG.ACK �� ����, ������ ť ����
    if ((pstTCB->dwSendUNA < dwSegmentACK) && (dwSegmentACK <= pstTCB->dwSendNXT)) {
      pstTCB->dwSendUNA = dwSegmentACK;
      kTCP_UpdateRetransmitQueue(pstTCB, dwSegmentACK);
    }

    // FIN�� ���� ACK�� ������ ��� : TCP_CLOSED
    // (������ Segment ���� ���� ���� ���)
    if (pstTCB->dwSendUNA == pstTCB->dwSendNXT) {
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // FIN Bit�� ���� �ִ� ��� : PASS
    if (bFIN == TRUE) {
      break;
    }

    break; /* TCP_LAST_ACK */

  case TCP_TIME_WAIT:
    // ���� Segment ������ȣ Ȯ��
    if (kTCP_IsDuplicateSegment(pstTCB, wPayloadLen, dwSegmentSEQ) == FALSE) {
      // ���� Segment�� �ùٸ��� ���� ��� 
      // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK> 
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
      break;
    }

    // RST Segment �� ��� : TCP_CLOSED
    if (bRST == TRUE) {
      // ��� ��û�� "reset" ����
      // ���� �ڿ� �ʱ�ȭ
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // SYN Segment �� ��� : TCP_CLOSED
    if (bSYN == TRUE) {
      // ��� ��û, ���� �ڿ� �ʱ�ȭ
      // Closed ���·� ���� �� ����
      kTCP_StateTransition(pstTCB, TCP_CLOSED);
      break;
    }

    // ACK Bit�� ���� �ִ� ��� : Segment ����
    if (bACK == FALSE) {
      break;
    }

    // FIN Bit�� ���� �ִ� ��� : ACK ����, 2 MSL Timeout �� ����
    if (bFIN == TRUE) {
      pstTCB->dwRecvNXT = pstTCB->dwRecvNXT + 1;
      bACK = TRUE;
      dwSegmentSEQ = pstTCB->dwSendNXT;
      dwSegmentACK = pstTCB->dwRecvNXT;
      kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
      kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
      kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

      // 2 MSL Timeout �� ����
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
  // �ʱ�ȭ
  if (kTCP_Initialize() == FALSE)
    return;

  while (1)
  {
    // ť Ȯ��
    if (kTCP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_UP:
      // kPrintf("TCP | Receive TCP Segment %d\n", i++);

      // ���׸�Ʈ ��� �и�
      pstHeader = (TCP_HEADER*)stFrame.pbCur;

      // ���� ���� ���� (Destination Address, Port, Local Port);
      // IP Header�� �ٿ��� �ּ�, TCP Header�� �ٿ��� ��Ʈ, ������ ��Ʈ
      dwForeignSocket = ntohs(pstHeader->wSourcePort);
      dwLocalSocket = ntohs(pstHeader->wDestinationPort);
      qwSocketPair = ((stFrame.qwDestAddress & 0xFFFFFFFF00000000) | (dwForeignSocket << 16) | dwLocalSocket);

      // --- CRITCAL SECTION BEGIN ---
      kLock(&(gs_stTCPManager.stLock));

      // ����� ���� �˻�
      pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), qwSocketPair);
      // ����� ������ ���� ���
      if (pstTCB == NULL) {
        // ���� ���� �˻�
        pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), dwLocalSocket);
      }

      // ����� ����, ���� ���� ��� ���� ���
      if (pstTCB == NULL) {
        kFreeFrame(&stFrame);
      }
      // ��Ʈ ��ȣ�� ���� ��Ƽ �÷���
      else {
        kTCP_PutFrameToTCB(pstTCB, &stFrame);
      }

      kUnlock(&(gs_stTCPManager.stLock));
      // --- CRITCAL SECTION END ---

      break;  /* End of case FRAME_UP: */

    case FRAME_DOWN:
      // kPrintf("TCP | Send TCP Segment \n");

      // IP�� ����
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

  // �и� ������ ����
  qwDiff = qwNow - gs_stTCPManager.qwISSTime;

  // 4 ����ũ�� �����帶�� 1 ����
  dwISS = (gs_stTCPManager.dwISS + (qwDiff * 250)) % 4294967296;
  gs_stTCPManager.dwISS = dwISS;

  return dwISS;
}

BOOL kTCP_Initialize(void)
{
  // ���ؽ� �ʱ�ȭ
  kInitializeMutex(&(gs_stTCPManager.stLock));

  // ���� ��ȣ �ʱ�ȭ
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

  // TCB ����Ʈ �ʱ�ȭ
  kInitializeList(&(gs_stTCPManager.stTCBList));
  gs_stTCPManager.pstCurrentTCB = NULL;

  // ���̾� ����
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

  // TCB ����Ʈ �˻�
  QWORD qwSocketPair = (qwForeignSocket << 16) | wLocalPort;
  pstTCB = (TCP_TCB*)kFindList(&(gs_stTCPManager.stTCBList), qwSocketPair);

  // ���� �ֿ� �Ҵ�� TCB �� �̹� �����ϴ� ���.
  if (pstTCB != NULL) {
    // TODO : ó�� �ڵ� �߰�
    kPrintf("kTCP_CreateTCB | TCB already exists\n");
    return NULL;
  }

  // TCB �Ҵ�
  pstTCB = (TCP_TCB*)kAllocateMemory(sizeof(TCP_TCB));
  if (pstTCB == NULL) {
    kPrintf("kTCP_CreateTCB | TCB Allocate Fail\n");
    return NULL;
  }

  // TCB �ʱ�ȭ
  // ��û ť �Ҵ�
  pstTCB->pstRequestBuffer = (TCP_REQUEST*)kAllocateMemory(TCP_REQUEST_QUEUE_MAX_COUNT * sizeof(TCP_REQUEST));
  if (pstTCB->pstRequestBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | RequestBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    return NULL;
  }
  // ��û ť �ʱ�ȭ
  kInitializeQueue(&(pstTCB->stRequestQueue), pstTCB->pstRequestBuffer, TCP_REQUEST_QUEUE_MAX_COUNT, sizeof(TCP_REQUEST));

  // ������ ť �Ҵ�
  pstTCB->pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (pstTCB->pstFrameBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | FrameBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    kFreeMemory(pstTCB->pstRequestBuffer);
    return NULL;
  }
  // ������ ť �ʱ�ȭ
  kInitializeQueue(&(pstTCB->stFrameQueue), pstTCB->pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // ������ ������ ť �Ҵ�
  pstTCB->pstRetransmitFrameBuffer = (RE_FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(RE_FRAME));
  if (pstTCB->pstRetransmitFrameBuffer == NULL) {
    kPrintf("kTCP_CreateTCB | RetransmitFrameBuffer Allocate Fail\n");
    kFreeMemory(pstTCB);
    kFreeMemory(pstTCB->pstRequestBuffer);
    kFreeMemory(pstTCB->pstFrameBuffer);
    return NULL;
  }
  // ������ ������ ť �ʱ�ȭ
  kInitializeQueue(&(pstTCB->stRetransmitFrameQueue), pstTCB->pstRetransmitFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(RE_FRAME));

  //  ���� ������ �Ҵ�
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

  // ���ؽ� �ʱ�ȭ
  kInitializeMutex(&(pstTCB->stLock));

  // TCB ���� �ʱ�ȭ
  pstTCB->stLink.qwID = qwSocketPair;
  pstTCB->eState = TCP_CLOSED;
  pstTCB->dwISS = kTCP_GetISS();  // �ʱ� ������ȣ
  pstTCB->dwSendWL1 = pstTCB->dwSendWL2 = pstTCB->dwSendUNA = pstTCB->dwSendNXT = pstTCB->dwISS;
  pstTCB->pbSendWindow = NULL;

  // TCB ����Ʈ�� �߰�
  gs_stTCPManager.pstCurrentTCB = pstTCB;
  kAddListToHeader(&(gs_stTCPManager.stTCBList), pstTCB);

  // Request ����
  stRequest.eCode = TCP_OPEN;
  stRequest.eFlag = eFlag;
  stRequest.pqwRet = NULL;
  kTCP_PutRequestToTCB(pstTCB, &stRequest);
  
  // TCB ��� ���� �ӽ� ����
  if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_HIGHEST, 0, 0,
    (QWORD)kTCP_Machine, TASK_LOADBALANCINGID) == NULL)
  {
    kPrintf("Create kTCP_Machine Fail\n");
  }

  // �ӽ� ���� �Ϸ�
  while (gs_stTCPManager.pstCurrentTCB != NULL);

  kUnlock(&(gs_stTCPManager.stLock));
  // --- CRITCAL SECTION END ---

  return pstTCB;
}

BYTE kTCP_DeleteTCB(TCP_TCB* pstTCB)
{
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stTCPManager.stLock));

  // �۽� ������ ����
  if (pstTCB->pbSendWindow != NULL) {
    kFreeMemory(pstTCB->pbSendWindow);
  }
  // ���� ������ ����
  if (pstTCB->pbRecvWindow != NULL) {
    kFreeMemory(pstTCB->pbRecvWindow);
  }
  // ��û ó�� ť ����
  kFreeMemory(pstTCB->pstRequestBuffer);
  // ������ ť ����
  kFreeMemory(pstTCB->pstFrameBuffer);
  // ������ ť ����
  kFreeMemory(pstTCB->pstRetransmitFrameBuffer);

  // ���� �ݳ�
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
    // ������ ť �� �� ������ Ȯ��
    bResult = kFrontQueue(&(pstTCB->stRetransmitFrameQueue), &stReFrame);
    if (bResult == FALSE)
      return;

    // ������ �ð� �ʰ��� ��� : ť���� ����
    if (stReFrame.qwTime <= qwTime) {
      bResult = kGetQueue(&(pstTCB->stRetransmitFrameQueue), &stReFrame);
      // ACK�� ���� �������� �ƴ� ��� : ������
      // ACK ���� ��� : PASS
      if (stReFrame.stFrame.bType != FRAME_PASS) {
        // ������ �������� ������ ������ ����
        kAllocateReFrame(&stReReFrame, &(stReFrame.stFrame));
        kTCP_PutRetransmitFrameToTCB(pstTCB, &stReReFrame);

        // ������ (TCP ���� ���� ����)
        kTCP_PutFrameToFrameQueue(&(stReFrame.stFrame));
      }
    }
    // ������ �ð� �ʰ����� ���� ��� : PASS
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

    // ��� Ȯ��
    pstHeader = (TCP_HEADER*)(stReFrame.stFrame.pbCur);

    // ������ȣ�� Ȯ���Ͽ� ���� �Ϸ�� ���׸�Ʈ : ť���� ����
    // �Ϸ���� ���� ���׸�Ʈ : ������ ť�� �����
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

  // ACK �� �������ִ� ��� ������ �ʿ����
  if ((bSYN == TRUE) || (bFIN == TRUE))
    return TRUE;
  return FALSE;
}

BOOL kTCP_IsDuplicateSegment(const TCP_TCB* pstTCB, DWORD dwSEGLEN, DWORD dwSEGSEQ)
{
  // 1. SEQ ��ȣ�� Ȯ���Ͽ� �ߺ� Segment ����
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
  // TCB ��������
  TCP_TCB* pstTCB = gs_stTCPManager.pstCurrentTCB;
  if (pstTCB == NULL)
    return;
  gs_stTCPManager.pstCurrentTCB = NULL;

  // �⺻ ��� ����
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

    // TIME_WAIT Ÿ�̸� ����� : CLOSE ���·� ����
    if (kTCP_GetCurrentState(pstTCB) == TCP_TIME_WAIT) {
      if (kGetTickCount() >= pstTCB->qwTimeWaitTime) {
        kTCP_StateTransition(pstTCB, TCP_CLOSED);
      }
    }

    // ������ ó��
    kTCP_ProcessRetransmitQueue(pstTCB);

    // ��û ó��
    bRequest = kTCP_GetRequestFromTCB(pstTCB, &stRequest, FALSE);
    // ��û�� �ִ� ���
    if (bRequest) {
      switch (kTCP_GetCurrentState(pstTCB))
      {
      case TCP_CLOSED:
        switch (stRequest.eCode)
        {
          // Open Call ó��
        case TCP_OPEN:
          if (stRequest.eFlag == TCP_PASSIVE) {
            // LISTEN ���·� ����
            kTCP_StateTransition(pstTCB, TCP_LISTEN);
          }
          else if (stRequest.eFlag == TCP_ACTIVE) {
            // SYN Segment �۽� (SEQ : ISS, CTL : SYN)
            bSYN = TRUE;
            dwSegmentSEQ = pstTCB->dwISS;
            dwSegmentACK = 0;

            stHeader.pbOption = vbOption;
            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 6, 0, 0, 0, bSYN, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 24, NULL, 0, dwDestAddress);

            // SND.UNA=ISS, SND.NXT=ISS+1�� ����
            pstTCB->dwSendUNA = pstTCB->dwISS;
            pstTCB->dwSendNXT = pstTCB->dwISS + 1;

            // SYN_SENT ���·� ����
            kTCP_StateTransition(pstTCB, TCP_SYN_SENT);
          }

          // TCB ��ȯ
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, pstTCB);
          break;

          // ������ : Error
        default: /* return ��error: connection does not exist�� */
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
          // Open Call ó��
        case TCP_OPEN:  /* Return ��error: connection already exists�� */
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;

          // Send, Recv : ��û ó�� ����. 
        case TCP_SEND:
        case TCP_RECEIVE:
          break;
        }
        break;  /* TCP_SYN_SENT */

      case TCP_ESTABLISHED:
        switch (stRequest.eCode)
        {
        case TCP_SEND:
          // ������ ���� ��� ��� : Keep Alive Segment �����Ͽ� ������ Ȯ��
          // TODO : �ٺ� ������ ���� 
          if (pstTCB->dwSendWND == 0)
          {
            // Keep Alive (ACK) Segment �۽� (SEQ : SND.NXT - 1, ACK : RCV.NXT, CTL : ACK)
            bACK = TRUE;
            dwSegmentSEQ = pstTCB->dwSendNXT - 1;
            dwSegmentACK = pstTCB->dwRecvNXT;

            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
          }

          // �۽� �����쿡 ���԰����� ��ŭ ����
          for (i = 0; i < stRequest.wLen; i++) {
            if (kPutQueue(&(pstTCB->stSendQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // PUSH �÷��� ������ ��� OR 
          // �Ӱ�ġ ��ŭ �����Ͱ� ���� ��� ���׸�Ʈ�� ����� ����
          // ���׸�Ʈ ũ��� MSS�� ���� ����.
          dwSegmentLen = MIN(kGetQueueSize(&(pstTCB->stSendQueue)) - (pstTCB->dwSendNXT - pstTCB->dwSendUNA), pstTCB->dwMSS);
          if ((stRequest.eFlag == TCP_PUSH) || (dwSegmentLen >= TCP_MINIMUM_SEGMENT_SIZE)) {
            dwSegmentSEQ = pstTCB->dwSendNXT;
            dwSegmentACK = pstTCB->dwRecvNXT;

            // TODO : Deque ���·� ����
            // �տ� ��ġ�� UNA �κ� ť���� �����Ͽ� �ٽ� ���� (�ǵڷ� ��ġ)
            k = kGetQueueSize(&(pstTCB->stSendQueue));
            for (j = 0; j < (pstTCB->dwSendNXT - pstTCB->dwSendUNA); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // ���׸�Ʈ�� ���� ������ �����Ͽ� �ӽù��ۿ� ���� �� �ٽ� ���� (�ǵڷ� ��ġ)
            for (j = 0; j < dwSegmentLen; j++) {
              kGetQueue(&(pstTCB->stSendQueue), vbTemp + j);
              kPutQueue(&(pstTCB->stSendQueue), vbTemp + j);
            }

            // ���׸�Ʈ�� ������ ���� ������ ���� �� �ٽ� ���� (�ǵڷ� ��ġ)
            for (j = 0; j < (k - dwSegmentLen - (pstTCB->dwSendNXT - pstTCB->dwSendUNA)); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // SND.NXT ����
            pstTCB->dwSendNXT = pstTCB->dwSendNXT + dwSegmentLen;

            // ���׸�Ʈ ����
            bPSH = bACK = TRUE;
            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, bPSH, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, vbTemp, dwSegmentLen, dwDestAddress);
          }

          // �۽� �����쿡 ������ ��ŭ ��ȯ
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, i);
          break; /* TCP_SEND */

        case TCP_RECEIVE:
          // ���� ������ ��ŭ ���� ���ۿ� ����
          for (i = 0; i < stRequest.wLen; i++) {
            if (kGetQueue(&(pstTCB->stRecvQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }
          // 1����Ʈ �̻� ���� ���ۿ� ���� �� ��ȯ OR NONBLOCK ��û�� ���
          if ((i > 0) || (stRequest.eFlag == TCP_NONBLOCK)) {
            // ���� ������ ũ�� ����
            pstTCB->dwRecvWND = pstTCB->dwRecvWND + i;

            // ���� ���ۿ� ������ ��ŭ ��ȯ
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, i);
          }
          break; /* TCP_RECEIVE */

        case TCP_CLOSE:
          // FIN Segment �۽�
          bACK = bFIN = TRUE;
          dwSegmentSEQ = pstTCB->dwSendNXT;
          dwSegmentACK = pstTCB->dwRecvNXT;

          kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
          kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, bFIN);
          kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

          // SND.NXT= 1���� ����
          pstTCB->dwSendNXT = pstTCB->dwSendNXT + 1;
          // TCP_FIN_WAIT_1 ���·� ����
          kTCP_StateTransition(pstTCB, TCP_FIN_WAIT_1);

          // OK ��ȯ
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
          //  Return ��error: connection already exists��
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;
        case TCP_SEND:
        case TCP_CLOSE:
          //  return ��error: connection closing��
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_CLOSING);
          break;

        case TCP_RECEIVE:
          // ���� ������ ��ŭ ���� ���ۿ� ����
          for (i = 0; i < stRequest.wLen; i++) {
            if (kGetQueue(&(pstTCB->stRecvQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // 1����Ʈ �̻� ���� ���ۿ� ���� �� ��ȯ OR NONBLOCK ��û�� ���
          if ((i > 0) || (stRequest.eFlag == TCP_NONBLOCK)) {
            // ���� ������ ũ�� ����
            pstTCB->dwRecvWND = pstTCB->dwRecvWND + i;

            // ���� ���ۿ� ������ ��ŭ ��ȯ
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
          //  Return ��error: connection already exists��
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;
        case TCP_SEND:
          // ������ ���� ��� ��� : Keep Alive Segment �����Ͽ� ������ Ȯ��
          // TODO : �ٺ� ������ ���� 
          if (pstTCB->dwSendWND == 0)
          {
            // Keep Alive (ACK) Segment �۽� (SEQ : SND.NXT - 1, ACK : RCV.NXT, CTL : ACK)
            bACK = TRUE;
            dwSegmentSEQ = pstTCB->dwSendNXT - 1;
            dwSegmentACK = pstTCB->dwRecvNXT;

            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);
          }

          // �۽� �����쿡 ���԰����� ��ŭ ����
          for (i = 0; i < stRequest.wLen; i++) {
            if (kPutQueue(&(pstTCB->stSendQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // PUSH �÷��� ������ ��� OR 
          // �Ӱ�ġ ��ŭ �����Ͱ� ���� ��� ���׸�Ʈ�� ����� ����
          // ���׸�Ʈ ũ��� MSS�� ���� ����.
          dwSegmentLen = MIN(kGetQueueSize(&(pstTCB->stSendQueue)) - (pstTCB->dwSendNXT - pstTCB->dwSendUNA), pstTCB->dwMSS);
          if ((stRequest.eFlag == TCP_PUSH) || (dwSegmentLen >= TCP_MINIMUM_SEGMENT_SIZE)) {
            dwSegmentSEQ = pstTCB->dwSendNXT;
            dwSegmentACK = pstTCB->dwRecvNXT;


            // TODO : Deque ���·� ����
            // �տ� ��ġ�� UNA �κ� ť���� �����Ͽ� �ٽ� ���� (�ǵڷ� ��ġ)
            k = kGetQueueSize(&(pstTCB->stSendQueue));
            for (j = 0; j < (pstTCB->dwSendNXT - pstTCB->dwSendUNA); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // ���׸�Ʈ�� ���� ������ �����Ͽ� �ӽù��ۿ� ���� �� �ٽ� ���� (�ǵڷ� ��ġ)
            for (j = 0; j < dwSegmentLen; j++) {
              kGetQueue(&(pstTCB->stSendQueue), vbTemp + j);
              kPutQueue(&(pstTCB->stSendQueue), vbTemp + j);
            }

            // ���׸�Ʈ�� ������ ���� ������ ���� �� �ٽ� ���� (�ǵڷ� ��ġ)
            for (j = 0; j < (k - dwSegmentLen); j++) {
              kGetQueue(&(pstTCB->stSendQueue), &l);
              kPutQueue(&(pstTCB->stSendQueue), &l);
            }

            // SND.NXT ����
            pstTCB->dwSendNXT = pstTCB->dwSendNXT + dwSegmentLen;

            // ���׸�Ʈ ����
            bPSH = bACK = TRUE;
            kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
            kTCP_SetHeaderFlags(&stHeader, 5, bACK, bPSH, 0, 0, 0);
            kTCP_SendSegment(pstTCB, &stHeader, 20, vbTemp, dwSegmentLen, dwDestAddress);
          }

          // �۽� �����쿡 ������ ��ŭ ��ȯ
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, i);
          break; /* TCP_SEND */

        case TCP_RECEIVE:
          // ���� ������ ��ŭ ���� ���ۿ� ����
          for (i = 0; i < stRequest.wLen; i++) {
            if (kGetQueue(&(pstTCB->stRecvQueue), stRequest.pbBuf + i) == FALSE)
              break;
          }

          // 1����Ʈ �̻� ���� ���ۿ� ���� �� ��ȯ
          if (i > 0) {
            // ���� ������ ũ�� ����
            pstTCB->dwRecvWND = pstTCB->dwRecvWND + i;

            // ���� ���ۿ� ������ ��ŭ ��ȯ
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, i);
          }
          else {
            //  return ��error: connection closing��
            kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
            kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_CLOSING);
          }
          break; /* TCP_RECEIVE */

        case TCP_CLOSE:
          // FIN Segment �۽�
          bACK = bFIN = TRUE;
          dwSegmentSEQ = pstTCB->dwSendNXT;
          dwSegmentACK = pstTCB->dwRecvNXT;

          kTCP_SetHeaderSEQACK(&stHeader, dwSegmentSEQ, dwSegmentACK);
          kTCP_SetHeaderFlags(&stHeader, 5, bACK, 0, 0, 0, bFIN);
          kTCP_SendSegment(pstTCB, &stHeader, 20, NULL, 0, dwDestAddress);

          // SND.NXT= 1���� ����
          pstTCB->dwSendNXT = pstTCB->dwSendNXT + 1;
          // TCP_LAST_ACK ���·� ����
          kTCP_StateTransition(pstTCB, TCP_LAST_ACK);

          // OK ��ȯ
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
          //  Return ��error: connection already exists��
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_ALREADY_EXIST);
          break;
        case TCP_SEND:
        case TCP_CLOSE:
        case TCP_RECEIVE:
          //  return ��error: connection closing��
          kTCP_GetRequestFromTCB(pstTCB, &stRequest, TRUE);
          kTCP_ReturnRequest(&stRequest, TCP_RET_ERROR_CONNECTION_CLOSING);
          break;
        }

        break; /* TCP_TIME_WAIT */

      } /* switch (pstTCB->eState) */
    } /* if (bRequest) */

    // ���� ���׸�Ʈ ó��
    bFrame = kTCP_GetFrameFromTCB(pstTCB, &stFrame);
    if (bFrame) {
      // ���� ���׸�Ʈ ��ĸ��ȭ
      FRAME stBackupFrame = stFrame;
      dwSegmentLen = kDecapuslationSegment(&stFrame, &pstHeader, &pbPayload);
     
      pstTCB->qwTempSocket = ((stFrame.qwDestAddress & 0xFFFFFFFF00000000) | ((DWORD)(ntohs(pstHeader->wSourcePort)) << 16) | ntohs(pstHeader->wDestinationPort));

      // ���׸�Ʈ ó�� ����
      if (kTCP_ProcessSegment(pstTCB, pstHeader, pbPayload, dwSegmentLen) == TRUE)
        kFreeFrame(&stFrame);
      // ���׸�Ʈ ó�� ����
      else
        kTCP_PutFrameToTCB(pstTCB, &stBackupFrame);
    } /* if (bFrame) */

    // TCB �ݳ� 
    if ((kTCP_GetCurrentState(pstTCB) == TCP_CLOSED) && (bRequest == FALSE) && (bFrame == FALSE)) {
      kTCP_DeleteTCB(pstTCB);
      return;
    }
  } /* while (1) */
}

void kTCP_SetHeaderSEQACK(TCP_HEADER* pstHeader, DWORD dwSEQ, DWORD dwACK)
{
  // RST Segment �۽�
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
  
  // üũ�� ���
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

  // �ʿ�� ������ ť�� ���� (SYN, FIN, Data Segment)
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

  // üũ�� �ʱ�ȭ
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

  // ������ �ѹ���Ʈ ���� ���
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

  // �ɼ��� �����ϴ� ��� ����
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

  // ���׸�Ʈ ������ ���� ��ȯ
  return pstFrame->wLen;
}