#include "DNS.h"
#include "UDP.h"
#include "IP.h"
#include "Utility.h"
#include "DynamicMemory.h"

static DNSMANAGER gs_stDNSManager = { 0, };

DNS_RETURN kDNS_GetAddress(const BYTE* pbName, BYTE* pbAddress)
{
  volatile BYTE bFlag = 0;
  DNS_REQUEST stRequest;
  stRequest.pbName = pbName;
  stRequest.pbOut = pbAddress;
  stRequest.pbFlag = &bFlag;
  stRequest.bTryCount = 5;
  // �ִ� 2.5�ʰ� ���
  stRequest.qwTime = kGetTickCount() + 2500;

  // Put into Request Queue
  kDNS_PutRequest(&stRequest);

  // ���
  while (bFlag == 0);

  return bFlag;
}

void kDNS_ProcessRequest(void)
{
  int i, num;
  DNS_REQUEST stRequest;
  DNS_ENTRY* pstEntry;
  QWORD qwTime;

  qwTime = kGetTickCount();

  // ���μ����� ���� �� ��� ��û�� ó��
  num = kGetQueueSize(&(gs_stDNSManager.stRequestQueue));
  for (i = 0; i < num; i++)
  {
    // ó�� �� ��û�� ���� ���
    if (kDNS_GetRequest(&stRequest) == FALSE) {
      return;
    }
    
    // �ð��� ����� ��û�� �н� ��
    if (qwTime > stRequest.qwTime) {
      *(stRequest.pbFlag) = DNS_ERROR;
      continue;
    }

    // ���� DNS ĳ�ø� Ȯ��
    // ĳ�ÿ� �ִ� ���
    pstEntry = kDNSCache_Get(stRequest.pbName);
    if (pstEntry != NULL) {
      // �ּ�
      kMemCpy(stRequest.pbOut, pstEntry->vbAddress, 4);
      *(stRequest.pbFlag) = DNS_NOERROR;
    }
    // ĳ�ÿ� ���� ���
    else {

      // 500ms �������� ��û�� ������ ��.
      if (stRequest.qwTime < (stRequest.bTryCount * DNS_CACHE_RETRY_PERIOD + qwTime)) {
        stRequest.bTryCount--;
        // DNS ��û�� ���� ��.
        kDNS_SendDNSQuery(stRequest.pbName);
      }

      // ������ �ٽ� ó���ϱ� ���� ť�� �ٽ� ����
      kDNS_PutRequest(&stRequest);
    }
  }
}

WORD kDNS_ConvertAddressToDotFormat(BYTE* pbIn, BYTE* pbOut)
{
  // 03 77 77 77 (www)
  // 06 67 6f 6f 67 6c 65 (google)
  // 03 63 6f 6d (com)
  // 00 (ROOT)
  // ---------->
  // www.google.com 

  int i;
  WORD wLen = 0;
  WORD wIndex = 0;

  BYTE* pbCur = pbIn;
  while (1) {
    if (*pbCur <= 64) {
      wLen = *pbCur;

      // ���� �� ��ŭ ���ڿ� ����
      pbCur++;
      for (i = 0; i < wLen; i++) {
        pbOut[wIndex++] = *pbCur;
        pbCur++;
      }

      // ROOT
      if (wLen == 0)
        break;

      pbOut[wIndex++] = '.';
    }
  }
  wIndex--;
  pbOut[wIndex] = 0;

  return wIndex;
}

