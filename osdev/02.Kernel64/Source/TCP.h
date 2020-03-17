#ifndef __TCP_H__
#define __TCP_H__

#include "Synchronization.h"
#include "Frame.h"
#include "Queue.h"
#include "List.h"
#include "IP.h"

#define TCP_FLAGS_DATAOFFSET_SHIFT 12
#define TCP_FLAGS_FIN_SHIFT  0
#define TCP_FLAGS_SYN_SHIFT  1
#define TCP_FLAGS_RST_SHIFT  2
#define TCP_FLAGS_PSH_SHIFT  3
#define TCP_FLAGS_ACK_SHIFT  4
#define TCP_FLAGS_URG_SHIFT  5

#define TCP_OPTION_EOL 0
#define TCP_OPTION_NO  1
#define TCP_OPTION_MSS 2

#define TCP_REQUEST_QUEUE_MAX_COUNT 100

#define TCP_WINDOW_DEFAULT_SIZE 8192 * 5
#define TCP_MAXIMUM_SEGMENT_SIZE 1380
#define TCP_MINIMUM_SEGMENT_SIZE 500
#define TCP_MSL_DEFAULT_SECOND 120

typedef enum kTCPFlag
{
  TCP_PASSIVE = 0,
  TCP_ACTIVE = 1,
  TCP_PUSH = 2,
  TCP_NONBLOCK = 3,
  TCP_BLOCK = 4,
} TCP_FLAG;

typedef enum kTCPState
{
  TCP_CLOSED        = 0,
  TCP_LISTEN        = 1,
  TCP_SYN_SENT      = 2,
  TCP_SYN_RECEIVED  = 3,
  TCP_ESTABLISHED   = 4,
  TCP_FIN_WAIT_1    = 5,
  TCP_FIN_WAIT_2    = 6,
  TCP_CLOSE_WAIT    = 7,
  TCP_CLOSING       = 8,
  TCP_LAST_ACK      = 9,
  TCP_TIME_WAIT     = 10,
  TCP_UNKNOWN       = 11,
} TCP_STATE;

typedef enum kTCPRequestCode
{
  TCP_OPEN = 0,
  TCP_SEND = 1,
  TCP_RECEIVE = 2,
  TCP_CLOSE = 3,
  TCP_ABORT = 4,
  TCP_STATUS = 5,
} TCP_RCODE;

typedef enum kTCPReturnCode
{
  TCP_RET_OK  = 1,
  TCP_RET_ERROR_CONNECTION_DOES_NOT_EXIST = -1,
  TCP_RET_ERROR_CONNECTION_ALREADY_EXIST = -2,
  TCP_RET_ERROR_CONNECTION_CLOSING = -3,
} TCP_RETCODE;

#pragma pack(push, 1)

typedef struct kTCPRequest {
  TCP_RCODE eCode;
  TCP_FLAG eFlag;
  QWORD qwTime;
  BYTE* pbBuf;
  WORD wLen;
  volatile long* pqwRet;
} TCP_REQUEST;

typedef struct kTCPHeader {
  WORD wSourcePort;
  WORD wDestinationPort;
  DWORD dwSequenceNumber;
  DWORD dwAcknowledgmentNumber;
  WORD wDataOffsetAndFlags;
  WORD wWindow;
  WORD wChecksum;
  WORD wUrgentPointer;
  BYTE* pbOption;
} TCP_HEADER;

typedef struct kTCPControlBlock
{
  LISTLINK stLink;
  QWORD qwTempSocket;

  // 동기화 객체
  MUTEX stLock;

  // 상태
  TCP_STATE eState; // 이전 상태 (16 bits) | 현재 상태 (16 bits)
  QWORD qwTimeWaitTime;

  // 송신 관련
  QUEUE stSendQueue;
  BYTE* pbSendWindow; // 송신 윈도우
  DWORD dwSendUNA;
  DWORD dwSendNXT;
  DWORD dwSendWND;
  DWORD dwSendUP;
  DWORD dwSendWL1;
  DWORD dwSendWL2;
  DWORD dwISS;  // 초기 송신 순서번호
  DWORD dwMSS;

  // 수신 관련
  QUEUE stRecvQueue;
  BYTE* pbRecvWindow; // 수신 윈도우
  DWORD dwRecvNXT;
  DWORD dwRecvWND;
  DWORD dwRecvUP;
  DWORD dwIRS;  // 초기 수신 순서번호

  // 요청 처리 큐
  QUEUE stRequestQueue;
  TCP_REQUEST* pstRequestBuffer;

  // 프레임 큐
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  // 재전송 프레임 큐
  QUEUE stRetransmitFrameQueue;
  RE_FRAME* pstRetransmitFrameBuffer;
} TCP_TCB;

