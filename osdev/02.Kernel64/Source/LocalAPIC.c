#include "LocalAPIC.h"
#include "MPConfigurationTable.h"

/*
  Get Local APIC Memory Map Address
*/
QWORD kGetLocalAPICBaseAddress(void)
{
  MPCONFIGURATIONTABLEHEADER* pstMPHeader;

  pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
  return pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
}

/*
  Set Spurious Interrupt Register's APIC Software Enable Bit (8)
*/
void kEnableSoftwareLocalAPIC(void)
{
  QWORD qwLocalAPICBaseAddress;

  qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

  // Set Spurious Interrupt Register's APIC Software Enable Bit (8)
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_SVR) |= 0x100;
}

/*
  Send EOI To Local APIC, finally EOI send to I/O APIC
*/
void kSendEOIToLocalAPIC(void)
{
  QWORD qwLocalAPICBaseAddress;
  qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_EOI) = 0;
}

/*
  Set TPR (Task Priority Register)
*/
void kSetTaskPriority(BYTE bPriority)
{
  QWORD qwLocalAPICBaseAddress;
  qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_TASKPRIORITY) = bPriority;
}

/*
  Initialize Local Vector Table
*/
void kInitializeLocalVectorTable(void)
{
  QWORD qwLocalAPICBaseAddress;
  DWORD dwTempValue;

  qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

  // Set Mask LVT Timer Register (Do not Interrupt)
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_TIMER) |= APIC_INTERRUPT_MASK;

  // Set Mask LVT LINT0 Register (Do not Interrupt)
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_LINT0) |= APIC_INTERRUPT_MASK;

  // Set NMI LVT LINT1 Register
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_LINT1) = APIC_TRIGGERMODE_EDGE |
    APIC_POLARITY_ACTIVEHIGH | APIC_DELIVERYMODE_NMI;

  // Set Mask LVT Error Register (Do not Interrupt)
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ERROR) |= APIC_INTERRUPT_MASK;

  // Set Mask Performance Monitor Counter Register (Do not Interrupt)
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_PERFORMANCEMONITORINGCOUNTER) |= APIC_INTERRUPT_MASK;
  
  // Set Mask LVT Themal Sensor Tegister (Do not Interrupt)
  *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_THERMALSENSOR) |= APIC_INTERRUPT_MASK;
}