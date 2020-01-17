#ifndef __WINDOWMANAGER_H__
#define __WINDOWMANAGER_H__

#define WINDOWMANAGER_DATAACCUMULATECOUNT    20

// function
void kStartWindowManager(void);
BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);

#endif /*__WINDOWMANAGER_H__*/