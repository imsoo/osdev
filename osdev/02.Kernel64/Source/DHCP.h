#ifndef __DHCP_H__
#define __DHCP_H__

#include "Synchronization.h"
#include "Frame.h"
#include "Queue.h"

#define DHCP_MESSAGETYPE_BOOTREQUEST  0x01
#define DHCP_MESSAGETYPE_BOOTREPLY    0x02

#define DHCP_HARDWARETYPE_ETHERNET    0x01

#define DHCP_HARDWARELEN_ETHERNET     0x06

#define DHCP_HOPS_DEFAULT 0x00
#define DHCP_FLAGS_BROADCAST_SHIFT 15
#define DHCP_MAGICCOOKIE  0x63825363

#define BOOTP_VENDOR_EXTENSIONS_CODE_SUBNETMASK 1
#define BOOTP_VENDOR_EXTENSIONS_CODE_ROUTER 3
#define BOOTP_VENDOR_EXTENSIONS_CODE_DOMAINNAMESERVER 6
#define BOOTP_VENDOR_EXTENSIONS_CODE_DOMAINNAME 15
#define BOOTP_VENDOR_EXTENSIONS_CODE_END 255

#define DHCP_PARAMETER_REQUEST_LIST_SIZE  4

// DHCP Option
#define DHCP_OPTION_CODE_SUBNETMASK 0x01
#define DHCP_OPTION_CODE_ROUTER 0x03
#define DHCP_OPTION_CODE_DOMAINNAMESERVER 0x06

#define DHCP_OPTION_CODE_REQUESTEDIPADDRESS 0x32
#define DHCP_OPTION_CODE_IPADDRESSLEASETIME 0x33
#define DHCP_OPTION_CODE_DHCPMESSAGETYPE  0x35
#define DHCP_OPTION_CODE_DHCPSERVERIDENTIFIER  0x36
#define DHCP_OPTION_CODE_PARAMETERREQUESTLIST  0x37
#define DHCP_OPTION_CODE_CLIENTIDENTIFIER 0x3D

#define DHCP_OPTION_CODE_ENDMARK  0xFF

#define DHCP_OPTION_VALUE_DHCPDISCOVER  0x01
#define DHCP_OPTION_VALUE_DHCPOFFER     0x02
#define DHCP_OPTION_VALUE_DHCPREQUEST     0x03
#define DHCP_OPTION_VALUE_DHCPDECLINE     0x04
#define DHCP_OPTION_VALUE_DHCPACK     0x05
#define DHCP_OPTION_VALUE_DHCPNACK     0x06

typedef enum kDHCPState
{
  DHCP_INIT = 0,
  DHCP_SELECTING = 1,
  DHCP_REQUESTING = 2,
  DHCP_BOUND = 3,
  DHCP_RENEWING = 4,
  DHCP_REBINDING = 5
} DHCP_STATE;

#pragma pack(push, 1)

typedef struct kDHCPManager
{
  DHCP_STATE eState;
  DWORD dwTransactionID;
  QWORD qwTime;

  BYTE vbServerIPAddress[4];
  BYTE vbMyIPAddress[4];
  BYTE vbGateWayIPAddress[4];
  BYTE vbDNSIPAddress[4];
  BYTE vbSubnetMask[4];

  // µø±‚»≠ ∞¥√º
  MUTEX stLock;

  QUEUE stFrameQueue;
  FRAME* pstFrameBuffer;

  UpFunction pfUp;
  DownFunction pfDownUDP;

} DHCPMANAGER;

typedef struct kDHCPHeader
{
  BYTE bMessageType;
  BYTE bHardwareType;
  BYTE bHardwareAddressLength;
  BYTE bHops;
  DWORD dwTransactionID;
  WORD wSecs;
  WORD wFlags;
  BYTE vbClientIPAddress[4];
  BYTE vbYourIPAddress[4];
  BYTE vbServerIPAddress[4];
  BYTE vbGatewayIPAddress[4];
  BYTE vbClientHardwareAddress[16];
  BYTE vbHostName[64];
  BYTE vbFileName[128];
  DWORD dwMagicCookie;
  BYTE vbOptions[308];
} DHCP_Header;

typedef struct kDHCPOPtion
{
  BYTE bCode;
  BYTE bLength;
  union 
  {
    BYTE bValue;
    BYTE* pbValue;
  };
} DHCP_Option;

#pragma pack(pop)

void kDHCP_Task(void);
BOOL kDHCP_Initialize(void);

BOOL kDHCP_UpDirectionPoint(FRAME stFrame);

BOOL kDHCP_PutFrameToFrameQueue(const FRAME* pstFrame);
BOOL kDHCP_GetFrameFromFrameQueue(FRAME* pstFrame);

void kDHCP_SendMessage(BYTE bMessageType, BYTE bRenewFlag);
BYTE kDHCP_AddOption(DHCP_Header* pstHeader, DHCP_Option* pstOption, WORD wIndex);
BOOL kDHCP_FindOption(DHCP_Header* pstHeader, DHCP_Option* pstOption, BYTE bCode);
BOOL kDHCP_CheckDHCPMessage(DHCP_Header* pstHeader, BYTE bValue);

DWORD kDHCP_GetLeaseTime(BYTE* pbTime);
void kDHCP_ShowState(void);

#endif /* __DHCP_H__ */