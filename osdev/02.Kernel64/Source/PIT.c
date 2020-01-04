#include "PIT.h"

/*
  Init PIT
*/
void kInitializePIT(WORD wCount, BOOL bPeriodic)
{
  // Init counter : mode 0, Binary Counter
  kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

  // Set Periodic
  if (bPeriodic == TRUE) {
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
  }

  // Write LSB Count
  kOutPortByte(PIT_PORT_COUNTER0, wCount);

  // Write MSB Count
  kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

/*
  return Counter0 
*/
WORD kReadCounter0(void)
{
  BYTE bHighByte, bLowByte;
  WORD wTemp = 0;

  kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

  // read counter
  bLowByte = kInPortByte(PIT_PORT_COUNTER0);
  bHighByte = kInPortByte(PIT_PORT_COUNTER0);

  wTemp = bHighByte;
  wTemp = (wTemp << 8) | bLowByte;
  return wTemp;
}

/*
  Set Counter & wait to Counter = 0
  maximum 50ms
*/
void kWaitUsingDirectPIT(WORD wCount)
{
  WORD wLastCounter0;
  WORD wCurrentCounter0;

  kInitializePIT(0, TRUE);

  wLastCounter0 = kReadCounter0();
  while (1) {
    wCurrentCounter0 = kReadCounter0();
    if (((wLastCounter0 - wCurrentCounter0) & 0xFFFF) >= wCount) {
      break;
    }
  }
}