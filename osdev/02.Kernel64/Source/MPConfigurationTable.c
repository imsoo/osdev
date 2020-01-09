#include "MPConfigurationTable.h"
#include "Console.h"


static MPCONFIGRUATIONMANAGER gs_stMPConfigurationManager = { 0, };

/*
  Find MP Floating Header and Return Address
*/
BOOL kFindMPFloatingPointerAddress(QWORD* pstAddress)
{
  char* pcMPFloatingPointer;
  QWORD qwEBDAAddress;
  QWORD qwSystemBaseMemory;

  // Print Extended BIOS Data Area
  kPrintf("Extended BIOS Data Area = [0x%X] \n",
    (DWORD)(*(WORD*)0x040E) * 16);

  // Print System Base Address
  kPrintf("System Base Address = [0x%X]\n",
    (DWORD)(*(WORD*)0x0413) * 1024);

  // Find MP Floating Header in Extended BIOS Data Area(0x040E)
  qwEBDAAddress = *(WORD*)(0x040E);
  qwEBDAAddress *= 16;

  for (pcMPFloatingPointer = (char*)qwEBDAAddress;
    (QWORD)pcMPFloatingPointer <= (qwEBDAAddress + 1024);
    pcMPFloatingPointer++)
  {
    if (kMemCmp(pcMPFloatingPointer, "_MP_", 4) == 0)
    {
      kPrintf("MP Floating Pointer Is In EBDA, [0x%X] Address\n",
        (QWORD)pcMPFloatingPointer);
      *pstAddress = (QWORD)pcMPFloatingPointer;
      return TRUE;
    }
  }

  // Find MP Floating Header in System Base Memory
  // System Base Memory Size (KB) in 0x0413
  qwSystemBaseMemory = *(WORD*)0x0413;
  qwSystemBaseMemory *= 1024;

  for (pcMPFloatingPointer = (char*)(qwSystemBaseMemory - 1024);
    (QWORD)pcMPFloatingPointer <= qwSystemBaseMemory;
    pcMPFloatingPointer++)
  {
    if (kMemCmp(pcMPFloatingPointer, "_MP_", 4) == 0)
    {
      kPrintf("MP Floating Pointer Is In System Base Memory, [0x%X] Address\n",
        (QWORD)pcMPFloatingPointer);
      *pstAddress = (QWORD)pcMPFloatingPointer;
      return TRUE;
    }
  }

  // Find MP Floating Header in BIOS ROM Area
  for (pcMPFloatingPointer = (char*)0x0F0000;
    (QWORD)pcMPFloatingPointer < 0x0FFFFF; pcMPFloatingPointer++)
  {
    if (kMemCmp(pcMPFloatingPointer, "_MP_", 4) == 0)
    {
      kPrintf("MP Floating Pointer Is In ROM, [0x%X] Address\n",
        pcMPFloatingPointer);
      *pstAddress = (QWORD)pcMPFloatingPointer;
      return TRUE;
    }
  }

  return FALSE;
}

