#include "Task.h"
#include "Descriptor.h"
#include "Synchronization.h"

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

static void kInitializeTCBPool()
{
    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));

    gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
    kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    for(int i=0; i<TASK_MAXCOUNT; i++)
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

static TCB* kAllocateTCB()
{
    TCB* pstEmptyTCB;
    int i;

    if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount)
        return NULL;

    for(i=0; i<gs_stTCBPoolManager.iMaxCount; i++)
    {
        if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0)
        {
            pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
            break;
        }
    }

    pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if(gs_stTCBPoolManager.iAllocatedCount == 0)
        gs_stTCBPoolManager.iAllocatedCount = 1;

    return pstEmptyTCB;
}

static void kFreeTCB(QWORD qwID)
{
    int i;

    i = GETTCBOFFSET(qwID);

    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress)
{
    TCB* pstTask;
    void* pvStackAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    pstTask = kAllocateTCB();
    if(pstTask == NULL)
    {
        kUnlockForSystemData(bPreviousFlag);
        return NULL;
    }
    
    kUnlockForSystemData(bPreviousFlag);
    
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);
    
    bPreviousFlag = kLockForSystemData();
    kAddTaskToReadyList(pstTask);
    kUnlockForSystemData(bPreviousFlag);

    return pstTask;
}

static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize)
{
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD) pvStackAddress + qwStackSize;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD) pvStackAddress + qwStackSize;

    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}

void kInitializeScheduler()
{
    int i;
    kInitializeTCBPool();

    for(i=0;i<TASK_MAXREADYLISTCOUNT;i++)
    {
        kInitializeList(&(gs_stScheduler.vstReadyList[i]));
        gs_stScheduler.viExecuteCount[i] = 0;
    }

    kInitializeList(&(gs_stScheduler.stWaitList));

    gs_stScheduler.pstRunningTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;
}

void kSetRunningTask(TCB* pstTask)
{
    BOOL bPreviousFlag;
    
    bPreviousFlag = kLockForSystemData();
    gs_stScheduler.pstRunningTask = pstTask;
    kUnlockForSystemData(bPreviousFlag);
}

TCB* kGetRunningTask()
{
    BOOL bPreviousFlag;
    TCB* pstRunningTask;

    bPreviousFlag = kLockForSystemData();
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kUnlockForSystemData(bPreviousFlag);

    return pstRunningTask;
}

static TCB* kGetNextTaskToRun()
{
    TCB* pstTarget = NULL;
    int iTaskCount;

    for(int i=0; i<2; i++)
    {
        for(int j=0; j<TASK_MAXREADYLISTCOUNT; j++)
        {
            iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[j]));

            if(gs_stScheduler.viExecuteCount[j] < iTaskCount)
            {
                pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[j]));
                gs_stScheduler.viExecuteCount[j]++;
                break;
            }
            else
                gs_stScheduler.viExecuteCount[j] = 0;
        }

        if(pstTarget != NULL)
            break;
    }

    return pstTarget;
}

static BOOL kAddTaskToReadyList(TCB* pstTask)
{
    BYTE bPriority;

    bPriority = GETPRIORITY(pstTask->qwFlags);
    
    if(bPriority >= TASK_MAXREADYLISTCOUNT)
        return FALSE;

    kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
    return TRUE;
}

static TCB* kRemoveTaskFromReadyList(QWORD qwTASKID)
{
    TCB* pstTarget;
    BYTE bPriority;

    if(GETTCBOFFSET(qwTASKID) >= TASK_MAXCOUNT)
        return NULL;

    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTASKID)]);

    if(pstTarget->stLink.qwID != qwTASKID)
        return NULL;

    bPriority = GETPRIORITY(pstTarget->qwFlags);

    pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTASKID);

    return pstTarget;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority)
{
    TCB* pstTarget;
    BOOL bPreviousFlag;

    if(bPriority > TASK_MAXREADYLISTCOUNT)
        return FALSE;


    bPreviousFlag = kLockForSystemData();

    pstTarget = gs_stScheduler.pstRunningTask;

    if(pstTarget->stLink.qwID == qwTaskID)
        SETPRIORITY(pstTarget->qwFlags, bPriority);
    else
    {
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == NULL)
        {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL)
                SETPRIORITY(pstTarget->qwFlags, bPriority);
        } 
        else
        {
            SETPRIORITY(pstTarget->qwFlags, bPriority);
            kAddTaskToReadyList(pstTarget);
        }
    }

    kUnlockForSystemData(bPreviousFlag);

    return TRUE;
}

