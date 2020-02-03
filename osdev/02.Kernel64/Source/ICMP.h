#ifndef __ICMP_H__
#define __ICMP_H__

#include "IP.h"
#include "Frame.h"
#include "Queue.h"
#include "Synchronization.h"

#define ICMP_TYPE_ECHOREPLY               0x00
#define ICMP_TYPE_DESTINATIONUNREACHABLE  0x03
#define ICMP_TYPE_SOURECEQUENCE           0x04
#define ICMP_TYPE_REDIRECT                0x05
#define ICMP_TYPE_ECHO                    0x08
#define ICMP_TYPE_ROUTERADVERTISMENT      0x09
#define ICMP_TYPE_ROUTERSOLICITATION      0x0A
#define ICMP_TYPE_TIMEEXCEEDED            0x0B
#define ICMP_TYPE_PARAMETERPROBLEM        0x0C
#define ICMP_TYPE_TIMESTAMP               0x0D
#define ICMP_TYPE_TIMESTAMPREPLY         0x0E
#define ICMP_TYPE_INFROMATIONREQUEST      0x0F
#define ICMP_TYPE_INFORMATIONREPLY       0x10

// Destination Unreachable Message
#define ICMP_CODE_NETUNREACHABLE              0x00
#define ICMP_CODE_HOSTUNREACHABLE             0x01
#define ICMP_CODE_PROTOCOLUNREACHABLE         0x02
#define ICMP_CODE_PORTUNREACHABLE             0x03
#define ICMP_CODE_FRAGMENTATIONNEEDEDANDDFSET 0x04
#define ICMP_CODE_SOURCEROUTEFAILED           0x05

// Time Exceeded Message
#define ICMP_CODE_TIMETOLIVEEXCEEDEDINTRANSIT     0x00
#define ICMP_CODE_FRAGMENTREASSEMBLYTIMEEXCEEDED  0x01

#define ICMP_CODE_DEFAULT 0x00
#define ICMP_SEQUENCENUMBER_DEFAULT 0x00

#pragma pack(push, 1)

typedef struct kICMPManager
{
  // µø±‚»≠ ∞¥√º
  MUTEX stLock;

  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  SideOutFunction pfSideOut;

  WORD wIdentifier;
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
    DWORD bPointer;
    ICMP_RR_HEADER stRRHeader;
  };
} ICMP_HEADER;

#pragma pack(pop)

// function
void kICMP_Task(void);
BOOL kICMP_Initialize(void);
BOOL kICMP_SideInPoint(FRAME stFrame);

void kICMP_SendEcho(DWORD dwDestinationAddress);
void kICMP_SendMessage(DWORD dwDestinationAddress, BYTE bType, BYTE bCode, IP_HEADER* pstIP_Header, BYTE* pbDatagram);
WORD kICMP_CalcChecksum(ICMP_HEADER* pstHeader, void* pvData, WORD wDataLen);

BOOL kICMP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kICMP_GetFrameFromFrameQueue(FRAME* pstFrame);

#endif // !__ICMP_H__