/*
  Get Info From MP Configuration Table
*/
BOOL kAnalysisMPConfigurationTable(void)
{
  QWORD qwMPFloatingPointerAddress;
  MPFLOATINGPOINTER* pstMPFloatingPointer;
  MPCONFIGURATIONTABLEHEADER* pstMPConfigurationHeader;
  BYTE bEntryType;
  WORD i;
  QWORD qwEntryAddress;
  PROCESSORENTRY* pstProcessorEntry;
  BUSENTRY* pstBusEntry;

  // Init Manager
  kMemSet(&gs_stMPConfigurationManager, 0, sizeof(MPCONFIGRUATIONMANAGER));
  gs_stMPConfigurationManager.bISABusID = 0xFF;

  // Find MP Floating Pointer
  if (kFindMPFloatingPointerAddress(&qwMPFloatingPointerAddress) == FALSE)
  {
    return FALSE;
  }

  // Set MP Floating Table
  pstMPFloatingPointer = (MPFLOATINGPOINTER*)qwMPFloatingPointerAddress;
  gs_stMPConfigurationManager.pstMPFloatingPointer = pstMPFloatingPointer;
  pstMPConfigurationHeader = (MPCONFIGURATIONTABLEHEADER*)
    ((QWORD)pstMPFloatingPointer->dwMPConfigurationTableAddress & 0xFFFFFFFF);

  // Set PIC Flag
  if (pstMPFloatingPointer->vbMPFeatureByte[1] &
    MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE)
  {
    gs_stMPConfigurationManager.bUsePICMode = TRUE;
  }

  // Set MP configuration Header address and table entry address
  gs_stMPConfigurationManager.pstMPConfigurationTableHeader =
    pstMPConfigurationHeader;
  gs_stMPConfigurationManager.qwBaseEntryStartAddress =
    pstMPFloatingPointer->dwMPConfigurationTableAddress +
    sizeof(MPCONFIGURATIONTABLEHEADER);

  // Search all entry and get Core Count, ISA Bus ID
  qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;
  for (i = 0; i < pstMPConfigurationHeader->wEntryCount; i++)
  {
    bEntryType = *(BYTE*)qwEntryAddress;
    switch (bEntryType)
    {
      // Processor Entry
    case MP_ENTRYTYPE_PROCESSOR:
      pstProcessorEntry = (PROCESSORENTRY*)qwEntryAddress;
      if (pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE)
      {
        gs_stMPConfigurationManager.iProcessorCount++;
      }
      qwEntryAddress += sizeof(PROCESSORENTRY);
      break;

      // Bus Entry
    case MP_ENTRYTYPE_BUS:
      pstBusEntry = (BUSENTRY*)qwEntryAddress;
      // check ISA Bus
      if (kMemCmp(pstBusEntry->vcBusTypeString, MP_BUS_TYPESTRING_ISA,
        kStrLen(MP_BUS_TYPESTRING_ISA)) == 0)
      {
        gs_stMPConfigurationManager.bISABusID = pstBusEntry->bBusID;
      }
      qwEntryAddress += sizeof(BUSENTRY);
      break;

      // ETC
    case MP_ENTRYTYPE_IOAPIC:
    case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
    case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
    default:
      qwEntryAddress += 8;
      break;
    }
  }
  return TRUE;
}


/*
  Get MP Configuration Manager
*/
MPCONFIGRUATIONMANAGER* kGetMPConfigurationManager(void)
{
  return &gs_stMPConfigurationManager;
}

