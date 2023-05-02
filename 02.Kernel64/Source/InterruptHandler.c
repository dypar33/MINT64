#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"
#include "Mouse.h"

static INTERRUPTMANAGER gs_stInterruptManager;

void kInitializeHandler()
{
    kMemSet(&gs_stInterruptManager, 0, sizeof(gs_stInterruptManager));
}

void kSetSymmetricIOMode(BOOL bSymmetricIOMode)
{
    gs_stInterruptManager.bSymmetricIOMode = bSymmetricIOMode;
}

void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing)
{
    gs_stInterruptManager.bUseLoadBalancing = bUseLoadBalancing;
}

void kIncreaseInterruptCount(int iIRQ)
{
    gs_stInterruptManager.vvqwCoreInterruptCount[kGetAPICID()][iIRQ]++;
}

void kSendEOI(int iIRQ)
{
    if (gs_stInterruptManager.bSymmetricIOMode == FALSE)
    {
        kSendEOIToPIC(iIRQ);
    }
    else
    {
        kSendEOIToLocalAPIC();
    }
}

INTERRUPTMANAGER *kGetInterruptManager(void)
{
    return &gs_stInterruptManager;
}

void kProcessLoadBalancing(int iIRQ)
{
    QWORD qwMinCount = 0xFFFFFFFFFFFFFFFF;

    int iMinCountCoreIndex;
    int iCoreCount;
    int i;
    BOOL bResetCount = FALSE;
    BYTE bAPICID;

    bAPICID = kGetAPICID();

    if ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] == 0) ||
        ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0) ||
        (gs_stInterruptManager.bUseLoadBalancing == FALSE))
    {
        return;
    }

    iMinCountCoreIndex = 0;
    iCoreCount = kGetProcessorCount();
    for (i = 0; i < iCoreCount; i++)
    {
        if ((gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] < qwMinCount))
        {
            qwMinCount = gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ];

            iMinCountCoreIndex = i;
        }
        else if (gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] >= 0xFFFFFFFFFFFFFFFE)
        {
            bResetCount = TRUE;
        }
    }

    kRoutingIRQToAPICID(iIRQ, iMinCountCoreIndex);

    if (bResetCount == TRUE)
    {
        for (i = 0; i < iCoreCount; i++)
        {
            gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] = 0;
        }
    }
}

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode)
{
    char vcBuffer[3] = {
        0,
    };

    kPrintStringXY(0, 0, "===================================================");
    kPrintStringXY(0, 1, "                 Exception Occured!!               ");
    kPrintStringXY(0, 2, "               Vector:        Core ID:             ");

    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;
    kPrintStringXY(21, 2, vcBuffer);
    kSPrintf(vcBuffer, "0x%X", kGetAPICID());
    kPrintStringXY(40, 2, vcBuffer);
    kPrintStringXY(0, 3, "===================================================");

    while (1)
        ;
}

void kCommonInterruptHandler(int iVectorNumber)
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer);

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}

void kKeyboardHandler(int iVectorNumber)
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    if (kIsOutputBufferFull() == TRUE)
    {
        if(kIsMouseDataInOutputBuffer() == FALSE)
        {
            bTemp = kGetKeyboardScanCode();
            kConvertScanCodeAndPutQueue(bTemp);
        }
        else
        {
            bTemp = kGetKeyboardScanCode();
            kAccumulateMouseDataAndPutQueue(bTemp);
        }
    }

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}

void kTimerHandler(int iVectorNumber)
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;
    int iIRQ;
    BYTE bCurrentAPICID;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    vcBuffer[8] = '0' + g_iTimerInterruptCount;
    g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer);

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);

    bCurrentAPICID = kGetAPICID();
    if (kGetAPICID() == 0)
    {
        g_qwTickCount++;
    }

    kDecreaseProcessorTime(bCurrentAPICID);

    if (kIsProcessorTimeExpired(bCurrentAPICID) == TRUE)
    {
        kScheduleInInterrupt();
    }
}

void kDeviceNotAvailableHandler(int iVectorNumber)
{
    TCB *pstFPUTask, *pstCurrentTask;
    QWORD qwLastFPUTaskID;
    BYTE bCurrentAPICID;

    char vcBuffer[] = "[EXC:  , ]";
    static int g_iFPUInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iFPUInterruptCount;

    g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;

    kPrintStringXY(0, 0, vcBuffer);

    bCurrentAPICID = kGetAPICID();

    kClearTS();

    qwLastFPUTaskID = kGetLastFPUUsedTaskID(bCurrentAPICID);
    pstCurrentTask = kGetRunningTask(bCurrentAPICID);

    if (qwLastFPUTaskID == pstCurrentTask->stLink.qwID)
        return;
    else if (qwLastFPUTaskID != TASK_INVALIDID)
    {
        pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
        if (pstFPUTask != NULL && pstFPUTask->stLink.qwID == qwLastFPUTaskID)
            kSaveFPUContext(pstFPUTask->vqwFPUContext);
    }

    if (pstCurrentTask->bFPUUsed == FALSE)
    {
        kInitializeFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    }
    else
        kLoadFPUContext(pstCurrentTask->vqwFPUContext);

    kSetLastFPUUsedTaskID(bCurrentAPICID, pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber)
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iHDDInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iHDDInterruptCount;
    g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;

    kPrintStringXY(10, 0, vcBuffer);

    kSetHDDInterruptFlag(TRUE, TRUE);

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}

void kMouseHandler(int iVectorNumber)
{
    char vcBuffer[] = "[INT:  , ]";
    static int g_iMouseInterruptCount = 0;
    BYTE bTemp;
    int iIRQ;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iMouseInterruptCount;
    g_iMouseInterruptCount = (g_iMouseInterruptCount + 1) % 10;
    kPrintStringXY(0, 0, vcBuffer);

    if (kIsOutputBufferFull() == TRUE)
    {
        if(kIsMouseDataInOutputBuffer() == FALSE)
        {
            bTemp = kGetKeyboardScanCode();
            kConvertScanCodeAndPutQueue(bTemp);
        }
        else
        {
            bTemp = kGetKeyboardScanCode();
            kAccumulateMouseDataAndPutQueue(bTemp);
        }
    }

    iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

    kSendEOI(iIRQ);
    kIncreaseInterruptCount(iIRQ);
    kProcessLoadBalancing(iIRQ);
}