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

  // 초기화
  if (kIP_Initialize() == FALSE)
    return;

  // 기본 헤더
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
    // 재조합 버퍼 확인
    kIP_CheckReassemblyBufferList();

    // 큐 확인
    if (kIP_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      kSleep(0);
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_OUT:
      kDecapuslationFrame(&stFrame, &pstIPHeader, sizeof(IP_HEADER), &pbIPPayload);

      kPrintf("IP | Receive IP Datagram %x\n", pstIPHeader->bProtocol);

      // IP 버전 확인
      if ((stIPHeader.bVersionAndIHL >> IP_VERSION_SHIFT) != IP_VERSION_IPV4) {
        kPrintf("IP | Not Supported Version %d\n", stIPHeader.bVersionAndIHL >> IP_VERSION_SHIFT);
        break;
      }

      // MF 와 Fragment Offset이 0이 아닌 경우 단편화 패킷
      // 재조립 과정을 수행 함
      if (ntohs(stIPHeader.wFlagsAndFragmentOffset) != 0) {
        kIP_Reassembly(&stFrame);
      }
      // 아닌 경우 상위 레이어로 전달 함.
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

        // IP 데이터그램의 크기가 MTU 보다 큰 경우 단편화 진행
        if (ntohs(pstIPHeader->wTotalLength) > IP_MAXIMUMTRANSMITUNIT) {
          kIP_Fragmentation(&stFrame);
        }
        // MTU 보다 작은 경우 하위 레이어로 이동
        else {
          gs_stIPManager.pfDown(stFrame);
        }
        break;

      case FRAME_ICMP:
        // IP 헤더 추가
        stIPHeader.wIdentification = htons(gs_stIPManager.wIdentification++);
        stIPHeader.bProtocol = IP_PROTOCOL_ICMP;
        stFrame.wLen += sizeof(IP_HEADER);
        stIPHeader.wTotalLength = htons(stFrame.wLen);
        stFrame.pbCur = stFrame.pbBuf + FRAME_MAX_SIZE - stFrame.wLen;
        kNumberToAddressArray(stIPHeader.vbDestinationIPAddress, stFrame.qwDestAddress, 4);
        stIPHeader.wHeaderChecksum = kIP_CalcChecksum(&stIPHeader);
        kMemCpy(stFrame.pbCur, &stIPHeader, sizeof(IP_HEADER));

        stFrame.bType = FRAME_IP;
        stFrame.qwDestAddress = kAddressArrayToNumber(gs_stIPManager.vbGatewayAddress, 4);

        kIP_PutFrameToFrameQueue(&stFrame);
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
  // 원본 프레임
  IP_HEADER* pstOriginalHeader;
  BYTE* pbOriginalData;

  // 단편화 프레임
  IP_HEADER stFragmentFrameIPHeader;
  FRAME stFragmentFrame;
  DWORD dwMaxNumberOfFragmentBlocks = 0;
  DWORD dwTotalNumberOfFragmentBlocks = 0;
  DWORD dwSendNumberOfFragmentBlocks = 0;
  DWORD dwFragmentOffset = 0;
  BOOL bMoreFragmentFlag = 0;

  pstOriginalHeader = (IP_HEADER*)stOriginalFrame->pbCur;
  pbOriginalData = (BYTE*)stOriginalFrame->pbCur + sizeof(IP_HEADER);

  // DF 비트가 설정된 경우 단편화 불가 -> 데이터그램 버림
  if (ntohs(pstOriginalHeader->wFlagsAndFragmentOffset) & (1 << IP_FLAGS_DF_SHIFT)) {
    kPrintf("IP | Fragment Fail | Frame Discard (DF) \n");
    // TODO : 에러 코드 반환 할것.
    return FALSE;
  }

  dwFragmentOffset = 0;
  bMoreFragmentFlag = TRUE;

  // 총 단편화 블록 (8 Byte) 수 계산
  dwTotalNumberOfFragmentBlocks = ((ntohs(pstOriginalHeader->wTotalLength) - (IP_INETERNETHEADERLENGTH_DEFAULT * 4)) + 7) / 8;

  // 하나의 IP 데이터그램으로 전송할수 있는 최대 단편화 블록 수 계산
  dwMaxNumberOfFragmentBlocks = (IP_MAXIMUMTRANSMITUNIT - (IP_INETERNETHEADERLENGTH_DEFAULT * 4)) / 8;

  while (bMoreFragmentFlag) {

    // 원본 헤더 복사
    kMemCpy(&stFragmentFrameIPHeader, pstOriginalHeader, sizeof(IP_HEADER));

    // 단편화 용 새 프레임 할당
    if (kAllocateFrame(&stFragmentFrame) == FALSE) {
      kPrintf("IP | Fragment Fail | Buffer Memory Allocation Fail \n");

      // TODO : 에러 코드 반환 할것.
      return FALSE;
    }

    // 전송 할 단편화 블록 수 계산
    dwSendNumberOfFragmentBlocks = MIN(dwMaxNumberOfFragmentBlocks, dwTotalNumberOfFragmentBlocks);

    // 단편화 헤더 설정
    // Total Length 필드 설정
    stFragmentFrameIPHeader.wTotalLength =
      htons((IP_INETERNETHEADERLENGTH_DEFAULT * 4) + (dwSendNumberOfFragmentBlocks * 8));

    // 마지막 프레임 
    if (dwSendNumberOfFragmentBlocks >= dwTotalNumberOfFragmentBlocks) {
      // MF =  0 설정
      bMoreFragmentFlag = FALSE;
    }

    // MF, Fragment Offset 필드 설정
    stFragmentFrameIPHeader.wFlagsAndFragmentOffset = htons(
      (bMoreFragmentFlag << IP_FLAGS_MF_SHIFT) |
      (dwFragmentOffset * dwMaxNumberOfFragmentBlocks));

    // checksum 재계산
    stFragmentFrameIPHeader.wHeaderChecksum = kIP_CalcChecksum(&stFragmentFrameIPHeader);


    // 캡슐화
    kEncapuslationFrame(&stFragmentFrame, &stFragmentFrameIPHeader, sizeof(IP_HEADER),
      pbOriginalData, dwSendNumberOfFragmentBlocks * 8);

    pbOriginalData += dwSendNumberOfFragmentBlocks * 8;
    dwTotalNumberOfFragmentBlocks -= dwSendNumberOfFragmentBlocks;

    // 단편화 오프셋 1증가
    dwFragmentOffset += 1;

    // 하위 레이어로 전송
    kPrintf("IP | Fragment Send \n");
    stFragmentFrame.qwDestAddress = stOriginalFrame->qwDestAddress;
    stFragmentFrame.bType = stOriginalFrame->bType;
    stFragmentFrame.eDirection = stOriginalFrame->eDirection;
    gs_stIPManager.pfDown(stFragmentFrame);

  }

  // 원본 프레임 폐기
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

  // 리스트 확인
  if (kGetListCount(&(gs_stIPManager.stReassemblyBufferList)) > 0) {
    pstBuffer = kGetHeaderFromList(&(gs_stIPManager.stReassemblyBufferList));

    do
    {
      bReleaseFlag = FALSE;
      dwElapsedTime = kGetTickCount() - pstBuffer->qwTimer;
      dwReaminTime = pstBuffer->stIPHeader.bTimeToLive * 1000;
      // 마지막 단편화 프레임이 수신 된 상태
      if (pstBuffer->dwTotalLength != 0) {
        // 비트맵을 확인하여 모든 프레임의 수신 여부를 확인
        if (kIP_CheckReassemblyBufferBitmap(pstBuffer) == TRUE) {
          // 헤더의 totalLength 필드를 재조합된 프레임 길이로 변경
          pstBuffer->stIPHeader.wTotalLength = htons((IP_INETERNETHEADERLENGTH_DEFAULT * 5) + pstBuffer->dwTotalLength);

          // 패킷 생성
          kAllocateBiggerFrame(&stFrame); // 변경
          kEncapuslationFrame(&stFrame, &(pstBuffer->stIPHeader), sizeof(IP_HEADER), pstBuffer->pbDataBuffer, pstBuffer->dwTotalLength - (IP_INETERNETHEADERLENGTH_DEFAULT * 5));
          
          // 상위 레이어로 전송하기 위해 큐에 삽입
          stFrame.eDirection = FRAME_OUT;
          kIP_PutFrameToFrameQueue(&stFrame);

          // 자원 해제
          bReleaseFlag = TRUE;
        }
      }

      // TTL 값, 타이머 만료 시 버퍼 자원 해제
      if ((dwElapsedTime > dwReaminTime) || (dwElapsedTime > IP_TIMER_DEFAULT_SECOND * 1000)) {
        
        // 자원 해제
        bReleaseFlag = TRUE;
      }

      // 다음 리스트로 이동
      pstBuffer = kGetNextFromList(&(gs_stIPManager.stReassemblyBufferList), pstBuffer);

      // 재조합 완료 혹은 타이머 만료 시 버퍼 자원 해제
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

  /* 테스트 필요함. */
  // 비트맵 시작 비트 오프셋이 중간에 위치한 경우
  // 첫 바이트를 채워 정렬함.
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

  /* 테스트 필요함. */
}

IPREASSEMBLYBUFFER* kIP_GetReassemblyBuffer(QWORD qwID_1, QWORD qwID_2)
{
  IPREASSEMBLYBUFFER* pstBuffer = NULL;
  static QWORD qwID = 0;

  // 리스트 확인
  if (kGetListCount(&(gs_stIPManager.stReassemblyBufferList)) > 0) {
    pstBuffer = kGetHeaderFromList(&(gs_stIPManager.stReassemblyBufferList));

    do
    {
      if ((pstBuffer->qwBufID_1 == qwID_1) && (pstBuffer->qwBUfID_2 == qwID_2)) {
        // 해당 버퍼 
        return pstBuffer;
      }
    } while (pstBuffer != NULL);
  }

  // 해당 버퍼가 없는 경우
  // TODO : 동적할당이 아닌 POOL 형태로 개선
  pstBuffer = (IPREASSEMBLYBUFFER*)kAllocateMemory(sizeof(IPREASSEMBLYBUFFER));
  if (pstBuffer == NULL) {
    kPrintf("IP | kIP_GetReassemblyBuffer | Fail");
    return NULL;
  }

  // 버퍼 초기화
  kIP_InitReassemblyBuffer(pstBuffer);
  pstBuffer->stEntryLink.qwID = qwID++;
  pstBuffer->qwBufID_1 = qwID_1;
  pstBuffer->qwBUfID_2 = qwID_2;

  // 리스트에 추가
  kAddListToTail(&(gs_stIPManager.stReassemblyBufferList), pstBuffer);

  return pstBuffer;
}

void kIP_ReleaseReassemblyBuffer(IPREASSEMBLYBUFFER* pstBuffer)
{
  // 리스트에서 제거
  pstBuffer = kRemoveList(&(gs_stIPManager.stReassemblyBufferList), pstBuffer->stEntryLink.qwID);
  if (pstBuffer != NULL) {
    kFreeMemory(pstBuffer->pbDataBuffer);
    kFreeMemory(pstBuffer->pbFragmentBlockBitmap);
    kFreeMemory(pstBuffer);
  }
}

BOOL kIP_InitReassemblyBuffer(IPREASSEMBLYBUFFER* pstBuffer)
{
  // 버퍼 할당
  pstBuffer->pbDataBuffer = (BYTE*)kAllocateMemory(8192 * 8);
  if (pstBuffer->pbDataBuffer == NULL) {
    kPrintf("IP | kIP_InitReassemblyBuffer | Fail");
    return FALSE;
  }

  // 비트맵 할당
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

  // 재조합 버퍼 가져오기 (없으면 할당함)
  pstReassemblyBuffer = kIP_GetReassemblyBuffer(qwBufID_1, qwBufID_2);
  if (pstReassemblyBuffer == NULL) {
    kPrintf("IP | kIP_Reassembly | Fail");
    return FALSE;
  }

  dwFragmentOffset = ntohs(pstIPHeader->wFlagsAndFragmentOffset) & ((1 << IP_FLAGS_MF_SHIFT) - 1);
  dwDataLength = ntohs(pstIPHeader->wTotalLength) - (IP_INETERNETHEADERLENGTH_DEFAULT * 8);

  // 재조합 버퍼 올바른 위치에 데이터 복사
  kMemCpy(pstReassemblyBuffer->pbDataBuffer + dwFragmentOffset * 8, pbIPData, dwDataLength);

  // 비트맵 채우기
  kIP_FillReassemblyBufferBitmap(pstReassemblyBuffer, dwFragmentOffset, dwDataLength);

  // MF = 0, 마지막 패킷 인 경우
  // 총 데이터 길이 설정
  if ((ntohs(pstIPHeader->wFlagsAndFragmentOffset) & (1 << IP_FLAGS_MF_SHIFT)) == 0) {
    pstReassemblyBuffer->dwTotalLength = dwFragmentOffset * 8 + dwDataLength;
  }

  // FO = 0, 첫 번째 패킷 인 경우
  // 헤더 복사
  if (dwFragmentOffset == 0) {
    kMemCpy(&(pstReassemblyBuffer->stIPHeader), pstIPHeader, sizeof(IP_HEADER));

    // MF Flag 해제
    pstReassemblyBuffer->stIPHeader.wFlagsAndFragmentOffset = 0x0000;
  }

  // 단편화 프레임 폐기
  kFreeFrame(stFrame);

  return TRUE;
}

BOOL kIP_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_OUT;
  if (kIP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kIP_SideInPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_IN;
  if (kIP_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kIP_Initialize(void)
{
  // 뮤텍스 초기화
  kInitializeMutex(&(gs_stIPManager.stLock));

  // Allocate Frame Queue
  gs_stIPManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stIPManager.pstFrameBuffer == NULL) {
    kPrintf("kIP_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }

  // Inint Frame Queue
  kInitializeQueue(&(gs_stIPManager.stFrameQueue), gs_stIPManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // 재조합 버퍼 리스트 초기화
  kInitializeList(&(gs_stIPManager.stReassemblyBufferList));

  // 레이어 설정
  gs_stIPManager.pfDown = kEthernet_DownDirectionPoint;
  gs_stIPManager.pfUp = kStub_UpDirectionPoint;

  // 임시
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

  // 체크섬 초기화
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

BOOL kIP_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stIPManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stIPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stIPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kIP_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stIPManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stIPManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stIPManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}