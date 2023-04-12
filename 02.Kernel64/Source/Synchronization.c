#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

#if 0

BOOL kLockForSystemData(void)
{
    return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag)
{
    kSetInterruptFlag(bInterruptFlag);
}
#endif

void kInitializeMutex(MUTEX *pstMutex)
{
    pstMutex->bLockFlag = FALSE;
    pstMutex->dwLockCount = 0;
    pstMutex->qwTaskID = TASK_INVALIDID;
}

void kLock(MUTEX *pstMutex)
{
    BYTE bCurrentAPICID;
    BOOL bInterruptFlag;

    bInterruptFlag = kSetInterruptFlag(FALSE);

    bCurrentAPICID = kGetAPICID();

    if (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
    {
        if (pstMutex->qwTaskID == kGetRunningTask(bCurrentAPICID)->stLink.qwID)
        {
            kSetInterruptFlag(bInterruptFlag);
            pstMutex->dwLockCount++;
            return;
        }

        while (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
            kSchedule();
    }

    pstMutex->dwLockCount = 1;
    pstMutex->qwTaskID = kGetRunningTask(bCurrentAPICID)->stLink.qwID;
    kSetInterruptFlag(bInterruptFlag);
}

void kUnlock(MUTEX *pstMutex)
{
    BOOL bInterruptFlag;

    bInterruptFlag = kSetInterruptFlag(FALSE);

    if (pstMutex->bLockFlag == FALSE || pstMutex->qwTaskID != kGetRunningTask(kGetAPICID())->stLink.qwID)
    {
        kSetInterruptFlag(bInterruptFlag);
        return;
    }

    if (pstMutex->dwLockCount > 1)
    {
        pstMutex->dwLockCount--;
    }
    else
    {
        pstMutex->qwTaskID = TASK_INVALIDID;
        pstMutex->dwLockCount = 0;
        pstMutex->bLockFlag = FALSE;
    }

    kSetInterruptFlag(bInterruptFlag);
}

void kInitializeSpinLock(SPINLOCK *pstSpinLock)
{
    pstSpinLock->bLockFlag = FALSE;
    pstSpinLock->dwLockCount = 0;
    pstSpinLock->bAPICID = 0xFF;
    pstSpinLock->bInterruptFlag = FALSE;
}

void kLockForSpinLock(SPINLOCK *pstSpinLock)
{
    BOOL bInterruptFlag;

    bInterruptFlag = kSetInterruptFlag(FALSE);

    if (kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE)
    {
        if (pstSpinLock->bAPICID == kGetAPICID())
        {
            pstSpinLock->dwLockCount++;

            return;
        }

        while (kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE)
        {
            while (pstSpinLock->bLockFlag == TRUE)
            {
                kPause();
            }
        }
    }

    pstSpinLock->dwLockCount = 1;
    pstSpinLock->bAPICID = kGetAPICID();

    pstSpinLock->bInterruptFlag = bInterruptFlag;
}

void kUnlockForSpinLock(SPINLOCK *pstSpinLock)
{
    BOOL bInterruptFlag;

    bInterruptFlag = kSetInterruptFlag(FALSE);

    if ((pstSpinLock->bLockFlag == FALSE) || (pstSpinLock->bAPICID != kGetAPICID()))
    {
        kSetInterruptFlag(bInterruptFlag);

        return;
    }

    if (pstSpinLock->dwLockCount > 1)
    {
        pstSpinLock->dwLockCount--;

        return;
    }

    bInterruptFlag = pstSpinLock->bInterruptFlag;

    pstSpinLock->bAPICID = 0xFF;
    pstSpinLock->dwLockCount = 0;
    pstSpinLock->bInterruptFlag = FALSE;
    pstSpinLock->bLockFlag = FALSE;

    kSetInterruptFlag(bInterruptFlag);
}