/*
  Print MP Configuration Table 
*/
void kPrintMPConfigurationTable(void)
{
  MPCONFIGRUATIONMANAGER* pstMPConfigurationManager;
  QWORD qwMPFloatingPointerAddress;
  MPFLOATINGPOINTER* pstMPFloatingPointer;
  MPCONFIGURATIONTABLEHEADER* pstMPTableHeader;
  PROCESSORENTRY* pstProcessorEntry;
  BUSENTRY* pstBusEntry;
  IOAPICENTRY* pstIOAPICEntry;
  IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
  LOCALINTERRUPTASSIGNMENTENTRY* pstLocalAssignmentEntry;
  QWORD qwBaseEntryAddress;
  char vcStringBuffer[20];
  WORD i;
  BYTE bEntryType;

  char* vpcInterruptType[4] = { "INT", "NMI", "SMI", "ExtINT" };
  char* vpcInterruptFlagsPO[4] = { "Conform", "Active High",
          "Reserved", "Active Low" };
  char* vpcInterruptFlagsEL[4] = { "Conform", "Edge-Trigger", "Reserved",
          "Level-Trigger" };

  kPrintf("================ MP Configuration Table Summary ================\n");
  pstMPConfigurationManager = kGetMPConfigurationManager();
  if ((pstMPConfigurationManager->qwBaseEntryStartAddress == 0) &&
    (kAnalysisMPConfigurationTable() == FALSE))
  {
    kPrintf("MP Configuration Table Analysis Fail\n");
    return;
  }
  kPrintf("MP Configuration Table Analysis Success\n");

  kPrintf("MP Floating Pointer Address : 0x%Q\n",
    pstMPConfigurationManager->pstMPFloatingPointer);
  kPrintf("PIC Mode Support : %d\n", pstMPConfigurationManager->bUsePICMode);
  kPrintf("MP Configuration Table Header Address : 0x%Q\n",
    pstMPConfigurationManager->pstMPConfigurationTableHeader);
  kPrintf("Base MP Configuration Table Entry Start Address : 0x%Q\n",
    pstMPConfigurationManager->qwBaseEntryStartAddress);
  kPrintf("Processor Count : %d\n", pstMPConfigurationManager->iProcessorCount);
  kPrintf("ISA Bus ID : %d\n", pstMPConfigurationManager->bISABusID);

  kPrintf("Press any key to continue... ('q' is exit) : ");
  if (kGetCh() == 'q')
  {
    kPrintf("\n");
    return;
  }
  kPrintf("\n");

  // MP Floating Pointer
  kPrintf("=================== MP Floating Pointer ===================\n");
  pstMPFloatingPointer = pstMPConfigurationManager->pstMPFloatingPointer;
  kMemCpy(vcStringBuffer, pstMPFloatingPointer->vcSignature, 4);
  vcStringBuffer[4] = '\0';
  kPrintf("Signature : %s\n", vcStringBuffer);
  kPrintf("MP Configuration Table Address : 0x%Q\n",
    pstMPFloatingPointer->dwMPConfigurationTableAddress);
  kPrintf("Length : %d * 16 Byte\n", pstMPFloatingPointer->bLength);
  kPrintf("Version : %d\n", pstMPFloatingPointer->bRevision);
  kPrintf("CheckSum : 0x%X\n", pstMPFloatingPointer->bCheckSum);
  kPrintf("Feature Byte 1 : 0x%X ", pstMPFloatingPointer->vbMPFeatureByte[0]);

  // Use MP Configuration Table
  if (pstMPFloatingPointer->vbMPFeatureByte[0] == 0)
  {
    kPrintf("(Use MP Configuration Table)\n");
  }
  else
  {
    kPrintf("(Use Default Configuration)\n");
  }
  // PIC Mode Support
  kPrintf("Feature Byte 2 : 0x%X ", pstMPFloatingPointer->vbMPFeatureByte[1]);
  if (pstMPFloatingPointer->vbMPFeatureByte[2] &
    MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE)
  {
    kPrintf("(PIC Mode Support)\n");
  }
  else
  {
    kPrintf("(Virtual Wire Mode Support)\n");
  }

  // MP Configuration Table Header
  kPrintf("\n=============== MP Configuration Table Header ===============\n");
  pstMPTableHeader = pstMPConfigurationManager->pstMPConfigurationTableHeader;
  kMemCpy(vcStringBuffer, pstMPTableHeader->vcSignature, 4);
  vcStringBuffer[4] = '\0';
  kPrintf("Signature : %s\n", vcStringBuffer);
  kPrintf("Length : %d Byte\n", pstMPTableHeader->wBaseTableLength);
  kPrintf("Version : %d\n", pstMPTableHeader->bRevision);
  kPrintf("CheckSum : 0x%X\n", pstMPTableHeader->bCheckSum);
  kMemCpy(vcStringBuffer, pstMPTableHeader->vcOEMIDString, 8);
  vcStringBuffer[8] = '\0';
  kPrintf("OEM ID String : %s\n", vcStringBuffer);
  kMemCpy(vcStringBuffer, pstMPTableHeader->vcProductIDString, 12);
  vcStringBuffer[12] = '\0';
  kPrintf("Product ID String : %s\n", vcStringBuffer);
  kPrintf("OEM Table Pointer : 0x%X\n",
    pstMPTableHeader->dwOEMTablePointerAddress);
  kPrintf("OEM Table Size : %d Byte\n", pstMPTableHeader->wOEMTableSize);
  kPrintf("Entry Count : %d\n", pstMPTableHeader->wEntryCount);
  kPrintf("Memory Mapped I/O Address Of Local APIC : 0x%X\n",
    pstMPTableHeader->dwMemoryMapIOAddressOfLocalAPIC);
  kPrintf("Extended Table Length : %d Byte\n",
    pstMPTableHeader->wExtendedTableLength);
  kPrintf("Extended Table Checksum : 0x%X\n",
    pstMPTableHeader->bExtendedTableChecksum);

  kPrintf("Press any key to continue... ('q' is exit) : ");
  if (kGetCh() == 'q')
  {
    kPrintf("\n");
    return;
  }
  kPrintf("\n");

  // Base MP Configuration Table Entry
  kPrintf("\n============= Base MP Configuration Table Entry =============\n");
  qwBaseEntryAddress = pstMPFloatingPointer->dwMPConfigurationTableAddress +
    sizeof(MPCONFIGURATIONTABLEHEADER);
  for (i = 0; i < pstMPTableHeader->wEntryCount; i++)
  {
    bEntryType = *(BYTE*)qwBaseEntryAddress;
    switch (bEntryType)
    {
      // Processor Entry
    case MP_ENTRYTYPE_PROCESSOR:
      pstProcessorEntry = (PROCESSORENTRY*)qwBaseEntryAddress;
      kPrintf("Entry Type : Processor\n");
      kPrintf("Local APIC ID : %d\n", pstProcessorEntry->bLocalAPICID);
      kPrintf("Local APIC Version : 0x%X\n", pstProcessorEntry->bLocalAPICVersion);
      kPrintf("CPU Flags : 0x%X ", pstProcessorEntry->bCPUFlags);
      if (pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE)
      {
        kPrintf("(Enable, ");
      }
      else
      {
        kPrintf("(Disable, ");
      }
      // BSP/AP 
      if (pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_BSP)
      {
        kPrintf("BSP)\n");
      }
      else
      {
        kPrintf("AP)\n");
      }
      kPrintf("CPU Signature : 0x%X\n", pstProcessorEntry->vbCPUSignature);
      kPrintf("Feature Flags : 0x%X\n\n", pstProcessorEntry->dwFeatureFlags);

      // move next entry
      qwBaseEntryAddress += sizeof(PROCESSORENTRY);
      break;

      // Bus entry
    case MP_ENTRYTYPE_BUS:
      pstBusEntry = (BUSENTRY*)qwBaseEntryAddress;
      kPrintf("Entry Type : Bus\n");
      kPrintf("Bus ID : %d\n", pstBusEntry->bBusID);
      kMemCpy(vcStringBuffer, pstBusEntry->vcBusTypeString, 6);
      vcStringBuffer[6] = '\0';
      kPrintf("Bus Type String : %s\n\n", vcStringBuffer);

      qwBaseEntryAddress += sizeof(BUSENTRY);
      break;

      // I/O APIC entry
    case MP_ENTRYTYPE_IOAPIC:
      pstIOAPICEntry = (IOAPICENTRY*)qwBaseEntryAddress;
      kPrintf("Entry Type : I/O APIC\n");
      kPrintf("I/O APIC ID : %d\n", pstIOAPICEntry->bIOAPICID);
      kPrintf("I/O APIC Version : 0x%X\n", pstIOAPICEntry->bIOAPICVersion);
      kPrintf("I/O APIC Flags : 0x%X ", pstIOAPICEntry->bIOAPICFlags);
      if (pstIOAPICEntry->bIOAPICFlags == 1)
      {
        kPrintf("(Enable)\n");
      }
      else
      {
        kPrintf("(Disable)\n");
      }
      kPrintf("Memory Mapped I/O Address : 0x%X\n\n",
        pstIOAPICEntry->dwMemoryMapAddress);

      qwBaseEntryAddress += sizeof(IOAPICENTRY);
      break;

      // I/O Interrupt assignment
    case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
      pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)
        qwBaseEntryAddress;
      kPrintf("Entry Type : I/O Interrupt Assignment\n");
      kPrintf("Interrupt Type : 0x%X ", pstIOAssignmentEntry->bInterruptType);
      kPrintf("(%s)\n", vpcInterruptType[pstIOAssignmentEntry->bInterruptType]);
      kPrintf("I/O Interrupt Flags : 0x%X ", pstIOAssignmentEntry->wInterruptFlags);
      kPrintf("(%s, %s)\n", vpcInterruptFlagsPO[pstIOAssignmentEntry->wInterruptFlags & 0x03],
        vpcInterruptFlagsEL[(pstIOAssignmentEntry->wInterruptFlags >> 2) & 0x03]);
      kPrintf("Source BUS ID : %d\n", pstIOAssignmentEntry->bSourceBUSID);
      kPrintf("Source BUS IRQ : %d\n", pstIOAssignmentEntry->bSourceBUSIRQ);
      kPrintf("Destination I/O APIC ID : %d\n",
        pstIOAssignmentEntry->bDestinationIOAPICID);
      kPrintf("Destination I/O APIC INTIN : %d\n\n",
        pstIOAssignmentEntry->bDestinationIOAPICINTIN);

      qwBaseEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
      break;

      // Local Interrupt assignment entry
    case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
      pstLocalAssignmentEntry = (LOCALINTERRUPTASSIGNMENTENTRY*)
        qwBaseEntryAddress;
      kPrintf("Entry Type : Local Interrupt Assignment\n");
      kPrintf("Interrupt Type : 0x%X ", pstLocalAssignmentEntry->bInterruptType);
      kPrintf("(%s)\n", vpcInterruptType[pstLocalAssignmentEntry->bInterruptType]);
      kPrintf("I/O Interrupt Flags : 0x%X ", pstLocalAssignmentEntry->wInterruptFlags);
      kPrintf("(%s, %s)\n", vpcInterruptFlagsPO[pstLocalAssignmentEntry->wInterruptFlags & 0x03],
        vpcInterruptFlagsEL[(pstLocalAssignmentEntry->wInterruptFlags >> 2) & 0x03]);
      kPrintf("Source BUS ID : %d\n", pstLocalAssignmentEntry->bSourceBUSID);
      kPrintf("Source BUS IRQ : %d\n", pstLocalAssignmentEntry->bSourceBUSIRQ);
      kPrintf("Destination Local APIC ID : %d\n",
        pstLocalAssignmentEntry->bDestinationLocalAPICID);
      kPrintf("Destination Local APIC LINTIN : %d\n\n",
        pstLocalAssignmentEntry->bDestinationLocalAPICLINTIN);

      qwBaseEntryAddress += sizeof(LOCALINTERRUPTASSIGNMENTENTRY);
      break;

    default:
      kPrintf("Unknown Entry Type. %d\n", bEntryType);
      break;
    }

    // More 
    if ((i != 0) && (((i + 1) % 3) == 0))
    {
      kPrintf("Press any key to continue... ('q' is exit) : ");
      if (kGetCh() == 'q')
      {
        kPrintf("\n");
        return;
      }
      kPrintf("\n");
    }
  }
}

/*
  Get Core Count
*/
int kGetProcessorCount(void)
{
  // if MP Config table is not exist, set 1 (Single Core)
  if (gs_stMPConfigurationManager.iProcessorCount == 0)
  {
    return 1;
  }
  return gs_stMPConfigurationManager.iProcessorCount;
}

