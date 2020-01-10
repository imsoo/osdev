#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "LocalAPIC.h"
#include "PIT.h"

volatile int g_iWakeUpApplicationProcessorCount = 0;
volatile QWORD g_qwAPICIDAddress = 0;

/*
  Enable LocalAPIC and Wake up All AP
*/
BOOL kStartUpApplicationProcessor(void)
{
  // Get MP Configuration Table
  if (kAnalysisMPConfigurationTable() == FALSE) {
    return FALSE;
  }

  // All Processor's Local APIC Enable
  kEnableGlobalLocalAPIC();

  // BSP(Bootstrap Processor) Local APIC Enable
  kEnableSoftwareLocalAPIC();

  // Wake AP(Application Processor)
  if (kWakeUpApplicationProcessor() == FALSE) {
    return FALSE;
  }

  return TRUE;
}

/*
  Read APIC ID from LocalAPIC ID Register
*/
BYTE kGetAPICID(void)
{
  MPCONFIGURATIONTABLEHEADER* pstMPHeader;
  QWORD qwLocalAPICBaeAddress;

  // Get Local APIC Id Register Address
  if (g_qwAPICIDAddress == 0) {
    pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
    if (pstMPHeader == NULL) {
      return 0;
    }
    qwLocalAPICBaeAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
    g_qwAPICIDAddress = qwLocalAPICBaeAddress + APIC_REGISTER_APICID;
  }

  // Retrurn Bit 24 ~ 31
  return *((DWORD*)g_qwAPICIDAddress) >> 24;
}

/*
  Wake Up AP
*/
static BOOL kWakeUpApplicationProcessor(void)
{
  MPCONFIGRUATIONMANAGER* pstMPManager;
  MPCONFIGURATIONTABLEHEADER* pstMPHeader;

  QWORD qwLocalAPICBaseAddress;
  BOOL bInterruptFlag;
  int i;

  bInterruptFlag = kSetInterruptFlag(FALSE);

  pstMPManager = kGetMPConfigurationManager();
  pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
  qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

  g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;

  // Send INIT IPC
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
    APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
    APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;

  // Wait 10ms
  kWaitUsingDirectPIT(MSTOCOUNT(10));

  // Check Success
  if (*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING) {
    kInitializePIT(MSTOCOUNT(1), TRUE);
    kSetInterruptFlag(bInterruptFlag);
    return FALSE;
  }

  // Send StartUp IPC twice | 0x10 = Kernel Entry Address (0x10000)
  for (i = 0; i < 2; i++) {
    *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
      APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
      APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_STARTUP | 0x10;

    // Wait 200us
    kWaitUsingDirectPIT(USTOCOUNT(200));

    // Check Success
    if (*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING) {
      kInitializePIT(MSTOCOUNT(1), TRUE);
      kSetInterruptFlag(bInterruptFlag);
      return FALSE;
    }
  }

  kInitializePIT(MSTOCOUNT(1), TRUE);
  kSetInterruptFlag(bInterruptFlag);

  while (g_iWakeUpApplicationProcessorCount < (pstMPManager->iProcessorCount - 1)) {
    kSleep(50);
  }
  return TRUE;
}