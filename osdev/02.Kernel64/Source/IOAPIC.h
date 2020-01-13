#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include "Types.h"

// I/O APIC Register Offset (Memory MAP : 0xFEC00000)
#define IOAPIC_REGISTER_IOREGISTERSELECTOR              0x00
#define IOAPIC_REGISTER_IOWINDOW                        0x10

// I/O APIC Register Index
#define IOAPIC_REGISTERINDEX_IOAPICID                   0x00
#define IOAPIC_REGISTERINDEX_IOAPICVERSION              0x01
#define IOAPIC_REGISTERINDEX_IOAPICARBID                0x02
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE      0x10
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE     0x11

// IO Redirection table count
#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT               24

// Interrupt Mask
#define IOAPIC_INTERRUPT_MASK                           0x01

// Trigger Mode
#define IOAPIC_TRIGGERMODE_LEVEL                        0x80
#define IOAPIC_TRIGGERMODE_EDGE                         0x00

// Remote IRR
#define IOAPIC_REMOTEIRR_EOI                            0x40
#define IOAPIC_REMOTEIRR_ACCEPT                         0x00

// Interrupt Input Pin Polarity
#define IOAPIC_POLARITY_ACTIVELOW                       0x20
#define IOAPIC_POLARITY_ACTIVEHIGH                      0x00

// Delivery Status
#define IOAPIC_DELIFVERYSTATUS_SENDPENDING              0x10
#define IOAPIC_DELIFVERYSTATUS_IDLE                     0x00

// Destination Mode
#define IOAPIC_DESTINATIONMODE_LOGICALMODE              0x08
#define IOAPIC_DESTINATIONMODE_PHYSICALMODE             0x00

// Delivery Mode
#define IOAPIC_DELIVERYMODE_FIXED                       0x00
#define IOAPIC_DELIVERYMODE_LOWESTPRIORITY              0x01
#define IOAPIC_DELIVERYMODE_SMI                         0x02
#define IOAPIC_DELIVERYMODE_NMI                         0x04
#define IOAPIC_DELIVERYMODE_INIT                        0x05
#define IOAPIC_DELIVERYMODE_EXTINT                      0x07

// IRQ(0 ~ 15) to INTIN (I/O APIC) Count
#define IOAPIC_MAXIRQTOINTINMAPCOUNT                    16

#pragma pack( push, 1 )

// IO Redirection Table 
typedef struct kIORedirectionTableStruct
{
  // Interrupt Vector
  BYTE bVector;


  // Delivery Mode (3), Destination Mode (1), 
  // Status(1), Polarity(1), Remote IRR(1), Trigger(1)
  BYTE bFlagsAndDeliveryMode;

  // Interrupt Mask
  BYTE bInterruptMask;

  // RESERVED
  BYTE vbReserved[4];

  // Destination APIC ID
  BYTE bDestination;
} IOREDIRECTIONTABLE;

#pragma pack( pop )

// I/O APIC Manager
typedef struct kIOAPICManagerStruct
{
  // I/O APIC Memory Map Address
  QWORD qwIOAPICBaseAddressOfISA;

  // IRQ <-> I/O APIC INTIN Pin Map Table
  BYTE vbIRQToINTINMap[IOAPIC_MAXIRQTOINTINMAPCOUNT];
} IOAPICMANAGER;

// function
QWORD kGetIOAPICBaseAddressOfISA(void);
void kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID,
  BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector);
void kReadIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry);
void kWriteIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry);
void kMaskAllInterruptInIOAPIC(void);
void kInitializeIORedirectionTable(void);
void kPrintIRQToINTINMap(void);
void kRoutingIRQToAPICID(int iIRQ, BYTE bAPICID);

#endif // !__IOAPIC_H__
