#ifndef __ARP_H__
#define __ARP_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct kARPHeader {
  WORD wHardwareType;
  WORD wProtocolType;
  BYTE bHadrwareAddressLength;
  BYTE bProtocolAddressLength;
  WORD wOperation;
  BYTE vbSenderHardwareAddress[6];
  BYTE vbSenderProtocolAddress[4];
  BYTE vbTargetHardwareAddress[6];
  BYTE vbTargetProtocolAddress[4];
} ARP_HEADER;

#pragma pack(pop)

#endif // !__ARP_H__