void kSchedule()
{
    TCB* pstRunningTask, *pstNextTask;
    BOOL bPreviousFlag;

    if(kGetReadyTaskCount() < 1)
        return ;

    bPreviousFlag = kLockForSystemData();

    pstNextTask = kGetNextTaskToRun();

    if(pstNextTask == NULL)
    {
        kUnlockForSystemData(bPreviousFlag);
        return ;
    }

    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
    {
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
        kSwitchContext(NULL, &(pstNextTask->stContext));
    } 
    else
    {
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    kUnlockForSystemData(bPreviousFlag);
}

BOOL kScheduleInInterrupt()
{
    TCB* pstRunningTask, *pstNextTask;
    char* pcContextAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL)
    {
        kUnlockForSystemData(bPreviousFlag);
        return FALSE;
    }

    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;

    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    else
    {
        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }

    kUnlockForSystemData(bPreviousFlag);

    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME; // CHECK?

    return TRUE;
}

void kDecreaseProcessorTime()
{
    if(gs_stScheduler.iProcessorTime > 0)
        gs_stScheduler.iProcessorTime--;
}

BOOL kIsProcessorTimeExpired()
{
    if(gs_stScheduler.iProcessorTime <= 0)
        return TRUE;

    return FALSE;
}

BOOL kEndTask(QWORD qwTaskID)
{
    TCB* pstTarget;
    BYTE bPriority;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID)
    {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSystemData(bPreviousFlag);

        kSchedule();

        while(1);
    } 
    else
    {
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == NULL)
        {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            
            if(pstTarget != NULL)
            {
                pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
                SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
            }

            kUnlockForSystemData(bPreviousFlag);

            return TRUE; // CHECK
        }

        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
    }

    kUnlockForSystemData(bPreviousFlag);
    return TRUE;
}

void kExitTask()
{
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount()
{
    int iTotalCount = 0;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    for(int i=0; i<TASK_MAXREADYLISTCOUNT; i++)
    {
        iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    }

    kUnlockForSystemData(bPreviousFlag);

    return iTotalCount;
}

int kGetTaskCount()
{
    int iTotalCount;
    BOOL bPreviousFlag;

    iTotalCount = kGetReadyTaskCount();

    bPreviousFlag = kLockForSystemData();
    
    iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

    kUnlockForSystemData(bPreviousFlag);

    return iTotalCount;
}

TCB* kGetTCBInTCBPool(int iOffset)
{
    if((iOffset < -1) || (iOffset > TASK_MAXCOUNT))
        return NULL;

    return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

BOOL kIsTaskExist(QWORD qwID)
{
    TCB* pstTCB;

    pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

    if((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID))
        return FALSE;

    return TRUE;
}

QWORD kGetProcessorLoad()
{
    return gs_stScheduler.qwProcessorLoad;
}

void kIdleTask()
{
    TCB* pstTask;
    QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
    QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
    QWORD qwTaskID;
    BOOL bPreviousFlag;

    qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while(TRUE)
    {
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
            gs_stScheduler.qwProcessorLoad = 0;
        else 
            gs_stScheduler.qwProcessorLoad = 100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);

        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        kHaltProcessorByLoad();

        if(kGetListCount(&(gs_stScheduler.stWaitList)) >= 0)
        {
            while(1)
            {
                bPreviousFlag = kLockForSystemData();
                pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));

                if(pstTask == NULL)
                {
                    kUnlockForSystemData(bPreviousFlag);
                    break;
                }

                qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(qwTaskID);

                kUnlockForSystemData(bPreviousFlag);

                kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", pstTask->stLink.qwID);
                kFreeTCB(pstTask->stLink.qwID);
            }
        }

        kSchedule();
    }
}

void kHaltProcessorByLoad()
{
    if(gs_stScheduler.qwProcessorLoad < 40)
    {
        kHlt();
        kHlt();
        kHlt();
    } 
    else if(gs_stScheduler.qwProcessorLoad < 80)
    {
        kHlt();
        kHlt();
    } else if(gs_stScheduler.qwProcessorLoad < 95)
    {
        kHlt();
    }
}