WORD kDNS_ConvertAddressToDNSFormat(BYTE* pbIn, BYTE* pbOut)
{
  // Name (Var)
  // www.google.com
  // ---------->
  // 03 77 77 77 (www)
  // 06 67 6f 6f 67 6c 65 (google)
  // 03 63 6f 6d (com)
  // 00 (ROOT)

  int i;
  WORD wLen = 0;
  WORD wIndex = 0;

  BYTE* pbCur = pbIn;
  while (1) {
    // ������ ���ڿ��� Ž���Ͽ� ������(.)�� ã��.
    if ((*pbCur == '.') ||
      (*pbCur == 0))
    {
      // ���� �� ���
      pbOut[wIndex++] = wLen;

      // ���� �� ��ŭ ���ڿ� ����
      for (i = wLen; i > 0; i--) {
        pbOut[wIndex++] = *(pbCur - i);
      }

      // ī���� �ʱ�ȭ
      wLen = 0;

      // �������� 0 (Root) �߰�
      if (*pbCur == 0) {
        pbOut[wIndex++] = 0;
        break;
      }
    }
    // ������ �տ� ��ġ�� ���� ���� ī���� ��.
    else {
      wLen++;
    }
    pbCur++;
  }

  return wIndex;
}

WORD kDNS_AddRR(DNS_QUERYRR* pstRR, BYTE* pbBuf, WORD wIndex)
{
  WORD wLen;

  // Name (variable)
  wLen = kDNS_ConvertAddressToDNSFormat(pstRR->pbName, pbBuf + wIndex);
  wIndex += wLen;

  // Type(2), Class(2)
  kMemCpy(pbBuf + wIndex, &pstRR->wType, 2 + 2);
  return  wIndex + 2 + 2;
}

void kDNS_GetQueryRR(DNS_QUERYRR* pstRR, BYTE* pbPayload)
{
  int len = kStrLen(pbPayload) + 1;

  pstRR->pbName = pbPayload;
  kMemCpy((BYTE*)&(pstRR->wType), pbPayload + len, 4);
}

DNS_ANSWERRR* kDNS_GetAnswerRR(DNS_HEADER* pstHeader, BYTE* pbPayload, WORD wIndex)
{
  int i;
  DNS_QUERYRR* pstQueryRR;
  DNS_ANSWERRR* pstAnswerRR;
  BYTE* pbTemp;
  WORD wOffset;
  // Query
  pstQueryRR = pbPayload;

  // Answer Begin
  pstAnswerRR = pbPayload + kStrLen((BYTE*)pstQueryRR) + 1 + 4;

  for (i = 1; i <= wIndex; i++) {
    pstAnswerRR = ((BYTE*)&(pstAnswerRR->pbRData)) + ntohs(pstAnswerRR->wRDLength);
  }
  return pstAnswerRR;
}

void kDNS_SendDNSQuery(const BYTE* pbName)
{
  FRAME stFrame;
  DNS_HEADER stDNSHeader;
  DNS_QUERYRR stDNSQueryRR;
  BYTE vbPayload[512];
  WORD wIndex = 0;

  // ��� ����
  stDNSHeader.wTransactionID = htons(kRandom() % 0xFFFFFFFF);
  stDNSHeader.wFlags = htons(
    DNS_FLAGS_QUERYRESPOSE_QUERY << DNS_FLAGS_QUERYRESPOSE_OFFSET |
    DNS_FLAGS_OPCODE_QUERY << DNS_FLAGS_OPCODE_OFFSET |
    1 << DNS_FLAGS_RECURSIONDESIRED_OFFSET
  );

  stDNSHeader.wQuestion = htons(0x0001);
  stDNSHeader.wAnswer = 0x0000;
  stDNSHeader.wAuthority = 0x0000;
  stDNSHeader.wAdditional = 0x0000;

  // ���� ����
  stDNSQueryRR.pbName = pbName;
  stDNSQueryRR.wType = htons(DNS_RR_TYPE_A);
  stDNSQueryRR.wClass = htons(DNS_RR_CLASS_IN);
  wIndex = kDNS_AddRR(&stDNSQueryRR, vbPayload, wIndex);

  // ĸ��ȭ
  kAllocateFrame(&stFrame);
  kEncapuslationFrame(&stFrame, &stDNSHeader, sizeof(DNS_HEADER), vbPayload, wIndex);

  // IP�� ����
  stFrame.bType = FRAME_DNS;
  // Source : My IP, Dest : DNS Server IP
  stFrame.qwDestAddress = ((QWORD)kIP_GetIPAddress(NULL) << 32) | kAddressArrayToNumber(gs_stDNSManager.vbDNSAddress, 4);
  stFrame.dwDestPort = ((kRandom() & 0xFFFF) << 16) | UDP_PORT_DNS;
  stFrame.eDirection = FRAME_DOWN;

  kDNS_PutFrameToFrameQueue(&stFrame);
}

