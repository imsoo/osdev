#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include "Types.h"
#include "Synchronization.h"
#include "Queue.h"
#include "Frame.h"

#define VEN_INTEL       0x8086   // Vendor ID for Intel 
#define DEV_E1000_DEV   0x100E   // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
#define E1000_I217      0x153A   // Device ID for Intel I217
#define E1000_82577LM   0x10EA   // Device ID for Intel 82577LM

#define ETHERNET_HEADER_TYPE_ARP 0x0806
#define ETHERNET_HEADER_TYPE_IP  0x0800


typedef enum kHandlerStatus
{
  HANDLER_LSC = 0,
  HANDLER_RXDMT = 1,
  HANDLER_RXT0 = 2,
  HANDLER_UNKNOWN = 0xFF
} HANDLERSTATUS;

typedef BOOL(*InitFunction)(QWORD qwMMIOAddress, WORD wIOAddress);
typedef BOOL(*SendFunction)(const void* pvData, WORD wLen);
typedef BOOL(*ReceiveFunction)(FRAME* pstFrame);

typedef HANDLERSTATUS(*HandlerFunction)(void);
typedef BOOL(*GetAddressFunction)(void*);
typedef void(*LinkUpFunction)(void);

#pragma pack(push, 1)

typedef struct kEthernetManager
{
  // µø±‚»≠ ∞¥√º
  SPINLOCK stSpinLock;
  
  InitFunction pfInit;
  SendFunction pfSend;
  ReceiveFunction pfRecevie;

  HandlerFunction pfHandler;
  GetAddressFunction pfGetAddress;
  LinkUpFunction pfLinkUp;

  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUp;
  SideOutFunction pfSideOut;
} ETHERNETMANAGER;

typedef struct kEthernetHeader {
  BYTE vbDestinationMACAddress[6];
  BYTE vbSourceMACAddress[6];
  WORD wType;
} ETHERNET_HEADER;
#pragma pack(pop)

// function
void kEthernet_Task(void);
BOOL kEthernet_DownDirectionPoint(FRAME stFrame);
BOOL kEthernet_SideDirectionPoint(FRAME stFrame);

BOOL kEthernet_Initialize(void);
BOOL kEthernet_SetDriver(WORD wVendor, WORD wDevice);
void kEthernet_Handler(void);

BOOL kEthernet_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kEthernet_GetFrameFromFrameQueue(FRAME* pstFrame);

BOOL kEthernet_GetMACAddress(BYTE* pbAddress);



#endif // !__ETHERNET_H__
