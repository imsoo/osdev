#include "Ethernet.h"
#include "E1000.h"
#include "AssemblyUtility.h"
#include "Utility.h"

static E1000MANAGER gs_stE1000Manager = { 0, };

BOOL kE1000_Initialize(QWORD qwMMIOAddress, WORD wIOAddress)
{
  int i;

  kMemSet(&(gs_stE1000Manager), 0, sizeof(gs_stE1000Manager));

  gs_stE1000Manager.qwMMIOAddress = qwMMIOAddress;
  gs_stE1000Manager.wIOAddress = wIOAddress;

  // 기본적으로 Memory Mapped I/O를 사용합니다.
  gs_stE1000Manager.bUseMMIO = TRUE;

  // EEProm
  if (kE1000_DetectEEPROM() == FALSE) {
    gs_stE1000Manager.bEEPROMExists = FALSE;
  }
  else {
    gs_stE1000Manager.bEEPROMExists = TRUE;
  }

  // Get MAC Address
  if (kE1000_ReadMACAddress(gs_stE1000Manager.vbMacAddress) == FALSE) {
    return FALSE;
  }
  
  // Link 설정 
  kE1000_SetLinkUp();

  // MTA 초기화
  kE1000_MulticastTableArrayInit();

  // 인터럽트 초기화
  kE1000_EnableInterrupt();

  // Rx 초기화
  if (kE1000_RxInit() == FALSE) {
    return FALSE;
  }

  // Tx 초기화
  if (kE1000_TxInit() == FALSE) {
    // Rx 버퍼 해제
    for (i = 0; i < DESC_RX_NUM; i++) {
      kFreeMemory(gs_stE1000Manager.vpstRxDescriptor[i]->qwAddr);
    }
    kFreeMemory(gs_stE1000Manager.vpstRxDescriptor[0]);
    return FALSE;
  }

  kPrintf("E1000 Configuration Success\n");
  return TRUE;
}

BOOL kE1000_Send(const void* pvData, WORD wLen)
{
  WORD wOldTxTail;
  gs_stE1000Manager.vpstTxDescriptor[gs_stE1000Manager.wTxTail]->qwAddr = (QWORD)pvData;
  gs_stE1000Manager.vpstTxDescriptor[gs_stE1000Manager.wTxTail]->wLength = wLen;
  gs_stE1000Manager.vpstTxDescriptor[gs_stE1000Manager.wTxTail]->bCmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
  gs_stE1000Manager.vpstTxDescriptor[gs_stE1000Manager.wTxTail]->bStatus = 0;

  // Queue Tail 갱신
  wOldTxTail = gs_stE1000Manager.wTxTail;
  gs_stE1000Manager.wTxTail = (gs_stE1000Manager.wTxTail + 1) % DESC_TX_NUM;
  kE1000_WriteCommand(REG_TDT, gs_stE1000Manager.wTxTail);

  while (!(gs_stE1000Manager.vpstTxDescriptor[wOldTxTail]->bStatus & 0xff));
  return TRUE;
}

BOOL kE1000_Receive(FRAME* pstFrame)
{
  int i;
  WORD wOldRxTail;
  BYTE* pbReceiveBuf;
  WORD wReceiveLen;

  while ((gs_stE1000Manager.vpstRxDescriptor[gs_stE1000Manager.wRxTail]->bStatus & 0x1))
  {
    pbReceiveBuf = (BYTE*)gs_stE1000Manager.vpstRxDescriptor[gs_stE1000Manager.wRxTail]->qwAddr;
    wReceiveLen = gs_stE1000Manager.vpstRxDescriptor[gs_stE1000Manager.wRxTail]->wLength;

    kMemCpy(pstFrame->pbBuf, pbReceiveBuf, wReceiveLen);
    pstFrame->pbCur = pstFrame->pbBuf;
    pstFrame->wLen = wReceiveLen;

    gs_stE1000Manager.vpstRxDescriptor[gs_stE1000Manager.wRxTail]->bStatus = 0;
    wOldRxTail = gs_stE1000Manager.wRxTail;
    gs_stE1000Manager.wRxTail = (gs_stE1000Manager.wRxTail + 1) % DESC_RX_NUM;
    kE1000_WriteCommand(REG_RDT, wOldRxTail);
  }
  return TRUE;
}

