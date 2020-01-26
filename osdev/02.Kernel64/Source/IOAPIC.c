#include "IOAPIC.h"
#include "MPConfigurationTable.h"
#include "PIC.h"

static IOAPICMANAGER gs_stIOAPICManager;

/*
  Get IOAPIC Base Address
*/
QWORD kGetIOAPICBaseAddressOfISA(void)
{
  MPCONFIGRUATIONMANAGER* pstManager;
  IOAPICENTRY* pstIOAPIEntry;

  // Get IOAPIC Base Address
  if (gs_stIOAPICManager.qwIOAPICBaseAddressOfISA == NULL) {
    pstIOAPIEntry = kFindIOAPICEntryForISA();
    if (pstIOAPIEntry != NULL) {
      gs_stIOAPICManager.qwIOAPICBaseAddressOfISA =
        pstIOAPIEntry->dwMemoryMapAddress & 0xFFFFFFFF;
    }
  }

  return gs_stIOAPICManager.qwIOAPICBaseAddressOfISA;
}

/*
  Set IOAPIC Redirection Table Entry
*/
void kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID,
  BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector)
{
  kMemSet(pstEntry, 0, sizeof(IOREDIRECTIONTABLE));

  pstEntry->bDestination = bAPICID;
  pstEntry->bFlagsAndDeliveryMode = bFlagsAndDeliveryMode;
  pstEntry->bInterruptMask = bInterruptMask;
  pstEntry->bVector = bVector;
}

/*
  Read IOAPIC Redirection Table entry
*/
void kReadIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry)
{
  QWORD* pqwData;
  QWORD qwIOAPICBaseAddress;

  qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();

  pqwData = (QWORD*)pstEntry;

  // Read High IO Redirection table Register
  *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
    IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
  *pqwData = *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
  *pqwData = *pqwData << 32;

  // Read Low IO Redirection table Register
  *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
    IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2;
  *pqwData |= *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
}

/*
  Write IOAPIC Redirection Table entry
*/
void kWriteIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry)
{
  QWORD* pqwData;
  QWORD qwIOAPICBaseAddress;

  qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();

  pqwData = (QWORD*)pstEntry;

  // Write High IO Redirection table Register
  *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
    IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;
  *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData >> 32;

  // Write Low IO Redirection table Register
  *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
    IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2;
  *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData;
}

/*
  Set All IOAPIC redirection Table entry's Interrupt Mask
*/
void kMaskAllInterruptInIOAPIC(void)
{
  IOREDIRECTIONTABLE stEntry;
  int i;

  for (i = 0; i < IOAPIC_MAXIOREDIRECTIONTABLECOUNT; i++) {
    kReadIOAPICRedirectionTable(i, &stEntry);
    stEntry.bInterruptMask = IOAPIC_INTERRUPT_MASK;
    kWriteIOAPICRedirectionTable(i, &stEntry);
  }
}