typedef struct kTCPManager
{
  // 동기화 객체
  MUTEX stLock;

  // 초기 순서번호
  QWORD qwISSTime;
  DWORD dwISS;

  // 프레임 큐
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  // TCB List
  TCP_TCB* pstCurrentTCB;
  LIST stTCBList;

  DownFunction pfDownIP;
} TCPMANAGER;

#pragma pack(pop)

void kTCP_Task(void);
BOOL kTCP_Initialize(void);
DWORD kTCP_GetISS(void);

TCP_TCB* kTCP_CreateTCB(WORD wLocalPort, QWORD qwForeignSocket, TCP_FLAG bFlag);
BYTE kTCP_DeleteTCB(TCP_TCB* pstTCB);

BYTE kTCP_PutFrameToTCB(const TCP_TCB* pstTCB, const FRAME* pstFrame);
BYTE kTCP_GetFrameFromTCB(const TCP_TCB* pstTCB, FRAME* pstFrame);
BYTE kTCP_PutRequestToTCB(const TCP_TCB* pstTCB, const TCP_REQUEST* pstRequest);
BYTE kTCP_GetRequestFromTCB(const TCP_TCB* pstTCB, TCP_REQUEST* pstRequest, BOOL bPop);
void kTCP_Machine(void);

inline void kTCP_StateTransition(TCP_TCB* pstTCB, TCP_STATE eToState);
inline TCP_STATE kTCP_GetPreviousState(const TCP_TCB* pstTCB);
inline TCP_STATE kTCP_GetCurrentState(const TCP_TCB* pstTCB);

BYTE kTCP_PutRetransmitFrameToTCB(const TCP_TCB* pstTCB, RE_FRAME* pstFrame);
void kTCP_ProcessRetransmitQueue(const TCP_TCB* pstTCB);
void kTCP_UpdateRetransmitQueue(const TCP_TCB* pstTCB, DWORD dwACK);
BOOL kTCP_IsNeedRetransmit(const TCP_HEADER* pstHeader);
BOOL kTCP_IsDuplicateSegment(const TCP_TCB* pstTCB, DWORD dwSEGLEN, DWORD dwSEGSEQ);
BOOL kTCP_ProcessSegment(TCP_TCB* pstTCB, TCP_HEADER* pstHeader, BYTE* pbPayload, WORD wPayloadLen);
void kTCP_ProcessRequest();
void kTCP_ProcessOption(TCP_TCB* pstTCB, TCP_HEADER* pstHeader);
inline void kTCP_ReturnRequest(TCP_REQUEST* pstRequest, long qwReturnValue);

// 인터페이스
TCP_TCB* kTCP_Open(WORD wLocalPort, QWORD qwForeignSocket, BYTE bFlag);
long kTCP_Send(TCP_TCB* pstTCB, BYTE* pbBuf, WORD wLen, BYTE bFlag);
long kTCP_Recv(TCP_TCB* pstTCB, BYTE* pbBuf, WORD wLen, BYTE bFlag);
BYTE kTCP_Close(TCP_TCB* pstTCB);
BYTE kTCP_Abort(TCP_TCB* pstTCB);
BYTE kTCP_Status(TCP_TCB* pstTCB);

BOOL kTCP_SendSegment(TCP_TCB* pstTCB, TCP_HEADER* pstHeader, WORD wHeaderSize, BYTE* pbPayload, WORD wPayloadLen, DWORD dwDestAddress);
WORD kTCP_CalcChecksum(IPv4Pseudo_Header* pstPseudo_Header, TCP_HEADER* pstHeader, BYTE* pbPayload, WORD wPayloadLen);
void kTCP_SetHeaderSEQACK(TCP_HEADER* pstHeader, DWORD dwSEQ, DWORD dwACK);
void kTCP_SetHeaderFlags(TCP_HEADER* pstHeader, BYTE bHeaderLen, BOOL bACK, BOOL bPSH, BOOL bRST, BOOL bSYN, BOOL bFIN);


BOOL kTCP_UpDirectionPoint(FRAME stFrame);
BOOL kTCP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kTCP_GetFrameFromFrameQueue(FRAME* pstFrame);
void kEncapuslationSegment(FRAME* pstFrame, BYTE* pbHeader, DWORD dwHeaderSize,
  BYTE* pbPayload, DWORD dwPayloadSize);
DWORD kDecapuslationSegment(FRAME* pstFrame, BYTE** ppbHeader, BYTE** ppbPayload);

#endif /* __TCP_H__ */