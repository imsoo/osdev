#include "SerialPort.h"
#include "Utility.h"

static SERIALMANAGER gs_stSerialManager;

/*
  Init Serial Port Manager
*/
void kInitializeSerialPort(void)
{
  WORD wPortBaseAddress;

  // Init Mutex
  kInitializeMutex(&(gs_stSerialManager.stLock));

  // Select Serial I/O Port (COM1)
  wPortBaseAddress = SERIAL_PORT_COM1;

  // interrupt Disable
  kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);

  // Set Baud Rate (115200)
  kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL,
    SERIAL_LINECONTROL_DLAB);
  kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHLSB,
    SERIAL_DIVISORLATCH_115200);
  kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHMSB,
    SERIAL_DIVISORLATCH_115200 >> 8);

  // Line Control (8 Bit, NoParity, 1 Stop bit)
  kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL,
    SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_NOPARITY |
    SERIAL_LINECONTROL_1BITSTOP);

  // FIFO (14)
  kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_FIFOCONTROL,
    SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}

/*
  Check Transmit Queue is Empty
*/
static BOOL kIsSerialTransmitterBufferEmpty(void)
{
  BYTE bData;

  // Check TBE(bit 1)
  bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
  if ((bData & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) ==
    SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY)
  {
    return TRUE;
  }
  return FALSE;
}

/*
  Send Data to Serial Port
*/
void kSendSerialData(BYTE* pbBuffer, int iSize)
{
  int iSentByte;
  int iTempSize;
  int j;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stSerialManager.stLock));


  iSentByte = 0;
  while (iSentByte < iSize)
  {
    // Transmit Queue Check
    while (kIsSerialTransmitterBufferEmpty() == FALSE)
    {
      kSleep(0);
    }

    // Send
    iTempSize = MIN(iSize - iSentByte, SERIAL_FIFOMAXSIZE);
    for (j = 0; j < iTempSize; j++)
    {
      kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMITBUFFER,
        pbBuffer[iSentByte + j]);
    }
    iSentByte += iTempSize;
  }

  kUnlock(&(gs_stSerialManager.stLock));
  // --- CRITCAL SECTION END ---
}

/*
  Check Receive Queue is full
*/
static BOOL kIsSerialReceiveBufferFull(void)
{
  BYTE bData;

  // Check RxRD(bit 0)
  bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
  if ((bData & SERIAL_LINESTATUS_RECEIVEDDATAREADY) ==
    SERIAL_LINESTATUS_RECEIVEDDATAREADY)
  {
    return TRUE;
  }
  return FALSE;
}

/*
  Read Data From Serial Port
*/
int kReceiveSerialData(BYTE* pbBuffer, int iSize)
{
  int i;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stSerialManager.stLock));

  for (i = 0; i < iSize; i++)
  {
    // Receive Queue Check
    if (kIsSerialReceiveBufferFull() == FALSE)
    {
      break;
    }
    // Read
    pbBuffer[i] = kInPortByte(SERIAL_PORT_COM1 +
      SERIAL_PORT_INDEX_RECEIVEBUFFER);
  }

  kUnlock(&(gs_stSerialManager.stLock));
  // --- CRITCAL SECTION END ---

  return i;
}

/*
  Recv/Send Queue Clear
*/
void kClearSerialFIFO(void)
{
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stSerialManager.stLock));

  kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFOCONTROL,
    SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO |
    SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);

  kUnlock(&(gs_stSerialManager.stLock));
  // --- CRITCAL SECTION END ---
}
