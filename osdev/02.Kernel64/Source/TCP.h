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

#define TCP_REQUEST_QUEUE_MAX_COUNT 100

#define TCP_WINDOW_DEFAULT_SIZE 8192

typedef enum kTCPFlag
{
  TCP_PASSIVE = 0,
  TCP_ACTIVE = 1
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
  TCP_TIME_WAIT     = 10
} TCP_STATE;

typedef enum kTCPRequestCode
{
  TCP_OPEN = 0,
} TCP_RCODE;

#pragma pack(push, 1)

typedef struct kTCPRequest {
  TCP_RCODE eCode;
  TCP_FLAG eFlag;
  QWORD qwTime;
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

  // 동기화 객체
  MUTEX stLock;

  // 상태
  TCP_STATE eState;

  // 송신 관련
  BYTE* pbSendWindow; // 송신 윈도우
  DWORD dwSendUNA;
  DWORD dwSendNXT;
  DWORD dwSendWND;
  DWORD dwSendUP;
  DWORD dwSendWL1;
  DWORD dwSendWL2;
  DWORD dwISS;  // 초기 송신 순서번호

  // 수신 관련
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
BYTE kTCP_InitTCB(TCP_TCB* pstTCB);
BYTE* kTCP_FreeTCB(TCP_TCB* pstTCB);

BYTE kTCP_PutFrameToTCB(const TCP_TCB* pstTCB, const FRAME* pstRequest);
BYTE kTCP_GetFrameFromTCB(const TCP_TCB* pstTCB, FRAME* pstRequest);
BYTE kTCP_PutRequestToTCB(const TCP_TCB* pstTCB, const TCP_REQUEST* pstRequest);
BYTE kTCP_GetRequestFromTCB(const TCP_TCB* pstTCB, TCP_REQUEST* pstRequest);
void kTCP_Machine(void);

// 인터페이스
TCP_TCB* kTCP_Open(WORD wLocalPort, QWORD qwForeignSocket, BYTE bFlag);
BYTE kTCP_Send(TCP_TCB* pstTCB, BYTE* bBuf, WORD wLen, BYTE bFlag);
BYTE kTCP_Recv(TCP_TCB* pstTCB, BYTE* bBuf, WORD wLen, BYTE bFlag);
BYTE kTCP_Close(TCP_TCB* pstTCB);
BYTE kTCP_Abort(TCP_TCB* pstTCB);
BYTE kTCP_Status(TCP_TCB* pstTCB);

BOOL kTCP_SendSegment(TCP_HEADER* pstHeader, WORD wHeaderSize, BYTE* pbPayload, WORD wPayloadLen, DWORD dwDestAddress);
WORD kTCP_CalcChecksum(IPv4Pseudo_Header* pstPseudo_Header, TCP_HEADER* pstHeader, BYTE* pbPayload, WORD wPayloadLen);

BOOL kTCP_UpDirectionPoint(FRAME stFrame);
BOOL kTCP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kTCP_GetFrameFromFrameQueue(FRAME* pstFrame);
void kEncapuslationSegment(FRAME* pstFrame, BYTE* pbHeader, DWORD dwHeaderSize,
  BYTE* pbPayload, DWORD dwPayloadSize);
void kDecapuslationSegment(FRAME* pstFrame, BYTE** ppbHeader, BYTE** ppbPayload);

#endif /* __TCP_H__ */