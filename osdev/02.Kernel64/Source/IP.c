#include "IP.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "Ethernet.h"

static IPMANAGER gs_stIPManager = { 0, };

void kIP_Task(void)
{
  FRAME stFrame;
  IP_HEADER stIPHeader, *pstIPHeader;
  BYTE* pbIPPayload;

  // �ʱ�ȭ
  if (kIP_Initialize() == FALSE)
    return;

  // �⺻ ���
  stIPHeader.bVersionAndIHL = (IP_VERSION_IPV4 << IP_VERSION_SHIFT) | 0x05;
  stIPHeader.bDSCPAndECN = 0x00;
  stIPHeader.wIdentification = htons(0x2000);
  stIPHeader.wFlagsAndFragmentOffset = htons(0x0000);
  stIPHeader.bTimeToLive = 128;
  stIPHeader.wHeaderChecksum = 0x00;
  kMemCpy(stIPHeader.vbSourceIPAddress, gs_stIPManager.vbIPAddress, 4);
  kMemCpy(stIPHeader.vbDestinationIPAddress, gs_stIPManager.vbGatewayAddress, 4);


  while (1)
  {
    // ������ ���� Ȯ��
    kIP_CheckReassemblyBufferList();

    // ť Ȯ��
    if (kGetQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      kDecapuslationFrame(&stFrame, &pstIPHeader, sizeof(IP_HEADER), &pbIPPayload);

      kPrintf("IP | Receive IP Datagram %x\n", pstIPHeader->bProtocol);

      // IP ���� Ȯ��
      if ((stIPHeader.bVersionAndIHL >> IP_VERSION_SHIFT) != IP_VERSION_IPV4) {
        kPrintf("IP | Not Supported Version %d\n", stIPHeader.bVersionAndIHL >> IP_VERSION_SHIFT);
        break;
      }

      // MF �� Fragment Offset�� 0�� �ƴ� ��� ����ȭ ��Ŷ
      // ������ ������ ���� ��
      if (ntohs(stIPHeader.wFlagsAndFragmentOffset) != 0) {
        kIP_Reassembly(&stFrame);
      }
      // �ƴ� ��� ���� ���̾�� ���� ��.
      else {
        switch (pstIPHeader->bProtocol)
        {
        case IP_PROTOCOL_ICMP:
          gs_stIPManager.pfSideOut(stFrame);
          break;
        case IP_PROTOCOL_TCP:
        case IP_PROTOCOL_UDP:
          gs_stIPManager.pfUp(stFrame);
          break;
        default:
          break;
        }
      }

      break;  /* End of case FRAME_OUT: */
    case FRAME_IN:
      kPrintf("IP | Send IP Datagram \n");

      switch (stFrame.bType)
      {
      case FRAME_IP:
        pstIPHeader = (IP_HEADER*)stFrame.pbCur;

        // IP �����ͱ׷��� ũ�Ⱑ MTU ���� ū ��� ����ȭ ����
        if (ntohs(pstIPHeader->wTotalLength) > IP_MAXIMUMTRANSMITUNIT) {
          kIP_Fragmentation(&stFrame);
        }
        // MTU ���� ���� ��� ���� ���̾�� �̵�
        else {
          gs_stIPManager.pfDown(stFrame);
        }
        break;

      case FRAME_ICMP:
        // IP ��� �߰�
        stIPHeader.wIdentification = htons(gs_stIPManager.wIdentification++);
        stIPHeader.bProtocol = IP_PROTOCOL_ICMP;
        stFrame.wLen += sizeof(IP_HEADER);
        stIPHeader.wTotalLength = htons(stFrame.wLen);
        stFrame.pbCur = stFrame.pbBuf + FRAME_MAX_SIZE - stFrame.wLen;
        kNumberToAddressArray(stIPHeader.vbDestinationIPAddress, stFrame.qwDestAddress, 4);
        //stIPHeader.wHeaderChecksum = kIP_CalcChecksum(&stIPHeader);
        kMemCpy(stFrame.pbCur, &stIPHeader, sizeof(IP_HEADER));

        stFrame.bType = FRAME_IP;
        stFrame.qwDestAddress = kAddressArrayToNumber(gs_stIPManager.vbGatewayAddress, 4);

        kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame);
        break;

      default:
        break;
      }

      break;  /* End of case FRAME_IN: */
    }
  }
}

