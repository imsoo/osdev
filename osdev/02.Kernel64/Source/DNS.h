#ifndef __DNS_H__
#define __DNS_H__

#include "Frame.h"
#include "Queue.h"
#include "List.h"
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

#define DNS_CACHE_INDEX_MAX_COUNT 256
#define DNS_CACHE_RETRY_PERIOD 500   // 500ms
#define DNS_CACHE_CLEAN_PERIOD 5000   // 5s

#define DNS_REQUEST_QUEUE_MAX_COUNT 100

typedef enum eDNSRequestReturnCode {
  DNS_NOERROR = 1,
  DNS_ERROR = 2
} DNS_RETURN;

#pragma pack(push, 1)

typedef struct kDNSRequest {
  BYTE* pbName;
  BYTE* pbOut;
  BYTE* pbFlag;
  BYTE bTryCount; // 요청 수행 횟수
  QWORD qwTime;
} DNS_REQUEST;

typedef struct kDNSCacheEntry {
  LISTLINK stEntryLink;
  BYTE vbName[64];
  BYTE vbAddress[4];
  QWORD qwTime;
  DWORD dwTTL;
} DNS_ENTRY;

typedef struct kDNSCache {
  LIST vstEntryList[DNS_CACHE_INDEX_MAX_COUNT];
} DNS_CACHE;

typedef struct kDNSManager
{
  BYTE vbDNSAddress[4];

  DNS_CACHE stCache;
  
  // 요청 처리 큐
  QUEUE stRequestQueue;
  DNS_REQUEST* pstRequestBuffer;

  // 동기화 객체
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
  DWORD dwTTL;
  WORD wRDLength;
  BYTE* pbRData;
} DNS_ANSWERRR;

#pragma pack(pop)

// function
void kDNS_Task(void);
BOOL kDNS_Initialize(void);

// 외부 인터페이스
DNS_RETURN kDNS_GetAddress(const BYTE* pbName, BYTE* pbAddress);

void kDNS_ProcessRequest(void);
void kDNS_SendDNSQuery(const BYTE* pbName);
BOOL kDNS_PutRequest(const DNS_REQUEST* pstRequest);
BOOL kDNS_GetRequest(DNS_REQUEST* pstRequest);

void kDNSCache_Put(DNS_ENTRY* pstEntry);
DNS_ENTRY* kDNSCache_Get(BYTE* pbName);
void kDNSCache_Clean(void);
void kDNSCache_Print(void);

WORD kDNS_ConvertAddressToDotFormat(BYTE* pbIn, BYTE* pbOut);
WORD kDNS_ConvertAddressToDNSFormat(BYTE* pbIn, BYTE* pbOut);

WORD kDNS_AddRR(DNS_QUERYRR* pstRR, BYTE* pbBuf, WORD wIndex);
void kDNS_GetQueryRR(DNS_QUERYRR* pstRR, BYTE* pbPayload);
DNS_ANSWERRR* kDNS_GetAnswerRR(DNS_HEADER* pstHeader, BYTE* pbPayload, WORD wIndex);

BOOL kDNS_SetDNSAddress(BYTE* pbAddress);

BOOL kDNS_UpDirectionPoint(FRAME stFrame);

BOOL kDNS_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kDNS_GetFrameFromFrameQueue(FRAME* pstFrame);

#endif // !__DNS_H__
