#include "Mouse.h"
#include "Keyboard.h"
#include "Queue.h"
#include "AssemblyUtility.h"

static MOUSEMANAGER gs_stMouseManager = {0, };

static QUEUE gs_stMouseQueue;
static MOUSEDATA gs_vstMouseQueueBuffer[MOUSE_MAXQUEUECOUNT];

BOOL kInitializeMouse(void)
{
    kInitializeQueue(&gs_stMouseQueue, gs_vstMouseQueueBuffer, MOUSE_MAXQUEUECOUNT, sizeof(MOUSEDATA));

    kInitializeSpinLock(&(gs_stMouseManager.stSpinLock));

    if(kActivateMouse() == TRUE)
    {
        kEnableMouseInterrupt();
        
        return TRUE;
    }

    return FALSE;
}

BOOL kActivateMouse(void)
{
    int i, j;
    BOOL bPreviousInterrupt, bResult;

    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    kOutPortByte(0x64, 0xA8);
    kOutPortByte(0x64, 0xD4);

    for(i = 0; i < 0xFFFF; i++)
    {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    kOutPortByte(0x60, 0xF4);

    bResult = kWaitForACKAndPutOtherScanCode();

    kSetInterruptFlag(bPreviousInterrupt);

    return bResult;
}

void kEnableMouseInterrupt(void)
{
    BYTE bOutputPortData;

    int i;

    kOutPortByte(0x64, 0x20);

    for(i = 0; i < 0xFFFF; i++)
    {
        if(kIsOutputBufferFull() == TRUE)
            break;
    }

    bOutputPortData = kInPortByte(0x60);

    bOutputPortData |= 0x02;

    kOutPortByte(0x64, 0x60);

    for(i = 0; i < 0xFFFF; i++)
    {
        if(kIsInputBufferFull() == FALSE)
            break;
    }

    kOutPortByte(0x60, bOutputPortData);
}

BOOL kAccumulateMouseDataAndPutQueue(BYTE bMouseData)
{
    BOOL bResult;

    switch (gs_stMouseManager.iByteCount)
    {
    case 0:
        gs_stMouseManager.stCurrentData.bButtonStatusAndFlag = bMouseData;
        gs_stMouseManager.iByteCount++;
        break;
    case 1:
        gs_stMouseManager.stCurrentData.bXMovement = bMouseData;
        gs_stMouseManager.iByteCount++;
        break;
    case 2:
        gs_stMouseManager.stCurrentData.bYMovement = bMouseData;
        gs_stMouseManager.iByteCount++;
        break;
    default:
        gs_stMouseManager.iByteCount = 0;
        break;
    }

    if(gs_stMouseManager.iByteCount >= 3)
    {
        kLockForSpinLock(&(gs_stMouseManager.stSpinLock));

        bResult = kPutQueue(&gs_stMouseQueue, &gs_stMouseManager.stCurrentData);

        kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));

        gs_stMouseManager.iByteCount = 0;
    }

    return bResult;
}

BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX, int* piRelativeY)
{
    MOUSEDATA stData;
    BOOL bResult;

    if(kIsQueueEmpty(&(gs_stMouseQueue)) == TRUE)
        return FALSE;
    
    kLockForSpinLock(&(gs_stMouseManager.stSpinLock));

    bResult = kGetQueue(&(gs_stMouseQueue), &stData);

    kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));

    if(bResult == FALSE)
        return FALSE;

    *pbButtonStatus = stData.bButtonStatusAndFlag & 0x7;

    *piRelativeX = stData.bXMovement & 0xFF;
    if(stData.bButtonStatusAndFlag & 0x10)
        *piRelativeX |= (0xFFFFFF00);

    *piRelativeY = stData.bYMovement & 0xFF;   
    if(stData.bButtonStatusAndFlag & 0x20)
        *piRelativeY |= (0xFFFFFF00);
    
    *piRelativeY = -*piRelativeY;

    return TRUE;
}

BOOL kIsMouseDataInOutputBuffer(void)
{
    if(kInPortByte(0x64) & 0x20)
        return TRUE;
    
    return FALSE;
}