BYTE kIP_Fragmentation(FRAME* stOriginalFrame)
{
  // ���� ������
  IP_HEADER* pstOriginalHeader;
  BYTE* pbOriginalData;

  // ����ȭ ������
  IP_HEADER stFragmentFrameIPHeader;
  FRAME stFragmentFrame;
  DWORD dwMaxNumberOfFragmentBlocks = 0;
  DWORD dwTotalNumberOfFragmentBlocks = 0;
  DWORD dwSendNumberOfFragmentBlocks = 0;
  DWORD dwFragmentOffset = 0;
  BOOL bMoreFragmentFlag = 0;

  pstOriginalHeader = (IP_HEADER*)stOriginalFrame->pbCur;
  pbOriginalData = (BYTE*)stOriginalFrame->pbCur + sizeof(IP_HEADER);

  // DF ��Ʈ�� ������ ��� ����ȭ �Ұ� -> �����ͱ׷� ����
  if (ntohs(pstOriginalHeader->wFlagsAndFragmentOffset) & (1 << IP_FLAGS_DF_SHIFT)) {
    kPrintf("IP | Fragment Fail | Frame Discard (DF) \n");
    // TODO : ���� �ڵ� ��ȯ �Ұ�.
    return FALSE;
  }

  dwFragmentOffset = 0;
  bMoreFragmentFlag = TRUE;

  // �� ����ȭ ���� (8 Byte) �� ���
  dwTotalNumberOfFragmentBlocks = ((ntohs(pstOriginalHeader->wTotalLength) - (IP_INETERNETHEADERLENGTH_DEFAULT * 4)) + 7) / 8;

  // �ϳ��� IP �����ͱ׷����� �����Ҽ� �ִ� �ִ� ����ȭ ���� �� ���
  dwMaxNumberOfFragmentBlocks = (IP_MAXIMUMTRANSMITUNIT - (IP_INETERNETHEADERLENGTH_DEFAULT * 4)) / 8;

  while (bMoreFragmentFlag) {

    // ���� ��� ����
    kMemCpy(&stFragmentFrameIPHeader, pstOriginalHeader, sizeof(IP_HEADER));

    // ����ȭ �� �� ������ �Ҵ�
    if (kAllocateFrame(&stFragmentFrame) == FALSE) {
      kPrintf("IP | Fragment Fail | Buffer Memory Allocation Fail \n");

      // TODO : ���� �ڵ� ��ȯ �Ұ�.
      return FALSE;
    }

    // ���� �� ����ȭ ���� �� ���
    dwSendNumberOfFragmentBlocks = MIN(dwMaxNumberOfFragmentBlocks, dwTotalNumberOfFragmentBlocks);

    // ����ȭ ��� ����
    // Total Length �ʵ� ����
    stFragmentFrameIPHeader.wTotalLength =
      htons((IP_INETERNETHEADERLENGTH_DEFAULT * 4) + (dwSendNumberOfFragmentBlocks * 8));

    // ������ ������ 
    if (dwSendNumberOfFragmentBlocks >= dwTotalNumberOfFragmentBlocks) {
      // MF =  0 ����
      bMoreFragmentFlag = FALSE;
    }

    // MF, Fragment Offset �ʵ� ����
    stFragmentFrameIPHeader.wFlagsAndFragmentOffset = htons(
      (bMoreFragmentFlag << IP_FLAGS_MF_SHIFT) |
      (dwFragmentOffset * dwMaxNumberOfFragmentBlocks));

    // checksum ����
    stFragmentFrameIPHeader.wHeaderChecksum = kIP_CalcChecksum(&stFragmentFrameIPHeader);


    // ĸ��ȭ
    kEncapuslationFrame(&stFragmentFrame, &stFragmentFrameIPHeader, sizeof(IP_HEADER),
      pbOriginalData, dwSendNumberOfFragmentBlocks * 8);

    pbOriginalData += dwSendNumberOfFragmentBlocks * 8;
    dwTotalNumberOfFragmentBlocks -= dwSendNumberOfFragmentBlocks;

    // ����ȭ ������ 1����
    dwFragmentOffset += 1;

    // ���� ���̾�� ����
    kPrintf("IP | Fragment Send \n");
    stFragmentFrame.qwDestAddress = stOriginalFrame->qwDestAddress;
    stFragmentFrame.bType = stOriginalFrame->bType;
    stFragmentFrame.eDirection = stOriginalFrame->eDirection;
    gs_stIPManager.pfDown(stFragmentFrame);

  }

  // ���� ������ ���
  kFreeFrame(stOriginalFrame);

  return TRUE;
}

