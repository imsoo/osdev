#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include "Types.h"
#include "Queue.h"
#include "Synchronization.h"

 // Serial Port I/O Port Address
#define SERIAL_PORT_COM1                            0x3F8
#define SERIAL_PORT_COM2                            0x2F8
#define SERIAL_PORT_COM3                            0x3E8
#define SERIAL_PORT_COM4                            0x2E8

// Register Offset
#define SERIAL_PORT_INDEX_RECEIVEBUFFER             0x00
#define SERIAL_PORT_INDEX_TRANSMITBUFFER            0x00
#define SERIAL_PORT_INDEX_INTERRUPTENABLE           0x01
#define SERIAL_PORT_INDEX_DIVISORLATCHLSB           0x00
#define SERIAL_PORT_INDEX_DIVISORLATCHMSB           0x01
#define SERIAL_PORT_INDEX_INTERRUPTIDENTIFICATION   0x02
#define SERIAL_PORT_INDEX_FIFOCONTROL               0x02
#define SERIAL_PORT_INDEX_LINECONTROL               0x03
#define SERIAL_PORT_INDEX_MODEMCONTROL              0x04
#define SERIAL_PORT_INDEX_LINESTATUS                0x05
#define SERIAL_PORT_INDEX_MODEMSTATUS               0x06

// Interrupt 
#define SERIAL_INTERRUPTENABLE_RECEIVEBUFFERFULL        0x01
#define SERIAL_INTERRUPTENABLE_TRANSMITTERBUFFEREMPTY   0x02
#define SERIAL_INTERRUPTENABLE_LINESTATUS               0x04
#define SERIAL_INTERRUPTENABLE_DELTASTATUS              0x08

// FIFO
#define SERIAL_FIFOCONTROL_FIFOENABLE               0x01
#define SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO         0x02
#define SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO        0x04
#define SERIAL_FIFOCONTROL_ENABLEDMA                0x08
#define SERIAL_FIFOCONTROL_1BYTEFIFO                0x00
#define SERIAL_FIFOCONTROL_4BYTEFIFO                0x40
#define SERIAL_FIFOCONTROL_8BYTEFIFO                0x80
#define SERIAL_FIFOCONTROL_14BYTEFIFO               0xC0

// LINE Control
#define SERIAL_LINECONTROL_8BIT                     0x03
#define SERIAL_LINECONTROL_1BITSTOP                 0x00
#define SERIAL_LINECONTROL_NOPARITY                 0x00
#define SERIAL_LINECONTROL_ODDPARITY                0x08
#define SERIAL_LINECONTROL_EVENPARITY               0x18
#define SERIAL_LINECONTROL_MARKPARITY               0x28
#define SERIAL_LINECONTROL_SPACEPARITY              0x38
#define SERIAL_LINECONTROL_DLAB                     0x80

// LINE Status
#define SERIAL_LINESTATUS_RECEIVEDDATAREADY         0x01
#define SERIAL_LINESTATUS_OVERRUNERROR              0x02
#define SERIAL_LINESTATUS_PARITYERROR               0x04
#define SERIAL_LINESTATUS_FRAMINGERROR              0x08
#define SERIAL_LINESTATUS_BREAKINDICATOR            0x10
#define SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY       0x20
#define SERIAL_LINESTATUS_TRANSMITEMPTY             0x40
#define SERIAL_LINESTATUS_RECEIVEDCHARACTORERROR    0x80

// LATCH
#define SERIAL_DIVISORLATCH_115200                  1
#define SERIAL_DIVISORLATCH_57600                   2
#define SERIAL_DIVISORLATCH_38400                   3
#define SERIAL_DIVISORLATCH_19200                   6
#define SERIAL_DIVISORLATCH_9600                    12
#define SERIAL_DIVISORLATCH_4800                    24
#define SERIAL_DIVISORLATCH_2400                    48

// FIFO MAX SIZE
#define SERIAL_FIFOMAXSIZE                          16

typedef struct kSerialPortManager
{
  // Mutex
  MUTEX stLock;
} SERIALMANAGER;

// function
void kInitializeSerialPort(void);
void kSendSerialData(BYTE* pbBuffer, int iSize);
int kReceiveSerialData(BYTE* pbBuffer, int iSize);
void kClearSerialFIFO(void);

static BOOL kIsSerialTransmitterBufferEmpty(void);
static BOOL kIsSerialReceiveBufferFull(void);

#endif /*__SERIALPORT_H__*/
