#ifndef __MPCONFIGURATIONTABLE__
#define __MPCONFIGURATIONTABLE__

#include "Types.h"

 // MP Floating Pointer Feature Byte
#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE  0x00
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE     0x80

// Entry Type
#define MP_ENTRYTYPE_PROCESSOR                  0
#define MP_ENTRYTYPE_BUS                        1
#define MP_ENTRYTYPE_IOAPIC                     2
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT      3
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT   4

// Processor CPU Flag
#define MP_PROCESSOR_CPUFLAGS_ENABLE            0x01
#define MP_PROCESSOR_CPUFLAGS_BSP               0x02

// Bus Type String
#define MP_BUS_TYPESTRING_ISA                   "ISA"
#define MP_BUS_TYPESTRING_PCI                   "PCI"
#define MP_BUS_TYPESTRING_PCMCIA                "PCMCIA"
#define MP_BUS_TYPESTRING_VESALOCALBUS          "VL"

// Interrupt Type
#define MP_INTERRUPTTYPE_INT                    0
#define MP_INTERRUPTTYPE_NMI                    1
#define MP_INTERRUPTTYPE_SMI                    2
#define MP_INTERRUPTTYPE_EXTINT                 3

// Interrupt Flags
#define MP_INTERRUPT_FLAGS_CONFORMPOLARITY      0x00
#define MP_INTERRUPT_FLAGS_ACTIVEHIGH           0x01
#define MP_INTERRUPT_FLAGS_ACTIVELOW            0x03
#define MP_INTERRUPT_FLAGS_CONFORMTRIGGER       0x00
#define MP_INTERRUPT_FLAGS_EDGETRIGGERED        0x04
#define MP_INTERRUPT_FLAGS_LEVELTRIGGERED       0x0C

#pragma pack( push, 1 )

// MP Floating Pointer Data Structure
typedef struct kMPFloatingPointerStruct
{
  // Signature, _MP_
  char vcSignature[4];

  // MP Configuration Table Address
  DWORD dwMPConfigurationTableAddress;

  // MP Floating Pointer Length 
  BYTE bLength;

  // MultiProcessor Specification Version
  BYTE bRevision;

  // CheckSum
  BYTE bCheckSum;

  // MP Feature Byte 1~5
  BYTE vbMPFeatureByte[5];

} MPFLOATINGPOINTER;

// MP Configuration Table Header
typedef struct kMPConfigurationTableHeaderStruct
{
  // Signature, PCMP
  char vcSignature[4];

  // Table Length (Header + Entry)
  WORD wBaseTableLength;

  // MultiProcessor Specification Version
  BYTE bRevision;

  // CheckSum
  BYTE bCheckSum;

  // OEM ID String 
  char vcOEMIDString[8];

  // PRODUCT ID String
  char vcProductIDString[12];

  // OEM Table Address (if not define, set 0)
  DWORD dwOEMTablePointerAddress;

  // OEM Table Size (if not define, set 0)
  WORD wOEMTableSize;

  // Entry Count
  WORD wEntryCount;

  // Local APIC Memory Map I/O Address
  DWORD dwMemoryMapIOAddressOfLocalAPIC;

  // Extended Table Lnegth
  WORD wExtendedTableLength;

  // Extended Table CheckSum
  BYTE bExtendedTableChecksum;

  // RESERVED
  BYTE bReserved;
} MPCONFIGURATIONTABLEHEADER;

// Processor Entry
typedef struct kProcessorEntryStruct
{
  // Entry type, Processor : 0
  BYTE bEntryType;

  // Local APIC ID
  BYTE bLocalAPICID;

  // Local APIC Version
  BYTE bLocalAPICVersion;

  // CPU Flag
  BYTE bCPUFlags;

  // CPU Signature
  BYTE vbCPUSignature[4];

  // Feature Flag
  DWORD dwFeatureFlags;

  // RESERVED
  DWORD vdwReserved[2];
} PROCESSORENTRY;

// Bus Entry
typedef struct kBusEntryStruct
{
  // Entry type, Bus : 1
  BYTE bEntryType;

  // Bus ID
  BYTE bBusID;

  // Bus type string
  char vcBusTypeString[6];
} BUSENTRY;

// I/O APIC Entry
typedef struct kIOAPICEntryStruct
{
  // Entry type, I/O APIC : 2
  BYTE bEntryType;

  // I/O APIC ID
  BYTE bIOAPICID;

  // I/O APIC Version
  BYTE bIOAPICVersion;

  // I/O APIC Flag
  BYTE bIOAPICFlags;

  // Memory Map I/O Address
  DWORD dwMemoryMapAddress;
} IOAPICENTRY;

// I/O Interrupt Assignment Entry
typedef struct kIOInterruptAssignmentEntryStruct
{
  // Entry type, I/O Interrupt Assignment Entry : 3
  BYTE bEntryType;

  // Interrupt Type
  BYTE bInterruptType;

  // I/O Interrupt Flag
  WORD wInterruptFlags;

  // Source Bus ID
  BYTE bSourceBUSID;

  // Source Bus IRQ
  BYTE bSourceBUSIRQ;

  // Dest I/O APIC ID
  BYTE bDestinationIOAPICID;

  // Dest I/O APIC INTIN
  BYTE bDestinationIOAPICINTIN;
} IOINTERRUPTASSIGNMENTENTRY;

// Local Interrupt Assignment Entry
typedef struct kLocalInterruptEntryStruct
{
  // Entry type, Local Interrupt Assignment Entry : 4
  BYTE bEntryType;

  // Interrupt Type
  BYTE bInterruptType;

  // Local Interrupt flag
  WORD wInterruptFlags;

  // Source Bus ID
  BYTE bSourceBUSID;

  // Source Bus IRQ
  BYTE bSourceBUSIRQ;

  // Dest Local APIC ID
  BYTE bDestinationLocalAPICID;

  // Dest Local APIC INTIN
  BYTE bDestinationLocalAPICLINTIN;

} LOCALINTERRUPTASSIGNMENTENTRY;

#pragma pack( pop)

// MP Configuration Manager
typedef struct kMPConfigurationManagerStruct
{
  // MP Floating Table
  MPFLOATINGPOINTER* pstMPFloatingPointer;

  // MP Configuration Header
  MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader;

  // MP Configuration Table Address
  QWORD qwBaseEntryStartAddress;

  // Core Count
  int iProcessorCount;

  // Use PIC Flag
  BOOL bUsePICMode;

  // ISA Bus ID
  BYTE bISABusID;
} MPCONFIGRUATIONMANAGER;

// function
BOOL kFindMPFloatingPointerAddress(QWORD* pstAddress);
BOOL kAnalysisMPConfigurationTable(void);
MPCONFIGRUATIONMANAGER* kGetMPConfigurationManager(void);
void kPrintMPConfigurationTable(void);
int kGetProcessorCount(void);

#endif /*__MPCONFIGURATIONTABLE__*/
