#include "Page.h"
#include "../../02.Kernel64/Source/Task.h"

#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + 0x1fffff) & 0xffffffffffe00000)

/*
  Init Page Table for IA-32e mode kernel
*/
void kInitializePageTables(void)
{
  PML4TENTRY* pstPML4TEntry;
  PDPTENTRY* pstPDPTEntry;
  PDENTRY* pstPDEntry;
  DWORD dwMappingAddress;
  DWORD dwPageEntryFlags;
  int i;

  // create PML4 table
  pstPML4TEntry = (PML4TENTRY*)0x100000;
  kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  for (i = 1; i < PAGE_MAXENTRYCOUNT; i++) {
    kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0, 0);
  }

  // create PDP table
  pstPDPTEntry = (PDPTENTRY*)0x101000;
  for (i = 0; i < 64; i++) {
    kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0x102000 + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
  }
  for (i = 64; i < PAGE_MAXENTRYCOUNT; i++) {
    kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0, 0);
  }

  // create PD table
  pstPDEntry = (PDENTRY*)0x102000;
  dwMappingAddress = 0;
  for (i = 0; i < PAGE_MAXENTRYCOUNT * 64; i++) {
    
    // Kernel Page
    if (i < ((DWORD)DYNAMICMEMORY_START_ADDRESS / PAGE_DEFAULTSIZE)) {
      dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;
    }
    // User Page
    else {
      dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS | PAGE_FLAGS_US;
    }

    kSetPageEntryData(&(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12, dwMappingAddress, dwPageEntryFlags, 0);
    dwMappingAddress += PAGE_DEFAULTSIZE;
  }
}

void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
  pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
  pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}