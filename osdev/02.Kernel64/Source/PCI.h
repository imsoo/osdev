#ifndef __PCI_H__
#define __PCI_H__

#include "Types.h"

#define PCI_MAX_BUSCOUNT         256
#define PCI_MAX_DEVICECOUNT      32

// I/O Port
#define PCI_CONFIG_ADDRESS        0xCF8
#define PCI_CONFIG_DATA           0xCFC

// Configuration Space registers Offset
#define PCI_VENDORID_OFFSET       0x00
#define PCI_DEVICEID_OFFSET       0x02
#define PCI_COMMAND_OFFSET        0x04
#define PCI_STATUS_OFFSET         0x06
#define PCI_REVISIONID_OFFSET     0x08
#define PCI_CLASS_OFFSET          0x0A
#define PCI_CACHELINESIZE_OFFSET  0x0C
#define PCI_HEADERTYPE_OFFSET     0x0E
#define PCI_BAR0_OFFSET           0x10
#define PCI_BAR1_OFFSET           0x14
#define PCI_BAR2_OFFSET           0x18

#define PCI_HEADER_GENERALDEVICE  0x00
#define PCI_HEADER_PCITOPCIBRIDGE 0x01
#define PIC_HEADER_CARDBUSBRIDGE  0x02

#define PCI_INVALID_VENDORID  0xFFFF
#define PCI_NETWORK_CLASSCODE 0x0200  // Class Code : 0x02, SubClass : 0x00

// function
BOOL kPCI_IsEthernetController(BYTE bBus, BYTE bDevice);

DWORD kPCI_ConfigReadDWord(BYTE bBus, BYTE bDevice, BYTE bFunc, BYTE bOffset);
WORD kPCI_ConfigReadWord(BYTE bBus, BYTE bDevice, BYTE bFunc, BYTE bOffset);

void kPCI_ConfigWriteWord(BYTE bBus, BYTE bDevice, BYTE bFunc, BYTE bOffset, WORD wValue);


#endif // !__PCI_H__