void kIP_CheckReassemblyBufferList(void)
{
  IPREASSEMBLYBUFFER* pstBuffer = NULL;
  IPREASSEMBLYBUFFER* pstTempBuffer = NULL;
  FRAME stFrame;
  BOOL bReleaseFlag = FALSE;
  
  DWORD dwElapsedTime = 0;
  DWORD dwReaminTime = 0;

  // ����Ʈ Ȯ��
  if (kGetListCount(&(gs_stIPManager.stReassemblyBufferList)) > 0) {
    pstBuffer = kGetHeaderFromList(&(gs_stIPManager.stReassemblyBufferList));

    do
    {
      bReleaseFlag = FALSE;
      dwElapsedTime = kGetTickCount() - pstBuffer->qwTimer;
      dwReaminTime = pstBuffer->stIPHeader.bTimeToLive * 1000;
      // ������ ����ȭ �������� ���� �� ����
      if (pstBuffer->dwTotalLength != 0) {
        // ��Ʈ���� Ȯ���Ͽ� ��� �������� ���� ���θ� Ȯ��
        if (kIP_CheckReassemblyBufferBitmap(pstBuffer) == TRUE) {
          // ����� totalLength �ʵ带 �����յ� ������ ���̷� ����
          pstBuffer->stIPHeader.wTotalLength = htons((IP_INETERNETHEADERLENGTH_DEFAULT * 5) + pstBuffer->dwTotalLength);

          // ��Ŷ ����
          kAllocateBiggerFrame(&stFrame); // ����
          kEncapuslationFrame(&stFrame, &(pstBuffer->stIPHeader), sizeof(IP_HEADER), pstBuffer->pbDataBuffer, pstBuffer->dwTotalLength - (IP_INETERNETHEADERLENGTH_DEFAULT * 5));
          
          // ���� ���̾�� �����ϱ� ���� ť�� ����
          stFrame.eDirection = FRAME_OUT;
          kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame);

          // �ڿ� ����
          bReleaseFlag = TRUE;
        }
      }

      // TTL ��, Ÿ�̸� ���� �� ���� �ڿ� ����
      if ((dwElapsedTime > dwReaminTime) || (dwElapsedTime > IP_TIMER_DEFAULT_SECOND * 1000)) {
        
        // �ڿ� ����
        bReleaseFlag = TRUE;
      }

      // ���� ����Ʈ�� �̵�
      pstBuffer = kGetNextFromList(&(gs_stIPManager.stReassemblyBufferList), pstBuffer);

      // ������ �Ϸ� Ȥ�� Ÿ�̸� ���� �� ���� �ڿ� ����
      if (bReleaseFlag == TRUE) {
        kIP_ReleaseReassemblyBuffer(pstBuffer);
      }
    } while (pstBuffer != NULL);
  }
}

BOOL kIP_CheckReassemblyBufferBitmap(IPREASSEMBLYBUFFER* pstBuffer)
{
  int i;
  int iLastBitIndex = 0;
  int iByteCountToCheck = 0;
  BYTE* pbTempPosition;

  iByteCountToCheck = pstBuffer->dwTotalLength >> 3;

  // check 8 Bytes at once
  pbTempPosition = pstBuffer->pbFragmentBlockBitmap;
  for (i = 0; i < (iByteCountToCheck >> 3); i++)
  {
    if (*(QWORD*)(pbTempPosition) != 0)
    {
      return FALSE;
    }
    pbTempPosition += 8;
  }

  // check Remain Bytes
  for (i = 0; i < (iByteCountToCheck & 0x7); i++)
  {
    if (*pbTempPosition != 0)
    {
      return FALSE;
    }
    pbTempPosition++;
  }

  // check Remain bits
  iLastBitIndex = pstBuffer->dwTotalLength & 0x7;
  for (i = 0; i < iLastBitIndex; i++)
  {
    if (*pbTempPosition & (0x01 << i))
    {
      return FALSE;
    }
  }

  return TRUE;
}

