#ifndef __DNS_H__
#define __DNS_H__

#include "Frame.h"
#include "Queue.h"
#include "Synchronization.h"

#define DNS_FLAGS_QUERYRESPOSE_QUERY 0
#define DNS_FLAGS_QUERYRESPOSE_RESPONSE 1

#define DNS_FLAGS_OPCODE_QUERY 0
#define DNS_FLAGS_OPCODE_IQUERY  1
#define DNS_FLAGS_OPCODE_STATUS  2

#define DNS_FLAGS_QUERYRESPOSE_OFFSET           15
#define DNS_FLAGS_OPCODE_OFFSET                 11
#define DNS_FLAGS_AUTHORITATIVEANSWER_OFFSET    10
#define DNS_FLAGS_TRUNCATION_OFFSET             9
#define DNS_FLAGS_RECURSIONDESIRED_OFFSET       8
#define DNS_FLAGS_RECURSIONAVAILABLE_OFFSET     7
#define DNS_FLAGS_ZERO_OFFSET                   4
#define DNS_FLAGS_RESPONSECODE_OFFSET           0

#define DNS_RR_TYPE_A   0x0001
#define DNS_RR_TYPE_NS  0x0002
#define DNS_RR_TYPE_CNAME 0x0005
#define DNS_RR_TYPE_NULL  0x000A

#define DNS_RR_CLASS_IN 0x0001

#define DNS_RR_QUERY 0
#define DNS_RR_ANSWER 1

#pragma pack(push, 1)

typedef struct kDNSManager
{
  BYTE vbDNSAddress[4];

  // µø±‚»≠ ∞¥√º
  MUTEX stLock;

  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  DownFunction pfDownUDP;
} DNSMANAGER;

typedef struct kDNSHeader {
  WORD wTransactionID;
  WORD wFlags;
  WORD wQuestion;
  WORD wAnswer;
  WORD wAuthority;
  WORD wAdditional;
} DNS_HEADER;

typedef struct kDNSQueryResourceRecord {
  BYTE* pbName;
  WORD wType;
  WORD wClass;
} DNS_QUERYRR;

typedef struct kDNSAnswerResourceRecord {
  WORD wName;
  WORD wType;
  WORD wClass;
  DWORD bTTL;
  WORD wRDLength;
  BYTE* pbRData;
} DNS_ANSWERRR;

#pragma pack(pop)

// function
void kDNS_Task(void);
BOOL kDNS_Initialize(void);

void kDNS_SendDNSQuery(const char* pcName);
WORD kDNS_AddRR(DNS_QUERYRR* pstRR, BYTE* pbBuf, WORD wIndex);
WORD kDNS_GetRR(DNS_HEADER* pstHeader, BYTE* pbPayload);

BOOL kDNS_SetDNSAddress(BYTE* pbAddress);

BOOL kDNS_UpDirectionPoint(FRAME stFrame);

BOOL kDNS_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kDNS_GetFrameFromFrameQueue(FRAME* pstFrame);

#endif // !__DNS_H__
