#ifndef __ICMP_H__
#define __ICMP_H__

#include "Frame.h"
#include "Queue.h"

#define ICMP_TYPE_ECHOREPLY               0x00
#define ICMP_TYPE_DESTINATIONUNREACHABLE  0x03
#define ICMP_TYPE_SOURECEQUENCE           0x04
#define ICMP_TYPE_REDIRECT                0x05
#define ICMP_TYPE_ECHO                    0x08
#define ICMP_TYPE_TIMEEXCEEDED            0x11
#define ICMP_TYPE_PARAMETERPROBLEM        0x12
#define ICMP_TYPE_TIMESTAMP               0x13
#define ICMP_TYPE_TIMESTAMP_REPLY         0x14
#define ICMP_TYPE_INFROMATIONREQUEST      0x15
#define ICMP_TYPE_INFORMATION_REPLY       0x16

#define ICMP_CODE_DEFAULT 0x00

#pragma pack(push, 1)

typedef struct kICMPManager
{
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  SideOutFunction pfSideOut;
} ICMPMANAGER;

typedef struct kICMPRequestReplyHeader {
  WORD wIdentifier;
  WORD wSequenceNumber;
} ICMP_RR_HEADER;;

typedef struct kICMPHeader {
  BYTE bType;
  BYTE bCode;
  WORD wChecksum;
  union 
  {
    DWORD dwGatewayInternetAddress;
    BYTE bPointer;
    ICMP_RR_HEADER stRRHeader;
  };
} ICMP_HEADER;

#pragma pack(pop)

// function
void kICMP_Task(void);
BOOL kICMP_Initialize(void);
BOOL kICMP_SideInPoint(FRAME stFrame);

void kICMP_SendEchoTest(void);
WORD kICMP_CalcChecksum(ICMP_HEADER* pstHeader, void* pvData, WORD wDataLen);

#endif // !__ICMP_H__
