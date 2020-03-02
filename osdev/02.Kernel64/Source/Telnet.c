#include "Telnet.h"
#include "TCP.h"
#include "Utility.h"

BYTE kTelent_SimpleNegotiate(const TCP_TCB* pstTCB, BYTE* pbBuf, DWORD dwLen)
{
  int i;
  int iCursorX, iCursorY;
  BYTE vbReply[] = { TELNET_CMD_IAC, TELNET_CMD_WILL, TELNET_OPTION_NAWS };
  BYTE vbRequest[] = { 
    TELNET_CMD_IAC, TELNET_CMD_SB, TELNET_OPTION_NAWS,  // 형상 시작
    0, 80, 0, 24,
    TELNET_CMD_IAC, TELNET_CMD_SE // 협상 조료
  };

  if ((pbBuf[1] == TELNET_CMD_DO) && (pbBuf[2] == TELNET_OPTION_NAWS)) {
    if (kTCP_Send(pstTCB, vbReply, 3, TCP_PUSH) < 0)
      return -1;

    if (kTCP_Send(pstTCB, vbRequest, 9, TCP_PUSH) < 0)
      return -1;
  }

  for (i = 0; i < dwLen; i++) {
    if (pbBuf[i] == TELNET_CMD_DO)  // 나머지 옵션 WONT 응답
      pbBuf[i] = TELNET_CMD_WONT;
    else if (pbBuf[i] == TELNET_CMD_WILL)
      pbBuf[i] = TELNET_CMD_DO;
  }

  if (kTCP_Send(pstTCB, pbBuf, dwLen, TCP_PUSH) < 0)
    return -1;
  return 0;
}

void kTelent_SimpleClient(QWORD dwAddress, WORD wPort)
{
  int i;
  int iCursorX, iCursorY;
  int dwRecvLen;
  BYTE vbAddress[4];
  BOOL bCtrl = FALSE;
  BYTE bKey;
  BYTE vbBuf[TELNET_BUF_SIZE + 1];
  TCP_TCB* pstTCB;
  kPrintf("Telnet | Connecting to ");
  kNumberToAddressArray(vbAddress, dwAddress, 4);
  kPrintIPAddress(vbAddress);
  kPrintf(":%d...\n", wPort);

  pstTCB = kTCP_CreateTCB(51515, (dwAddress << 16) | wPort, TCP_ACTIVE);
  if (pstTCB == NULL) {
    kPrintf("Telnet | Connection failed...\n");
    return 0;
  }

  while (kTCP_GetCurrentState(pstTCB) != TCP_ESTABLISHED);

  kPrintf("Telnet | Connection established...\n");

  while (1) {
    dwRecvLen = kTCP_Recv(pstTCB, vbBuf, 1, TCP_NONBLOCK);
    if (dwRecvLen < 0) {  // 연결 종로
      kPrintf("Telnet | Connection closed by the remote end...\n");
      kTCP_Close(pstTCB);
      return 0;
    }
    else if (dwRecvLen != 0) {  // 수신 받은 데이터가 있는 경우
      if (vbBuf[0] == TELNET_CMD_IAC) { // 명령어 수신
        dwRecvLen = kTCP_Recv(pstTCB, vbBuf + 1, 2, NULL);  // 2 바이트 더 수신
        if (dwRecvLen < 0) {
          kPrintf("Telnet | Connection closed by the remote end...\n");
          kTCP_Close(pstTCB);
          return 0;
        }
        if (kTelent_SimpleNegotiate(pstTCB, vbBuf, 3) != 0) {
          kTCP_Close(pstTCB);
          return 0;
        }
      }
      else {  // 데이터 수신
        if (vbBuf[0] == 0x07) {
          // PASS
        }
        else if (vbBuf[0] == 0x08) {  // Backspace
          kGetCursor(&iCursorX, &iCursorY);
          kPrintStringXY(iCursorX - 1, iCursorY, " ");
          kSetCursor(iCursorX - 1, iCursorY);
        }
        else if (vbBuf[0] != '\r') {
          vbBuf[dwRecvLen] = '\0';
          kPrintf("%s", vbBuf);
        }
      }
    }

    bKey = kGetChNonBlock();
    if (bKey != 0xFF) {
      vbBuf[0] = bKey;
      if (bCtrl == TRUE) { // CTRL 키 + C : TELNET_CMD_IP
        if (bKey == 'c' || bKey == 'C') {
          vbBuf[0] = TELNET_CMD_IAC;
          vbBuf[1] = TELNET_CMD_IP;
          if (kTCP_Send(pstTCB, vbBuf, 2, TCP_PUSH) < 0) {
            kTCP_Close(pstTCB);
            return -1;
          }
          continue;
        }
        bCtrl = FALSE;
      }

      if (bKey == '\n') {
        vbBuf[0] = '\r';
        vbBuf[1] = '\n';
        if (kTCP_Send(pstTCB, vbBuf, 2, TCP_PUSH) < 0) {
          kTCP_Close(pstTCB);
          return -1;
        }
      }
      else if (bKey == 0x81) { // CTRL 키
        bCtrl = TRUE;
        continue;
      }
      else {
        if (kTCP_Send(pstTCB, vbBuf, 1, TCP_PUSH) < 0) {
          kTCP_Close(pstTCB);
          return -1;
        }
      }
    }
  }

  kTCP_Close(pstTCB);
  kPrintf("Telnet | Connection closed...\n");
  return 0;
}