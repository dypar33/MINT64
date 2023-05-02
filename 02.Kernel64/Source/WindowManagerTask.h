#ifndef __WINDOWMANAGER_H__
#define __WINDOWMANAGER_H__

#define WINDOWMANAGER_DATAACCUMULATECOUNT 20

void kStartWindowManager(void);
BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);

#endif