BOOL kDNS_SetDNSAddress(BYTE* pbAddress)
{
  kMemCpy(gs_stDNSManager.vbDNSAddress, pbAddress, 4);
  return TRUE;
}

void kDNS_Task(void)
{
  DNS_HEADER* pstDNSHeader;
  DNS_QUERYRR stDNSQueryRR;
  DNS_ANSWERRR* pstAnswerRR;
  DNS_ENTRY* pstCacheEntry;
  FRAME stFrame;

  BYTE* pbDNSPayload;
  BYTE vbName[256];
  BYTE bNameLen;

  int i;

  // 5�ʸ��� ĳ�� ���̺� ����
  QWORD qwCleanTime = kGetTickCount() + DNS_CACHE_CLEAN_PERIOD;
  QWORD qwTime;

  // �ʱ�ȭ
  if (kDNS_Initialize() == FALSE)
    return;

  while (1)
  {
    // ĳ�� ���̺� ����
    qwTime = kGetTickCount();
    if (qwCleanTime <= qwTime) {
      kDNSCache_Clean();
      qwCleanTime = qwTime + DNS_CACHE_CLEAN_PERIOD;
    }

    // ���μ����� ���� �� ��û ó��
    kDNS_ProcessRequest();

    if (kDNS_GetFrameFromFrameQueue(&stFrame) == FALSE) {
      continue;
    }

    switch (stFrame.eDirection)
    {
    case FRAME_DOWN:
      kPrintf("DNS | Send DNS Message\n");
      gs_stDNSManager.pfDownUDP(stFrame);
      break;
    case FRAME_UP:
      kPrintf("DNS | Recevie DNS Message\n");
      kDecapuslationFrame(&stFrame, &pstDNSHeader, sizeof(DNS_HEADER), &pbDNSPayload);

      // DNS ���� �޽��� ó��
      // ���� Ȯ��
      if ((pstDNSHeader->wFlags & 0x000F) != 0) {
        // Query Name 
        kDNS_GetQueryRR(&stDNSQueryRR, pbDNSPayload);
        bNameLen = kDNS_ConvertAddressToDotFormat(stDNSQueryRR.pbName, vbName);

        // Answer Address ȹ��
        for (i = 0; i < ntohs(pstDNSHeader->wAnswer); i++) {
          pstAnswerRR = kDNS_GetAnswerRR(pstDNSHeader, pbDNSPayload, i);

          switch (ntohs(pstAnswerRR->wType))
          {
          case DNS_RR_TYPE_A:
            // ĳ�� Ȯ��
            pstCacheEntry = kDNSCache_Get(vbName);
            // ĳ�� �ִ� ���
            if (pstCacheEntry != NULL) {
              // ĳ�� ����
              kMemCpy(pstCacheEntry->vbAddress, &(pstAnswerRR->pbRData), 4);
              pstCacheEntry->qwTime = kGetTickCount();
              pstCacheEntry->dwTTL = htonl(pstAnswerRR->dwTTL) * 1000;
            }
            // ĳ�� ���� ���
            else {
              // TODO : �����Ҵ��� �ƴ� POOL ���·� ����
              pstCacheEntry = (DNS_ENTRY*)kAllocateMemory(sizeof(DNS_ENTRY));
              if (pstCacheEntry == NULL) {
                return FALSE;
              }

              // ĳ�� �߰�
              pstCacheEntry->stEntryLink.qwID = HASH(vbName, bNameLen);
              kMemCpy(pstCacheEntry->vbName, vbName, bNameLen);
              pstCacheEntry->vbName[bNameLen] = '\0';
              kMemCpy(pstCacheEntry->vbAddress, &(pstAnswerRR->pbRData), 4);
              pstCacheEntry->qwTime = kGetTickCount();
              pstCacheEntry->dwTTL = htonl(pstAnswerRR->dwTTL) * 1000;
              kDNSCache_Put(pstCacheEntry);
            }
            i = 100; // tmp 
            break;
          case DNS_RR_TYPE_CNAME:
            break;
          case DNS_RR_TYPE_NS:
            break;
          default:
            break;
          }
        }
      }

      kFreeFrame(&stFrame);
      break;
    }
  }
}

