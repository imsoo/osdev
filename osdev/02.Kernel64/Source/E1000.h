#ifndef __E1000_H__
#define __E1000_H__

#include "Types.h"

// Desc
#define DESC_RX_NUM 32
#define DESC_TX_NUM 8

#define REG_EEPROM  0x0014

#define REG_IOADDR  0x00
#define REG_IODATA  0x04

#define REG_CTRL    0x00  // Device Control Register
// --------- CTRL Bit
#define CTRL_SLU			      (1 << 6)  // Set Link Up
// --------- CTRL Bit

#define REG_STATUS  0x08  // Device Status Register
#define REG_RAL0_S  0x5400  // Receive Address Low 0 Shadow
#define REG_RAH0_S  0x5404  // Recevie Address High 0 Shadow
#define REG_ICR     0xC0  // Interrupt Caues Read Register
#define REG_IMS     0xD0  // Interrupt Mask Set/Read Register

#define REG_RCTL  0x0100  // Receive Control Register
// --------- RCTL Bit
#define RCTL_EN				      (1 << 1)  // Enable
#define RCTL_SBP			      (1 << 2)  // Store Bad Packets
#define RCTL_UPE			      (1 << 3)  // Unicast Promiscuous Enable
#define RCTL_MPE			      (1 << 4)  // Multicast Promiscuous Enable
#define RCTL_LPE			      (1 << 5)  // Long Packet Enable
#define RCTL_LBM_NONE       (0 << 6)  // No Loopback
#define RCTL_RDMTS_HALF		  (0 << 8)  // Receive Descriptor Minimum Threshold Size
#define RCTL_RDMTS_QUARTER	(1 << 8)
#define RCTL_RDMTS_EIGHTH		(2 << 8)
#define RCTL_BAM			      (1 << 15) // Boradcast Accept Mode
#define RCTL_BSIZE_256		  (3 << 16) // Receive Buffer Size
#define RCTL_BSIZE_512		  (2 << 16)
#define RCTL_BSIZE_1024		  (1 << 16)
#define RCTL_BSIZE_2048		  (0 << 16)
#define RCTL_BSIZE_4096		  ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192		  ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384	  ((1 << 16) | (1 << 25))
#define RCTL_SECRC			    (1 << 26) // Strip Ethernet CRC from incoming packet
// --------- RCTL Bit

#define REG_RDBAL 0x2800  // Receive Descriptor Base Address Low Qeueu
#define REG_RDBAH 0x2804  // Receive Descriptor Base Address High Queue
#define REG_RDLEN 0x2808  // Receive Descriptor Length Queue
#define REG_RDH   0x2810  // Receive Descriptor Head Queue
#define REG_RDT   0x2818  // Receive Descriptor Tail Queue


#define REG_TCTL  0x0400  // Trasmit Control Register
// --------- TCTL Bit
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision
// --------- TCTL Bit

#define REG_TIPG  0x0410  // Transmit IPG Register
#define REG_TDBAL 0x3800  // Transmit Descriptor Base Address Low
#define REG_TDBAH 0x3804  // Transmit Descriptor Base Address High
#define REG_TDLEN 0x3808  // Transmit Descriptor Length Queue
#define REG_TDH   0x3810  // Transmit Descriptor Head Queue
#define REG_TDT   0x3818  // Transmit Descriptor Tail Queue
#define REG_MTA   0x5200  // Multicast Table Array

// Transmit Command
#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable

#define MTA_ENTRY_COUNT 0x80

#pragma pack(push, 1)

typedef struct kE1000RxDescriptor
{
  volatile QWORD qwAddr;

  volatile WORD wLength;
  volatile WORD wChecksum;

  volatile BYTE bStatus;
  volatile BYTE bError;

  volatile WORD wSpecial
} RXDESCRIPTOR;

typedef struct kE1000TxDescriptor
{
  volatile QWORD qwAddr;

  volatile WORD wLength;

  volatile BYTE bCso;
  volatile BYTE bCmd;
  volatile BYTE bStatus;
  volatile BYTE bCss;

  volatile WORD wSpecial
} TXDESCRIPTOR;

typedef struct kE1000Manager
{
  BOOL bUseMMIO;
  BOOL bEEPROMExists;

  QWORD qwMMIOAddress;
  WORD wIOAddress;

  BYTE vbMacAddress[6];

  RXDESCRIPTOR* vpstRxDescriptor[DESC_RX_NUM];
  volatile WORD	wRxTail;

  TXDESCRIPTOR* vpstTxDescriptor[DESC_TX_NUM];
  volatile WORD	wTxTail;
} E1000MANAGER;

#pragma pack(pop)

// function
BOOL kE1000_Initialize(QWORD qwMMIOAddress, WORD wIOAddress);
BOOL kE1000_Send(const void* pvData, WORD wLen);
void kE1000_Handler(void);
void kE1000_Receive(void);
BOOL kE1000_ReadMACAddress(BYTE* pbBuf);

// TODO : 내부 함수 static으로 변경
BOOL kE1000_RxInit(void);
BOOL kE1000_TxInit(void);
void kE1000_MulticastTableArrayInit(void);
void kE1000_SetLinkUp(void);
void kE1000_EnableInterrupt(void);

static BOOL kE1000_DetectEEPROM(void);
WORD kE1000_EEPROMRead(WORD wAddress);

void kE1000_WriteCommand(WORD wAddress, DWORD dwValue);
DWORD kE1000_ReadCommand(WORD wAddress);

inline void kE1000_WriteDWORD(QWORD wAddress, DWORD dwValue);
inline DWORD kE1000_ReadDWORD(QWORD wAddress);

#endif  /* __E1000_H_ */