/*
  Init IOAPIC Redirection Table
*/
void kInitializeIORedirectionTable(void)
{
  MPCONFIGRUATIONMANAGER* pstMPManager;
  MPCONFIGURATIONTABLEHEADER* pstMPHeader;
  IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
  IOREDIRECTIONTABLE stIORedirectionEntry;
  QWORD qwEntryAddress;
  BYTE bEntryType;
  BYTE bDestination;
  int i;

  // IOAPIC Manager Init
  kMemSet(&gs_stIOAPICManager, 0, sizeof(gs_stIOAPICManager));
  kGetIOAPICBaseAddressOfISA();

  // Map Table Init
  for (i = 0; i < IOAPIC_MAXIRQTOINTINMAPCOUNT; i++) {
    gs_stIOAPICManager.vbIRQToINTINMap[i] = 0xFF;
  }

  kMaskAllInterruptInIOAPIC();
  // Redirection Table Init
  pstMPManager = kGetMPConfigurationManager();
  pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
  qwEntryAddress = pstMPManager->qwBaseEntryStartAddress;

  for (i = 0; i < pstMPHeader->wEntryCount; i++) {
    bEntryType = *(BYTE*)qwEntryAddress;
    switch (bEntryType)
    {
    case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
      pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwEntryAddress;

      
      // PCI Test Temp
      // TODO : 이더넷 인터럽트 설정 루틴 개선
      if ((pstIOAssignmentEntry->bSourceBUSID == 0) &&
        (pstIOAssignmentEntry->bInterruptType == MP_INTERRUPTTYPE_INT)) {

        if (pstIOAssignmentEntry->bSourceBUSIRQ == 12) {
          bDestination = 0x00;

          // Set Table Entry
          kSetIOAPICRedirectionEntry(&stIORedirectionEntry, bDestination, 0x00,
            IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH | IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED,
            PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->bSourceBUSIRQ - 1);
          kWriteIOAPICRedirectionTable(pstIOAssignmentEntry->bDestinationIOAPICINTIN, &stIORedirectionEntry);

          // Set IRQ <-> INITI Map Table 
          gs_stIOAPICManager.vbIRQToINTINMap[pstIOAssignmentEntry->bSourceBUSIRQ - 1] = pstIOAssignmentEntry->bDestinationIOAPICINTIN;
        }
      }
      

      // Check Source Bus ID, Interrupt Type
      if ((pstIOAssignmentEntry->bSourceBUSID == pstMPManager->bISABusID) &&
        (pstIOAssignmentEntry->bInterruptType == MP_INTERRUPTTYPE_INT)) {

        // if IRQ 0, set 0x00 (BSP)
        if (pstIOAssignmentEntry->bSourceBUSIRQ == 0) {
          bDestination = 0xFF;
        }
        // else, set 0xFF (ALL Processor)
        else {
          bDestination = 0x00;
        }

        // Set Table Entry
        kSetIOAPICRedirectionEntry(&stIORedirectionEntry, bDestination, 0x00,
          IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH | IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED,
          PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->bSourceBUSIRQ);
        kWriteIOAPICRedirectionTable(pstIOAssignmentEntry->bDestinationIOAPICINTIN, &stIORedirectionEntry);

        // Set IRQ <-> INITI Map Table 
        gs_stIOAPICManager.vbIRQToINTINMap[pstIOAssignmentEntry->bSourceBUSIRQ] = pstIOAssignmentEntry->bDestinationIOAPICINTIN;
      }
      qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
      break;
    case MP_ENTRYTYPE_PROCESSOR:
      qwEntryAddress += sizeof(PROCESSORENTRY);
      break;
    case MP_ENTRYTYPE_BUS:
    case MP_ENTRYTYPE_IOAPIC:
    case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
      qwEntryAddress += 8;
      break;
    }
  }
}

/*
  Print IRQ <-> INTI Map
*/
void kPrintIRQToINTINMap(void)
{
  int i;
  kPrintf("=========== IRQ To I/O APIC INT IN Mapping Table ===========\n");

  for (i = 0; i < IOAPIC_MAXIRQTOINTINMAPCOUNT; i++) {
    kPrintf("IRQ[%d] -> INTIN[%d]\n", i, gs_stIOAPICManager.vbIRQToINTINMap[i]);
  }
}

/*
  Change I/O Redirection Table (IRQ <-> Local APIC ID);
*/
void kRoutingIRQToAPICID(int iIRQ, BYTE bAPICID)
{
  int i;
  IOREDIRECTIONTABLE stEntry;

  if (iIRQ > IOAPIC_MAXIOREDIRECTIONTABLECOUNT) {
    return;
  }

  // Change Table;
  kReadIOAPICRedirectionTable(gs_stIOAPICManager.vbIRQToINTINMap[iIRQ], &stEntry);
  stEntry.bDestination = bAPICID;
  kWriteIOAPICRedirectionTable(gs_stIOAPICManager.vbIRQToINTINMap[iIRQ], &stEntry);
}