BOOL kDNS_Initialize(void)
{
  int i;

  // ���ؽ� �ʱ�ȭ
  kInitializeMutex(&(gs_stDNSManager.stLock));

  // ������ ť �Ҵ�
  gs_stDNSManager.pstFrameBuffer = (FRAME*)kAllocateMemory(FRAME_QUEUE_MAX_COUNT * sizeof(FRAME));
  if (gs_stDNSManager.pstFrameBuffer == NULL) {
    kPrintf("kDNS_Initialize | FrameBuffer Allocate Fail\n");
    return FALSE;
  }
  // ������ ť �ʱ�ȭ
  kInitializeQueue(&(gs_stDNSManager.stFrameQueue), gs_stDNSManager.pstFrameBuffer, FRAME_QUEUE_MAX_COUNT, sizeof(FRAME));

  // ��û ť �Ҵ�
  gs_stDNSManager.pstRequestBuffer = (DNS_REQUEST*)kAllocateMemory(DNS_REQUEST_QUEUE_MAX_COUNT * sizeof(DNS_REQUEST));
  if (gs_stDNSManager.pstRequestBuffer == NULL) {
    kPrintf("kDNS_Initialize | RequestBuffer Allocate Fail\n");
    kFreeMemory(gs_stDNSManager.pstFrameBuffer);
    return FALSE;
  }
  // ��û ť �ʱ�ȭ
  kInitializeQueue(&(gs_stDNSManager.stRequestQueue), gs_stDNSManager.pstRequestBuffer, DNS_REQUEST_QUEUE_MAX_COUNT, sizeof(DNS_REQUEST));

  // ĳ�� �ʱ�ȭ
  for (i = 0; i < DNS_CACHE_INDEX_MAX_COUNT; i++) {
    kInitializeList(&(gs_stDNSManager.stCache.vstEntryList[i]));
  }

  // ���̾� ����
  gs_stDNSManager.pfDownUDP = kUDP_DownDirectionPoint;

  return TRUE;
}

void kDNSCache_Put(DNS_ENTRY* pstEntry)
{
  BYTE dwIndex = pstEntry->stEntryLink.qwID;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  kAddListToHeader(&(gs_stDNSManager.stCache.vstEntryList[dwIndex]), &(pstEntry->stEntryLink));

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---
}

DNS_ENTRY* kDNSCache_Get(BYTE* pbName)
{
  DNS_ENTRY* pstEntry;
  const LIST* pstList;
  LISTLINK* pstLink;

  BYTE bCmpNameLen = 0;
  BYTE bNameLen = kStrLen(pbName);
  DWORD dwKey = HASH(pbName, bNameLen);
  BYTE dwIndex = dwKey;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  // Ž��
  pstList = &(gs_stDNSManager.stCache.vstEntryList[dwIndex]);
  for (pstLink = (LISTLINK*)pstList->pvHeader; pstLink != NULL; pstLink = pstLink->pvNext) {
    if (pstLink->qwID == dwKey) {
      pstEntry = (DNS_ENTRY*)pstLink;

      bCmpNameLen = kStrLen(pstEntry->vbName);
      // ���̰� ���� ���ڿ��� ������ ��� : ĳ�� �׸� �߰�
      if ((bCmpNameLen == bNameLen) &&
        (kMemCmp(pstEntry->vbName, pbName, bNameLen) == 0))
      {
        kUnlock(&(gs_stDNSManager.stLock));
        // --- CRITCAL SECTION END ---

        return pstEntry;
      }
    }
  }

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return NULL;
}

