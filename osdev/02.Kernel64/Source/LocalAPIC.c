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