HANDLERSTATUS kE1000_Handler(void)
{
  DWORD dwStatus;

  // Interrupt 정보 읽기
  dwStatus = kE1000_ReadCommand(REG_ICR);

  // 링크 상태 변경 (LSC : Bit 2) 
  if (dwStatus & 0x04) {
    return HANDLER_LSC;
  }
  // threshold 미만 (RXDMT : Bit 4)
  else if (dwStatus & 0x10) {
    kPrintf("Receive Descriptor Minimum Threshold hit\n");
    return HANDLER_RXDMT;
  }
  // 패킷 대기중 (RXT0 : Bit 7)
  else if (dwStatus & 0x80) {
    return HANDLER_RXT0;
  }
  return HANDLER_UNKNOWN;
}


BOOL kE1000_RxInit(void)
{
  int i;
  BYTE* pbMemoryAddress;
  RXDESCRIPTOR* pstDescriptor;

  pbMemoryAddress =
    (BYTE*)kAllocateMemory(sizeof(RXDESCRIPTOR) * DESC_RX_NUM + 16);
  if (pbMemoryAddress == NULL) {
    kPrintf("kE1000_RxInit : Memory Allocation Fail\n");
    return FALSE;
  }

  // 메모리 초기화
  kMemSet((BYTE*)pbMemoryAddress, 0, sizeof(RXDESCRIPTOR) * DESC_RX_NUM + 16);

  pstDescriptor = (RXDESCRIPTOR*)pbMemoryAddress;
  // Descriptor 설정
  for (i = 0; i < DESC_RX_NUM; i++) {
    gs_stE1000Manager.vpstRxDescriptor[i] =
      (RXDESCRIPTOR*)((BYTE*)pstDescriptor + i * 16);

    gs_stE1000Manager.vpstRxDescriptor[i]->qwAddr =
      (QWORD)(BYTE*)(kAllocateMemory(8192 + 16));

    gs_stE1000Manager.vpstRxDescriptor[i]->bStatus = 0;
  }

  // Receive Descriptor Base Address 설정
  kE1000_WriteCommand(REG_RDBAH, (DWORD)((QWORD)pstDescriptor >> 32));
  kE1000_WriteCommand(REG_RDBAL, (DWORD)((QWORD)pstDescriptor & 0xFFFFFFFF));

  // Receive Descriptor Queue 설정 (Length, Head, Tail)
  kE1000_WriteCommand(REG_RDLEN, DESC_RX_NUM * 16);
  kE1000_WriteCommand(REG_RDH, 0);
  kE1000_WriteCommand(REG_RDT, DESC_RX_NUM - 1);

  // Manager Queue Tail 정보 초기화
  gs_stE1000Manager.wRxTail = 0;

  // Receieve Control Register 설정
  kE1000_WriteCommand(REG_RCTL,
    RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE |
    RCTL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_8192);

  return TRUE;
}

BOOL kE1000_TxInit(void)
{
  int i;
  BYTE* pbMemoryAddress;
  TXDESCRIPTOR* pstDescriptor;

  pbMemoryAddress =
    (BYTE*)kAllocateMemory(sizeof(TXDESCRIPTOR) * DESC_TX_NUM + 16);
  if (pbMemoryAddress == NULL) {
    kPrintf("kE1000_TxInit : Memory Allocation Fail\n");
    return FALSE;
  }

  // 메모리 초기화
  kMemSet((BYTE*)pbMemoryAddress, 0, sizeof(TXDESCRIPTOR) * DESC_TX_NUM + 16);

  pstDescriptor = (TXDESCRIPTOR*)pbMemoryAddress;
  // Descriptor 설정
  for (i = 0; i < DESC_TX_NUM; i++) {
    gs_stE1000Manager.vpstTxDescriptor[i] =
      (TXDESCRIPTOR*)((BYTE*)pstDescriptor + i * 16);

    gs_stE1000Manager.vpstTxDescriptor[i]->qwAddr = 0;
    gs_stE1000Manager.vpstTxDescriptor[i]->bCmd = 0;
    gs_stE1000Manager.vpstTxDescriptor[i]->bStatus = (1 << 0);
  }

  // Trasmit Descriptor Base Address 설정
  kE1000_WriteCommand(REG_TDBAH, (DWORD)((QWORD)pstDescriptor >> 32));
  kE1000_WriteCommand(REG_TDBAL, (DWORD)((QWORD)pstDescriptor & 0xFFFFFFFF));

  // Trasmit Descriptor Queue 설정 (Length, Head, Tail)
  kE1000_WriteCommand(REG_TDLEN, DESC_TX_NUM * 16);
  kE1000_WriteCommand(REG_TDH, 0);
  kE1000_WriteCommand(REG_TDT, 0);

  // Manager Queue Tail 정보 초기화
  gs_stE1000Manager.wTxTail = 0;

  // Trasmit Control Register 설정
  kE1000_WriteCommand(REG_TCTL, TCTL_EN
    | TCTL_PSP
    | (15 << TCTL_CT_SHIFT)
    | (64 << TCTL_COLD_SHIFT)
    | TCTL_RTLC);

  return TRUE;
}

