#include "PIC.h"

/*
  Init PIC
*/
void kInitializePIC(void)
{
  // init master PIC
  // ICW1
  kOutPortByte(PIC_MASTER_PORT1, 0x11);

  // ICW2
  kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);

  // ICW3 set slave pin 2
  kOutPortByte(PIC_MASTER_PORT2, 0x04);

  // ICW4
  kOutPortByte(PIC_MASTER_PORT2, 0x01);

  // init slave PIC
  // ICW1
  kOutPortByte(PIC_SLAVE_PORT1, 0x11);

  // ICW2
  kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

  // ICW3 set master pin
  kOutPortByte(PIC_SLAVE_PORT2, 0x02);

  // ICW4
  kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

/*
  set Interrupt mast
*/
void kMaskPICInterrupt(WORD wIRQBitmast)
{
  // Master IMR
  kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmast);

  // Slave IMR
  kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmast >> 8));
}

/*
  send EOI (end of interrupt)
*/
void kSendEOIToPIC(int iIRQNumber)
{
  // Master
  kOutPortByte(PIC_MASTER_PORT1, 0x20);

  if (iIRQNumber >= 8)
    kOutPortByte(PIC_SLAVE_PORT1, 0x20);
}