void kIP_FillReassemblyBufferBitmap(IPREASSEMBLYBUFFER* pstBuffer, DWORD dwFragmentOffset, DWORD dwDataLength)
{
  int i;
  int iByteBeginOffset, iBitBeginOffset;
  int iByteCountToFill, iBitCountToFill, iLastBitIndex;
  BYTE* pbTempPosition;

  iByteBeginOffset = dwFragmentOffset / 8;
  iBitBeginOffset = dwFragmentOffset % 8;
  iBitCountToFill = (dwDataLength + 7) / 8;

  /* �׽�Ʈ �ʿ���. */
  // ��Ʈ�� ���� ��Ʈ �������� �߰��� ��ġ�� ���
  // ù ����Ʈ�� ä�� ������.
  if (iBitBeginOffset != 0) {
    pstBuffer->pbFragmentBlockBitmap[iByteBeginOffset]
      |= (1 << (8 - iBitBeginOffset) - 1);
    iBitCountToFill -= (8 - iBitBeginOffset);
    iByteBeginOffset += 1;
    iBitBeginOffset = 0;
  }

  iByteCountToFill = iBitCountToFill >> 3;

  // check 8 Bytes at once
  pbTempPosition = pstBuffer->pbFragmentBlockBitmap + iByteBeginOffset;
  for (i = 0; i < (iByteCountToFill >> 3); i++)
  {
    *(QWORD*)(pbTempPosition) = 0xFFFFFFFF;
    pbTempPosition += 8;
  }

  // check Remain Bytes
  for (i = 0; i < (iByteCountToFill & 0x7); i++)
  {
    *pbTempPosition = 0xFF;
    pbTempPosition++;
  }

  // check Remain bits
  iLastBitIndex = iBitCountToFill & 0x7;
  for (i = 0; i < iLastBitIndex; i++)
  {
    *pbTempPosition |= (0x01 << i);
  }

  /* �׽�Ʈ �ʿ���. */
}

IPREASSEMBLYBUFFER* kIP_GetReassemblyBuffer(QWORD qwID_1, QWORD qwID_2)
{
  IPREASSEMBLYBUFFER* pstBuffer = NULL;
  static QWORD qwID = 0;

  // ����Ʈ Ȯ��
  if (kGetListCount(&(gs_stIPManager.stReassemblyBufferList)) > 0) {
    pstBuffer = kGetHeaderFromList(&(gs_stIPManager.stReassemblyBufferList));

    do
    {
      if ((pstBuffer->qwBufID_1 == qwID_1) && (pstBuffer->qwBUfID_2 == qwID_2)) {
        // �ش� ���� 
        return pstBuffer;
      }
    } while (pstBuffer != NULL);
  }

  // �ش� ���۰� ���� ���
  // TODO : �����Ҵ��� �ƴ� POOL ���·� ����
  pstBuffer = (IPREASSEMBLYBUFFER*)kAllocateMemory(sizeof(IPREASSEMBLYBUFFER));
  if (pstBuffer == NULL) {
    kPrintf("IP | kIP_GetReassemblyBuffer | Fail");
    return NULL;
  }

  // ���� �ʱ�ȭ
  kIP_InitReassemblyBuffer(pstBuffer);
  pstBuffer->stEntryLink.qwID = qwID++;
  pstBuffer->qwBufID_1 = qwID_1;
  pstBuffer->qwBUfID_2 = qwID_2;

  // ����Ʈ�� �߰�
  kAddListToTail(&(gs_stIPManager.stReassemblyBufferList), pstBuffer);

  return pstBuffer;
}

void kIP_ReleaseReassemblyBuffer(IPREASSEMBLYBUFFER* pstBuffer)
{
  // ����Ʈ���� ����
  pstBuffer = kRemoveList(&(gs_stIPManager.stReassemblyBufferList), pstBuffer->stEntryLink.qwID);
  if (pstBuffer != NULL) {
    kFreeMemory(pstBuffer->pbDataBuffer);
    kFreeMemory(pstBuffer->pbFragmentBlockBitmap);
    kFreeMemory(pstBuffer);
  }
}

BOOL kIP_InitReassemblyBuffer(IPREASSEMBLYBUFFER* pstBuffer)
{
  // ���� �Ҵ�
  pstBuffer->pbDataBuffer = (BYTE*)kAllocateMemory(8192 * 8);
  if (pstBuffer->pbDataBuffer == NULL) {
    kPrintf("IP | kIP_InitReassemblyBuffer | Fail");
    return FALSE;
  }

  // ��Ʈ�� �Ҵ�
  pstBuffer->pbFragmentBlockBitmap = (BYTE*)kAllocateMemory(8192 / 8);
  if (pstBuffer->pbFragmentBlockBitmap == NULL) {
    kPrintf("IP | kIP_InitReassemblyBuffer | Fail");
    return FALSE;
  }

  pstBuffer->dwTotalLength = 0;
  pstBuffer->stIPHeader.bTimeToLive = 255;
  pstBuffer->qwTimer = kGetTickCount();
  return TRUE;
}