void kE1000_MulticastTableArrayInit(void)
{
  int i;
  for (i = 0; i < MTA_ENTRY_COUNT; i++) {
    kE1000_WriteCommand(REG_MTA + i * 4, 0);
  }
}

void kE1000_SetLinkUp(void)
{
  kE1000_WriteCommand(REG_CTRL, (kE1000_ReadCommand(REG_CTRL) | CTRL_SLU));
}

void kE1000_EnableInterrupt(void)
{
  // Enable All interrupt
  kE1000_WriteCommand(REG_IMS, 0x1F6DC);
  kE1000_WriteCommand(REG_IMS, 0xff & ~4);
  kE1000_ReadCommand(REG_ICR);
}

BOOL kE1000_DetectEEPROM(void)
{
  int i;
  DWORD dwVal = 0;

  kE1000_WriteCommand(REG_EEPROM, 0x1);

  for (i = 0; i < 1000; i++) {
    dwVal = kE1000_ReadCommand(REG_EEPROM);
    if (dwVal & 0x10) {
      return TRUE;
    }
  }

  return FALSE;
}

WORD kE1000_EEPROMRead(WORD wAddress)
{
  WORD wData = 0;
  DWORD dwTmp = 0;

  if (gs_stE1000Manager.bEEPROMExists == TRUE) {
    kE1000_WriteCommand(REG_EEPROM, (1) | ((DWORD)(wAddress) << 8));
    while (!((dwTmp = kE1000_ReadCommand(REG_EEPROM)) & (1 << 4)));
  }
  else {
    kE1000_WriteCommand(REG_EEPROM, (1) | ((DWORD)(wAddress) << 2));
    while (!((dwTmp = kE1000_ReadCommand(REG_EEPROM)) & (1 << 1)));
  }

  wData = (WORD)((dwTmp >> 16) & 0xFFFF);
  return wData;
}

BOOL kE1000_ReadMACAddress(BYTE* pbBuf)
{
  int i;
  DWORD dwTemp;
  
  if (gs_stE1000Manager.bEEPROMExists == TRUE) {
    dwTemp = kE1000_EEPROMRead(0);
    pbBuf[0] = dwTemp & 0xff;
    pbBuf[1] = dwTemp >> 8;

    dwTemp = kE1000_EEPROMRead(1);
    pbBuf[2] = dwTemp & 0xff;
    pbBuf[3] = dwTemp >> 8;

    dwTemp = kE1000_EEPROMRead(2);
    pbBuf[4] = dwTemp & 0xff;
    pbBuf[5] = dwTemp >> 8;
  }
  else {
    if (((DWORD*)(gs_stE1000Manager.qwMMIOAddress + 0x5400))[0] != 0)
    {
      for (i = 0; i < 6; i++) {
        pbBuf[i] = ((BYTE*)(gs_stE1000Manager.qwMMIOAddress + 0x5400))[i];
      }
    }
    else
      return FALSE;
  }

  return TRUE;
}

void kE1000_WriteCommand(WORD wAddress, DWORD dwValue)
{
  // Memory Mapped I/O 사용
  if (gs_stE1000Manager.bUseMMIO == TRUE) {
    return kE1000_WriteDWORD(gs_stE1000Manager.qwMMIOAddress + wAddress, dwValue);
  }
  // Port I/O 사용
  else {
    kOutPortDWord(gs_stE1000Manager.wIOAddress + REG_IOADDR, wAddress);
    kOutPortDWord(gs_stE1000Manager.wIOAddress + REG_IODATA, dwValue);
  }
}

DWORD kE1000_ReadCommand(WORD wAddress)
{
  // Memory Mapped I/O 사용
  if (gs_stE1000Manager.bUseMMIO == TRUE) {
    return kE1000_ReadDWORD(gs_stE1000Manager.qwMMIOAddress + wAddress);
  }
  // Port I/O 사용
  else {
    kOutPortDWord(gs_stE1000Manager.wIOAddress + REG_IOADDR, wAddress);
    return kInPortDWord(gs_stE1000Manager.wIOAddress + REG_IODATA);
  }
}

void kE1000_WriteDWORD(QWORD wAddress, DWORD dwValue)
{
  (*((volatile DWORD*)(wAddress))) = dwValue;
}

DWORD kE1000_ReadDWORD(QWORD wAddress)
{
  return *((volatile DWORD*)(wAddress));
}