void kDNSCache_Print(void)
{
  int i;
  LIST* pstList;
  LISTLINK* pstLink;
  DNS_ENTRY* pstEntry;


  kPrintf("\nName\tAddress\tTTL\n");


  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  // Ž��
  for (i = 0; i < DNS_CACHE_INDEX_MAX_COUNT; i++) {
    pstList = &(gs_stDNSManager.stCache.vstEntryList[i]);
    for (pstLink = (LISTLINK*)pstList->pvHeader; pstLink != NULL; pstLink = pstLink->pvNext) {
      pstEntry = (DNS_ENTRY*)pstLink;

      kPrintf("%s\t", pstEntry->vbName);
      kPrintIPAddress(pstEntry->vbAddress);
      kPrintf("\t%d\n", pstEntry->dwTTL);
    }
  }

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

}

void kDNSCache_Clean(void)
{
  int i;
  LIST* pstList;
  LISTLINK* pstLink;
  LISTLINK* pstPreviousLink;
  DNS_ENTRY* pstEntry;
  QWORD qwTime;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  qwTime = kGetTickCount();
  for (i = 0; i < DNS_CACHE_INDEX_MAX_COUNT; i++) {
    pstList = &(gs_stDNSManager.stCache.vstEntryList[i]);

    pstPreviousLink = (LISTLINK*)pstList->pvHeader;

    for (pstLink = pstPreviousLink; pstLink != NULL; pstLink = pstLink->pvNext) {
      pstEntry = (DNS_ENTRY*)pstLink;

      // TTL ����� ���
      if ((pstEntry->qwTime + pstEntry->dwTTL) <= qwTime) {
        // only node
        if ((pstLink == pstList->pvHeader) &&
          (pstLink == pstList->pvTail)) {
          pstList->pvHeader = NULL;
          pstList->pvTail = NULL;
        }
        // if first node
        else if (pstLink == pstList->pvHeader) {
          pstList->pvHeader = pstLink->pvNext;
        }
        // if last node
        else if (pstLink == pstList->pvTail) {
          pstList->pvTail = pstPreviousLink;
        }
        // middle
        else {
          pstPreviousLink->pvNext = pstLink->pvNext;
        }
        pstList->iItemCount--;

        // ��Ʈ�� ����
        kFreeMemory(pstEntry);
      }
      // goto next node
      pstPreviousLink = pstLink;
    }
  }

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---
}

BOOL kDNS_UpDirectionPoint(FRAME stFrame)
{
  stFrame.eDirection = FRAME_UP;
  if (kDNS_PutFrameToFrameQueue(&stFrame) == FALSE)
    return FALSE;
  return TRUE;
}

BOOL kDNS_PutRequest(const DNS_REQUEST* pstRequest)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  bResult = kPutQueue(&(gs_stDNSManager.stRequestQueue), pstRequest);

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kDNS_GetRequest(DNS_REQUEST* pstRequest)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  bResult = kGetQueue(&(gs_stDNSManager.stRequestQueue), pstRequest);

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kDNS_PutFrameToFrameQueue(const FRAME* pstFrame)
{
  BOOL bResult;

  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  // Put into FrameQueue
  bResult = kPutQueue(&(gs_stDNSManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}

BOOL kDNS_GetFrameFromFrameQueue(FRAME* pstFrame)
{
  BOOL bResult;
  // --- CRITCAL SECTION BEGIN ---
  kLock(&(gs_stDNSManager.stLock));

  // Get from FrameQueue
  bResult = kGetQueue(&(gs_stDNSManager.stFrameQueue), pstFrame);

  kUnlock(&(gs_stDNSManager.stLock));
  // --- CRITCAL SECTION END ---

  return bResult;
}