BYTE kIP_Reassembly(FRAME* stFrame)
{
  IP_HEADER* pstIPHeader;
  BYTE* pbIPData;
  IPREASSEMBLYBUFFER* pstReassemblyBuffer;
  DWORD dwFragmentOffset = 0;
  DWORD dwDataLength = 0;
  QWORD qwBufID_1 = 0;  // Source Address, Destination Address
  QWORD qwBufID_2 = 0;  // Protocol, identification

  pstIPHeader = (IP_HEADER*)stFrame;
  pbIPData = pstIPHeader + sizeof(IP_HEADER);

  qwBufID_1 = ((kAddressArrayToNumber(pstIPHeader->vbSourceIPAddress, 4) << 32) |
    kAddressArrayToNumber(pstIPHeader->vbDestinationIPAddress, 4));
  qwBufID_2 = (pstIPHeader->bProtocol << 32) | ntohs(pstIPHeader->wIdentification);

  // ������ ���� �������� (������ �Ҵ���)
  pstReassemblyBuffer = kIP_GetReassemblyBuffer(qwBufID_1, qwBufID_2);
  if (pstReassemblyBuffer == NULL) {
    kPrintf("IP | kIP_Reassembly | Fail");
    return FALSE;
  }

  dwFragmentOffset = ntohs(pstIPHeader->wFlagsAndFragmentOffset) & ((1 << IP_FLAGS_MF_SHIFT) - 1);
  dwDataLength = ntohs(pstIPHeader->wTotalLength) - (IP_INETERNETHEADERLENGTH_DEFAULT * 8);

  // ������ ���� �ùٸ� ��ġ�� ������ ����
  kMemCpy(pstReassemblyBuffer->pbDataBuffer + dwFragmentOffset * 8, pbIPData, dwDataLength);

  // ��Ʈ�� ä���
  kIP_FillReassemblyBufferBitmap(pstReassemblyBuffer, dwFragmentOffset, dwDataLength);

  // MF = 0, ������ ��Ŷ �� ���
  // �� ������ ���� ����
  if ((ntohs(pstIPHeader->wFlagsAndFragmentOffset) & (1 << IP_FLAGS_MF_SHIFT)) == 0) {
    pstReassemblyBuffer->dwTotalLength = dwFragmentOffset * 8 + dwDataLength;
  }

  // FO = 0, ù ��° ��Ŷ �� ���
  // ��� ����
  if (dwFragmentOffset == 0) {
    kMemCpy(&(pstReassemblyBuffer->stIPHeader), pstIPHeader, sizeof(IP_HEADER));

    // MF Flag ����
    pstReassemblyBuffer->stIPHeader.wFlagsAndFragmentOffset = 0x0000;
  }

  // ����ȭ ������ ���
  kFreeFrame(stFrame);

  return TRUE;
}

BOOL kIP_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_OUT;
  if (kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kIP_SideInPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kPutQueue(&(gs_stIPManager.stFrameQueue), &stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kIP_Initialize(void)
{
  // Allocate Frame Queue
  gs_stIPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stIPManager.pstFrameBuffer == NULL) {
    kPrintf("kIP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stIPManager.stFrameQueue), gs_stIPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // ������ ���� ����Ʈ �ʱ�ȭ
  kInitializeList(&(gs_stIPManager.stReassemblyBufferList));

  // ���̾� ����
  gs_stIPManager.pfDown = kEthernet_DownDirectionPoint;
  gs_stIPManager.pfUp = kStub_UpDirectionPoint;

  // �ӽ�
  // QEMU Virtual Network Device : 10.0.2.15
  gs_stIPManager.vbIPAddress[0] = 10;
  gs_stIPManager.vbIPAddress[1] = 0;
  gs_stIPManager.vbIPAddress[2] = 2;
  gs_stIPManager.vbIPAddress[3] = 15;

  // QEMU GateWay : 10.0.2.3
  gs_stIPManager.vbGatewayAddress[0] = 10;
  gs_stIPManager.vbGatewayAddress[1] = 0;
  gs_stIPManager.vbGatewayAddress[2] = 2;
  gs_stIPManager.vbGatewayAddress[3] = 2;

  return TRUE;
}

WORD kIP_CalcChecksum(IP_HEADER* pstHeader)
{
  int i;
  DWORD dwSum = 0;
  WORD* pwP = pstHeader;

  // üũ�� �ʱ�ȭ
  pstHeader->wHeaderChecksum = 0;

  for (int i = 0; i < sizeof(IP_HEADER) / 2; i++)
  {
    dwSum += ntohs(pwP[i]);
  }

  if (dwSum > 0xFFFF) {
    dwSum = ((dwSum >> 16) + (dwSum & 0xFFFF));
  }

  return ~(dwSum & 0xFFFF) & 0xFFFF;
}