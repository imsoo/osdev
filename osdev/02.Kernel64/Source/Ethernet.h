#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include "Types.h"
#include "Synchronization.h"

#define VEN_INTEL       0x8086   // Vendor ID for Intel 
#define DEV_E1000_DEV   0x100E   // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
#define E1000_I217      0x153A   // Device ID for Intel I217
#define E1000_82577LM   0x10EA   // Device ID for Intel 82577LM

typedef BOOL(*InitFunction)(QWORD qwMMIOAddress, WORD wIOAddress);
typedef BOOL(*SendFunction)(const void* pvData, WORD wLen);
typedef void(*HandlerFunction)(void);
typedef void(*GetAddressFunction)(void*);

#pragma pack(push, 1)

typedef struct kEthernetManager
{
  InitFunction pfInit;
  SendFunction pfSend;
  HandlerFunction pfHandler;
  GetAddressFunction pfGetAddress;
} ETHERNETMANAGER;

typedef struct kEthernetHeader {
  BYTE vbDestinationMACAddress[6];
  BYTE vbSourceMACAddress[6];
  WORD wType;

  // Temp
  BYTE vbPayload[128];
} ETHERNET_HEADER;
#pragma pack(pop)

// function
BOOL kEthernet_Initialize(void);
BOOL kEthernet_SetDriver(WORD wVendor, WORD wDevice);
void kEthernet_Handler(void);

void kEthernet_TS(void);

#endif // !__ETHERNET_H__
