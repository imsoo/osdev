#include "List.h"

/*
  Init List
*/
void kInitalizeList(LIST* pstList)
{
  pstList->iItemCount = 0;
  pstList->pvHeader = NULL;
  pstList->pvTail = NULL;
}

/*
  Return List count
*/
int kGetListCount(const LIST* pstList)
{
  return pstList->iItemCount;
}

/*
  Add List to Tail
*/
void kAddListToTail(LIST* pstList, void* pvItem)
{
  LISTLINK* pstLink;

  pstLink = (LISTLINK*)pvItem;
  pstLink->pvNext = NULL;

  // empty list
  if (pstList->pvHeader == NULL) {
    pstList->pvHeader = pvItem;
    pstList->pvTail = pvItem;
    pstList->iItemCount = 1;
    return;
  }

  // else add to tail
  pstLink = (LISTLINK*)pstList->pvTail;
  pstLink->pvNext = pvItem;

  pstList->pvTail = pvItem;
  pstList->iItemCount++;
}

/*
  Add List to Header
*/
void kAddListToHeader(LIST* pstList, void* pvItem)
{
  LISTLINK* pstLink;

  pstLink = (LISTLINK*)pvItem;
  pstLink->pvNext = pstList->pvHeader;

  // empty list
  if (pstList->pvHeader == NULL) {
    pstList->pvHeader = pvItem;
    pstList->pvTail = pvItem;
    pstList->iItemCount = 1;

    return;
  }

  pstList->pvHeader = pstLink;
  pstList->iItemCount++;
}

/*
  Remove List
*/
void* kRemoveList(LIST* pstList, QWORD qwID)
{
  LISTLINK* pstLink;
  LISTLINK* pstPreviousLink;

  pstPreviousLink = (LISTLINK*)pstList->pvHeader;
  
  for (pstLink = pstPreviousLink; pstLink != NULL; pstLink = pstLink->pvNext) {
    if (pstLink->qwID == qwID) {

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
      return pstLink;
    }
    // goto next node
    pstPreviousLink = pstLink;
  }
  // not found
  return NULL;
}

/*
  Remove List From Header
*/
void* kRemoveListFromHeader(LIST* pstList)
{
  LISTLINK* pstLink;

  // nothing 
  if (pstList->iItemCount == 0) {
    return NULL;
  }

  // remove header and return
  pstLink = (LISTLINK*)pstList->pvHeader;
  return kRemoveList(pstList, pstLink->qwID);
}

/*
  Remove List From Tail
*/
void* kRemoveListFromTail(LIST* pstList)
{
  LISTLINK* pstLink;

  // nothing 
  if (pstList->iItemCount == 0) {
    return NULL;
  }

  // remove tail and return
  pstLink = (LISTLINK*)pstList->pvTail;
  return kRemoveList(pstList, pstLink->qwID);
}

/*
  Find List
*/
void* kFindList(const LIST* pstList, QWORD qwID)
{
  LISTLINK* pstLink;

  for (pstLink = (LISTLINK*)pstList->pvHeader; pstLink != NULL; pstLink = pstLink->pvNext) {
    if (pstLink->qwID == qwID) {
      return pstLink;
    }
  }
  return NULL;
}

/*
  Get Header
*/
void* kGetHeaderFromList(const LIST* pstList)
{
  return pstList->pvHeader;
}

/*
  Get Tail
*/
void* kGetTailFromList(const LIST* pstList)
{
  return pstList->pvTail;
}

/*
  Get Next
*/
void* kGetNextFromList(const LIST* pstList, void* pstCurrent)
{
  LISTLINK* pstLink;
  pstLink = (LISTLINK*)pstCurrent;

  return pstLink->pvNext;
}