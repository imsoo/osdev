#ifndef __TELNET_H__
#define __TELNET_H__

#include "Types.h"
#include "TCP.h"

#define TELNET_BUF_SIZE 30

#define TELNET_OPTION_NAWS  31
#define TELNET_OPTION_ECHO  1

#define TELNET_CMD_SE   240
#define TELNET_CMD_IP   244
#define TELNET_CMD_SB   250
#define TELNET_CMD_WILL 251
#define TELNET_CMD_WONT 252
#define TELNET_CMD_DO   253
#define TELNET_CMD_DONT 254
#define TELNET_CMD_IAC  255

BYTE kTelent_SimpleNegotiate(const TCP_TCB* pstTCB, BYTE* pbBuf, DWORD dwLen);
void kTelent_SimpleClient(QWORD dwAddress, WORD wPort);

#endif __TELNET_H__
