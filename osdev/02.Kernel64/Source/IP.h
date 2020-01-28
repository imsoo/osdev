#ifndef __IP_H__
#define __IP_H__

#include "Types.h"
#include "Queue.h"
#include "Frame.h"

#define IP_VERSION_IPV4   0x04
#define IP_VERSION_SHIFT  4

#define IP_PROTOCOL_ICMP  0x01
#define IP_PROTOCOL_TCP   0x06
#define IP_PROTOCOL_UDP   0x17


#pragma pack(push, 1)

typedef struct kIPManager
{
  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUp;
  DownFunction pfDown;
  SideOutFunction pfSideOut;

  BYTE vbIPAddress[4];
  BYTE vbGatewayAddress[4];
  WORD wIdentification;
} IPMANAGER;

typedef struct kIPHeader {
  BYTE bVersionAndIHL;
  BYTE bDSCPAndECN;
  WORD wTotalLength;
  WORD wIdentification;
  WORD wFlagsAndFragmentOffset;
  BYTE bTimeToLive;
  BYTE bProtocol;
  WORD wHeaderChecksum;
  BYTE vbSourceIPAddress[4];
  BYTE vbDestinationIPAddress[4];
} IP_HEADER;

#pragma pack(pop)

// function
void kIP_Task(void);
BOOL kIP_UpDirectionPoint(FRAME stFrame);
BOOL kIP_SideInPoint(FRAME stFrame);

static BOOL kIP_Initialize(void);
WORD kIP_CalcChecksum(IP_HEADER* pstHeader);


#endif // !__IP_H__
