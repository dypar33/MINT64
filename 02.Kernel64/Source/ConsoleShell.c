#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

SHELLCOMMANDENTRY gs_vstCommandTable[] = 
{
    {"help", "show help", kHelp},
    {"cls", "clear screen", kCls},
    {"totalram", "show total ram size", kShowTotalRAMSize},
    {"strtod", "str to decimal/hex", kStringToDecimalHexTest},
    {"shutdown", "shutdown and reboot os", kShutdown},
    {"settimer", "set pit controller counter0\n                 usage: settimer [ms] [periodic]\n", kSetTimer},
    {"sleep", "sleeping\n                 usage: sleep [ms]\n", kSleepUsingPIT},
    {"rdtsc", "read time stamp counter", kReadTimeStampCounter},
    {"cpuspeed", "measure cpu speed", kMeasureCPUSpeed},
    {"date", "show date and time", kShowDateAndTime},
    {"createtask", "create task\n                 usage: createtask [type] [count]\n", kCreateTestTask},
    {"changepriority", "change task priority\n                 usage: changepriority [ID] [Priority]\n", kChangeTaskPriority},
    {"tasklist", "show task list\n", kShowTaskList},
    {"killtask", "end task\n                 usage: killtask [id] or 0xffffffff(all)\n", kKillTask},
    {"cpuload", "show processor load\n", kCPULoad},
    {"testmutex", "test mutex function", kTestMutex},
    {"testthread", "test thread and process function", kTestThread},
    {"showmatrix", "show matrix screen~~!!", kShowMatrix},
    {"testpie", "pie calculation test", kTestPIE},
    {"dynamicmeminfo", "show dynamic memory info", kShowDynamicMemoryInformation},
    {"testseqalloc", "test sequential allocation & free", kTestSequentialAllocation},
    {"testranalloc", "test random allocation & free", kTestRandomAllocation},
    {"hddinfo", "show hdd info", kShowHDDInformation},
    {"readsector", "read hdd sector\n                 usage: readsector 0(LBA) 10(count)\n", kReadSector},
    {"writesector", "write hdd sector\n                 usage: writesector 0(LBA) 10(count)\n", kWriteSector},
    {"mounthdd", "mount hdd", kMountHDD},
    {"formathdd", "format hdd", kFormatHDD},
    {"filesysteminfo", "show file system information", kShowFileSystemInformation},
    {"createfile", "create file\n                 usage: createfile [file name]\n", kCreateFileInRootDirectory},
    {"deletefile", "delete file\n                 usage: deletefile [file name]\n", kDeleteFileInRootDirectory},
    {"ls", "show directory\n", kShowRootDirectory},
    {"writefile", "write data to file\n                 usage: writefile [file name]\n", kWriteDataToFile},
    {"readfile", "read data from  file\n                 usage: readfile [file name]\n", kReadDataFromFile},
    {"testfileio", "test file i/o function\n", kTestFileIO},
};

// main loop
void kStartConsoleShell(void)
{
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;

    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while(1)
    {
        bKey = kGetCh();

        if(bKey == KEY_BACKSPACE)
        {
            if(iCommandBufferIndex > 0)
            {
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStringXY(iCursorX -1, iCursorY, " ");
                kSetCursor(iCursorX -1, iCursorY);
                iCommandBufferIndex -= 1;
            }
        }
        else if(bKey == KEY_ENTER)
        {
            kPrintf("\n");

            if(iCommandBufferIndex > 0)
            {
                vcCommandBuffer[iCommandBufferIndex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }

            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        }
        else if(bKey == KEY_LSHIFT || bKey == KEY_RSHIFT || bKey == KEY_CAPSLOCK || bKey == KEY_NUMLOCK || bKey == KEY_SCROLLLOCK)
        {
            ;
        }
        else
        {
            if(bKey == KEY_TAB)
                bKey = ' ';
            
            if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT)
            {
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

void kExecuteCommand(const char* pcCommandBuffer)
{
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    iCommandBufferLength = kStrLen(pcCommandBuffer);

    for (iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++ )
    {
        if(pcCommandBuffer[iSpaceIndex] == ' ')
            break;
    }

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for (i = 0; i < iCount; i++)
    {
        iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iCommandLength == iSpaceIndex && kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)
        {
            gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
            break;
        }
    }

    if(i >= iCount)
        kPrintf("'%s' is not found\n", pcCommandBuffer);
}

void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter)
{
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter)
{
    int i;
    int iLength;

    if(pstList->iLength <= pstList->iCurrentPosition)
        return 0;
    
    for (i = pstList->iCurrentPosition; i < pstList->iLength; i++)
    {
        if(pstList->pcBuffer[i] == ' ')
            break;
    }
    
    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    pstList->iCurrentPosition += iLength + 1;
    
    return iLength;
}

/// command handler

static void kHelp(const char* pcCommandBuffer)
{
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;

    kPrintf( "=========================================================\n" );
    kPrintf( "                  MINT64 Shell Help by dy !              \n" );
    kPrintf( "=========================================================\n" );

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    for (int i = 0; i < iCount; i++)
    {
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iLength > iMaxCommandLength)
            iMaxCommandLength = iLength;
    }

    for (int i = 0; i < iCount; i++)
    {
        kPrintf("%s", gs_vstCommandTable[i].pcCommand);
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);

        if((i != 0) && ((i % 20) == 0))
        {
            kPrintf("Press any key to continue... ('q' is exit) : ");
            if(kGetCh() == 'q')
            {
                kPrintf("\n");
                break;
            }
            kPrintf("\n");
        }
    }
}

static void kCls(const char* pcParameterBuffer)
{
    kClearScreen();
    kSetCursor(0, 1);
}

static void kShowTotalRAMSize(const char* pcParameterBuffer)
{
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

static void kStringToDecimalHexTest(const char* pcParameterBuffer)
{
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;

    kInitializeParameter(&stList, pcParameterBuffer);

    while(1)
    {
        iLength = kGetNextParameter(&stList, vcParameter);
        if(iLength == 0)
        {   
            break;
        }

        kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

        if(kMemCpy(vcParameter, "0x", 2) == 0)
        {
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("HEX Value = %q\n", lValue);
        }
        else
        {
            lValue = kAToI(vcParameter, 10);
            kPrintf("Decimal Value = %d\n", lValue);
        }

        iCount++;
    }
}

static void kShutdown(const char* pcParameterBuffer)
{
    kPrintf("System Shutdown Start...\n");

    kPrintf("Press Any Key To Reboot PC");
    kGetCh();
    kReboot();
}

static void kSetTimer(const char* pcParameterBuffer)
{
    char vcParameter[100];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    kInitializeParameter(&stList, pcParameterBuffer);

    if(kGetNextParameter(&stList, vcParameter) == 0)
    {
        kPrintf("usage: settimer 10(ms) 1(periodic)");
        return;
    }

    bPeriodic = kAToI(vcParameter, 10);
    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

static void kSleepUsingPIT(const char* pcParameterBuffer)
{
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;

    kInitializeParameter(&stList, pcParameterBuffer);

    if(kGetNextParameter(&stList, vcParameter) == 0)
    {
        kPrintf("usage: sleep 100(ms)");
        return;
    }

    lMillisecond = kAToI(pcParameterBuffer, 10);
    kPrintf("%d ms Sleep Start...\n", lMillisecond);

    kDisableInterrupt();
    for (int i = 0; i < lMillisecond / 30; i++)
    {
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
    kEnableInterrupt();

    kPrintf("%d ms Sleep Complete!\n", lMillisecond);

    kInitializePIT(MSTOCOUNT(1), TRUE);
}

static void kReadTimeStampCounter(const char* pcParameterBuffer)
{
    QWORD qwTSC;

    qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter : %q\n", qwTSC);
}

static void kMeasureCPUSpeed(const char* pcParameterBuffer)
{
    QWORD qwLastTSC, qwTotalTSC = 0;

    kPrintf("Measuring .");
    kDisableInterrupt();
    for (int i = 0; i < 200; i++)
    {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf(".");
    }
    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();
    
    kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

static void kShowDateAndTime(const char* pcParameterBuffer)
{
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("%d-%d-%d %s / %d:%d:%d\n", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek), bHour, bMinute, bSecond);
}

static TCB gs_vstTask[2] = {0,};
static QWORD gs_vstStack[1024] = {0,};

static void kTestTask(void)
{
    int i = 0;

    while(1)
    {
        kPrintf("[%d] This message is from kTestTask.Press any key to switch kConsoleShell!\n", i++);
        kGetCh();

        kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
    }
}

static void kTestTask1(void)
{
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin;
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;

    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    for (int j = 0; j < 20000; j++)
    {
        switch (i)
        {
        case 0:
            iX++;
            if(iX >= (CONSOLE_WIDTH - iMargin))
                i = 1;
            break;
        case 1:
            iY++;
            if(iY >= (CONSOLE_HEIGHT - iMargin))
                i = 2;
            break;
        case 2:
            iX--;
            if(iX < iMargin)
                i = 3;
            break;
        case 3:
            iY--;
            if(iY < iMargin)
                i = 0;
            break;
        default:
            break;
        }

        pstScreen[iY * CONSOLE_WIDTH + iX].b_charactor = bData;
        pstScreen[iY * CONSOLE_WIDTH + iX].b_attribute = bData & 0x0F;
        bData++;

        //kSchedule();
    }
    
    kExitTask();
}

static void kTestTask2(void)
{
    int i = 0, iOffset;
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};

    pstRunningTask = kGetRunningTask();
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(TRUE)
    {
        pstScreen[iOffset].b_charactor = vcData[i%4];
        pstScreen[iOffset].b_attribute = (iOffset % 15) + 1;
        i++;

        //kSchedule();
    }

}

static void kCreateTestTask(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    int i;
    
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);
    switch( kAToI( vcType, 10 ) )
    {     
    case 1:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        { 
            if( kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask1 ) == NULL)
            {
                break;
            }
        }
    
        kPrintf( "Task1 %d Created~!\n", i );
        break;
    case 2:
    default:
        for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ )
        { 
            if( kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL)
            {
                break;
            }
        }
    
        kPrintf( "Task2 %d Created~!\n", i );
        break;
    } 
}

static void kChangeTaskPriority(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;

    char vcID[30];
    char vcPriority[30];
    QWORD qwID;
    BYTE bPriority;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);
    kGetNextParameter(&stList, vcPriority);

    if(kMemCmp(vcID, "0x", 2) == 0)
        qwID = kAToI(vcID+2, 16);   
    else
        qwID = kAToI(vcID, 10);

    bPriority = kAToI(vcPriority, 10);

    kPrintf("Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority);
    if(kChangePriority(qwID, bPriority) == TRUE)
        kPrintf("Success\n");
    else
        kPrintf("Fail\n");
}

static void kShowTaskList(const char* pcParameterBuffer)
{
    TCB* pstTCB;
    int iCount = 0;

    for (int i = 0; i < TASK_MAXCOUNT; i++)
    {
        pstTCB = kGetTCBInTCBPool(i);
        if((pstTCB->stLink.qwID >> 32) != 0)
        {
            if((iCount != 0) && ((iCount % 10) == 0))
            {
                kPrintf("Press any key to continue ... ('q' is exit): ");

                if(kGetCh() == 'q')
                {
                    kPrintf("\n");
                    break;
                }

                kPrintf("\n");
            }

            kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n \
   Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n", 
                1 + iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags), pstTCB->qwFlags, kGetListCount(&(pstTCB->stChildThreadList)),
                pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize);
            
        }
    }
}

static void kKillTask(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;
    char vcID[30];
    QWORD qwID;
    TCB* pstTCB;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);

    if(kMemCmp(vcID, "0x", 2) == 0)
        qwID = kAToI(vcID+2, 16);   
    else
        qwID = kAToI(vcID, 10);

    if(qwID != 0xFFFFFFFF)
    {
        pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
        qwID = pstTCB->stLink.qwID;

        if((qwID >> 32) != 0 && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00))
        {
            kPrintf("Kill task ID [0x%q] ", qwID);
            
            if(kEndTask(qwID) == TRUE)
                kPrintf("Success\n");
            else
                kPrintf("Fail\n");
        }
        else
            kPrintf("Task does not exist or task is system task\n");
    }
    else
    {
        for (int i = 0; i < TASK_MAXCOUNT; i++)
        {
            pstTCB = kGetTCBInTCBPool(i);
            qwID = pstTCB->stLink.qwID;

            if((qwID >> 32) != 0 && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00))
            {
                kPrintf("Kill task ID [0x%q] ", qwID);
        
                if(kEndTask(qwID) == TRUE)
                    kPrintf("Success\n");
                else
                    kPrintf("Fail\n");
            }
        }
        
    }

}

static void kCPULoad(const char* pcParameterBuffer)
{
    kPrintf("Processor Load: %d%%\n", kGetProcessorLoad());
}

static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask(void)
{
    QWORD qwTickCount;

    qwTickCount = kGetTickCount();
    while((kGetTickCount() - qwTickCount) < 50)
        kSchedule();

    for (int i = 0; i < 5; i++)
    {
        kLock(&(gs_stMutex));
        kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);

        gs_qwAdder += 1;
        kUnlock(&(gs_stMutex));

        for(int j = 0; j < 30000; j++);
    }

    qwTickCount = kGetTickCount();
    while((kGetTickCount() - qwTickCount) < 1000)
        kSchedule();

    kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer)
{
    int i;

    gs_qwAdder = 1;

    kInitializeMutex(&gs_stMutex);

    for (i = 0; i < 3; i++)
    {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD) kPrintNumberTask);
    }
    
    kPrintf("Wait Util %d Task End ... \n", i);
    kGetCh();
}

static void kCreateThreadTask(void)
{
    for (int i = 0; i < 3; i++)
    {
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD) kTestTask2);
    }
    
    while(1)
    {
        kSleep(1);
    }
}

static void kTestThread(const char* pcParameterBuffer)
{
    TCB* pstPorcess;

    pstPorcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE,
    0x1000, (QWORD) kCreateThreadTask);

    if(pstPorcess != NULL)
        kPrintf("Process [0x%Q] Create Success.. :)\n", pstPorcess->stLink.qwID);
    else
        kPrintf("Process Create Fail.. :(\n");
}

static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom(void)
{
    gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
    return gs_qwRandomValue;
}

static void kDropCharactorThread(void)
{
    int iX, iY;

    char vcText[2] = {0,};

    iX = kRandom() % CONSOLE_WIDTH;

    while(1)
    {
        kSleep(kRandom() % 20);

        if((kRandom() % 20) < 15)
        {
            vcText[0] = ' ';
            for (int i = 0; i < CONSOLE_HEIGHT; i++)
            {
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
            
        }
        else
        {
            for (int i = 0; i < CONSOLE_HEIGHT; i++)
            {
                vcText[0] = i + kRandom();
                kPrintStringXY(iX, i, vcText);
                kSleep(50);
            }
            
        }
    }
}

static void kMatrixProcess(void)
{
    int i;

    for (i = 0; i < 300; i++)
    {
        if(kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD) kDropCharactorThread) == NULL)
            break;
        
        kSleep(kRandom() % 5 + 5);
    }

    kPrintf("%d Thread is created\n", i);
    
    kGetCh();
}

static void kShowMatrix(const char* pcParameterBuffer)
{
    TCB* pstProcess;

    pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*) 0xE00000, 0xE00000,
    (QWORD) kMatrixProcess);

    if(pstProcess != NULL)
    {
        kPrintf("Matrix Process [0x%Q] Create Success\n");

        while((pstProcess->stLink.qwID >> 32) != 0)
            kSleep(100);
    }
    else
        kPrintf("Matrix Process Create Fail\n");
}

static void kFPUTestTask(void)
{
    double dValue1;
    double dValue2;

    TCB* pstRunningTask;

    QWORD qwCount = 0;
    QWORD qwRandomValue;

    int iOffset;

    char vcData[4] = {'-', '\\', '|', '/'};
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;

    pstRunningTask = kGetRunningTask();

    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(TRUE)
    {
        dValue1 = 1;
        dValue2 = 1;

        for (int i = 0; i < 10; i++)
        {
            qwRandomValue = kRandom();
            dValue1 *= (double) qwRandomValue;
            dValue2 *= (double) qwRandomValue;

            kSleep(1);

            qwRandomValue = kRandom();

            dValue1 /= (double) qwRandomValue;
            dValue2 /= (double) qwRandomValue;
        }
        
        if(dValue1 != dValue2)
        {
            kPrintf("Value is not same..! [%f] != [%f]\n", dValue1, dValue2);

            break;
        }

        qwCount++;

        pstScreen[iOffset].b_charactor = vcData[qwCount % 4];
        pstScreen[iOffset].b_attribute = (iOffset % 15) + 1;
    }
} 

static void kTestPIE(const char* pcParameterBuffer)
{
    double dResult;

    kPrintf("PIE calcuation test\n");
    kPrintf("Result: 355 / 113 = ");

    dResult = (double) 355 / 113;

    kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult*10) % 10), ((QWORD) (dResult * 100) % 10));

    for (int i = 0; i < 100; i++)
    {
        kSleep(0.001);
        kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD) kFPUTestTask);
    }
}

static void kShowDynamicMemoryInformation(const char* pcParameterBuffer)
{
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

    kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);

    kPrintf("============ Dynamic Memory Information ============\n");
    kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
    kPrintf("Total Size: [0x%Q]byte, [%d]MB\n", qwTotalSize, qwTotalSize / 1024 / 1024);
    kPrintf("Meta Size: [0x%Q]byte, [%d]KB\n", qwMetaSize, qwMetaSize / 1024);
    kPrintf("Used Size: [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024);
}

static void kTestSequentialAllocation(const char* pcParameterBuffer)
{
    DYNAMICMEMORY* pstMemory;
    QWORD* pqwBuffer;

    kPrintf("============ Dynamic Memory Test ============\n");
    pstMemory = kGetDynamicMemoryManager();

    for (long i = 0; i < pstMemory->iMaxLevelCount; i++)
    {
        kPrintf("Block List [%d] Test Start\n", i);
        kPrintf("Allocation And Compare: ");

        for (long j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++)
        {
            pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
            if(pqwBuffer == NULL)
            {
                kPrintf("\nAllocation Fail\n");
                return;
            }

            for (long k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++)
                pqwBuffer[k] = k;
            
            for (long k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++)
            {
                if(pqwBuffer[k] != k)
                {
                    kPrintf("\nCompare Fail\n");
                    return;
                }
            }

            kPrintf(".");
        }

        kPrintf("\nFree: ");

        for (long j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++)
        {
            if(kFreeMemory((void*) (pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE << i) * j)) == FALSE)
            {
                kPrintf("\nFree Fail\n");
                return;
            }

            kPrintf(".");
        }

        kPrintf("\n");
    }

    kPrintf("Test Complete!\n");
}

static void kRandomAllocationTask(void)
{
    TCB* pstTask;
    QWORD qwMemorySize;
    char vcBuffer[200];
    BYTE* pbAllocationBuffer;

    pstTask = kGetRunningTask();
    int iY = (pstTask->stLink.qwID) % 15 + 9;

    for (int j = 0; j < 10; j++)
    {
        do
        {
            qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
            pbAllocationBuffer = kAllocateMemory(qwMemorySize);

            if(pbAllocationBuffer == 0)
                kSleep(1);
        } while (pbAllocationBuffer == 0);
        
        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success...     ", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);
        kSleep(200);

        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);

        for (int i = 0; i < qwMemorySize / 2; i++)
        {
            pbAllocationBuffer[i] = kRandom() & 0xFF;
            pbAllocationBuffer[i + (qwMemorySize / 2)] = pbAllocationBuffer[i];
        }
        kSleep(200);
        
        kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...     ", pbAllocationBuffer, qwMemorySize);
        kPrintStringXY(20, iY, vcBuffer);

        for (int i = 0; i < qwMemorySize / 2; i++)
        {
            if(pbAllocationBuffer[i] != pbAllocationBuffer[i+(qwMemorySize/2)])
            {
                kPrintf("Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID);
                kExitTask();
            }
        }

        kFreeMemory(pbAllocationBuffer);
        kSleep(200);
    }

    kExitTask();
}

static void kTestRandomAllocation(const char* pcParameterBuffer)
{
    for (int i = 0; i < 1000; i++)
    {
        kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD) kRandomAllocationTask);
    }
    
}

static void kShowHDDInformation(const char* pcParameterBuffer)
{
    HDDINFORMATION stHDD;

    char vcBuffer[100];

    if(kReadHDDInformation(TRUE, TRUE, &stHDD) == FALSE)
    {
        kPrintf("HDD information read fail\n");
        return;
    }

    kPrintf("============ Primary Master HDD Information ============\n");

    kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
    vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
    kPrintf("Model Number:\t %s\n", vcBuffer);
 
    kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
    vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
    kPrintf("Serial Number:\t %s\n", vcBuffer);

    kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
    kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
    kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

    kPrintf("Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors / 2 / 1024);
}

static void kReadSector(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;

    char vcLBA[50], vcSectorCount[50];
    char* pcBuffer;

    DWORD dwLBA;

    int iSectorCount;

    BYTE bData;
    BOOL bExit = FALSE;

    kInitializeParameter(&stList, pcParameterBuffer);

    if((kGetNextParameter(&stList, vcLBA) == 0) || (kGetNextParameter(&stList, vcSectorCount) == 0))
    {
        kPrintf("usage) readsector 0(LBA) 10(count)\n");

        return;
    }

    dwLBA = kAToI(vcLBA, 10);
    iSectorCount = kAToI(vcSectorCount, 10);

    pcBuffer = kAllocateMemory(iSectorCount * 512);

    if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount)
    {
        kPrintf("LBA [%d], [%d] Sector Read Success!", dwLBA, iSectorCount);

        for(int j = 0; j < iSectorCount; j++)
        {
            for(int i = 0; i < 512; i++)
            {       
                if(!((j == 0) && (i == 0)) && ((i % 256) == 0))
                {
                    kPrintf("\nPress and key to continue ... ('q' is exit) : ");

                    if(kGetCh() == 'q')
                    {
                        bExit = TRUE;
                        break;
                    }
                }

                if((i%16) == 0)
                    kPrintf("\n[LBA%d, Offset:%d]\t| ", dwLBA + j, i);

                bData = pcBuffer[j * 512 + i] & 0xFF;

                if(bData < 16)
                    kPrintf("0");

                kPrintf("%X ", bData);
            }

            if(bExit == TRUE)
                break;
        }

        kPrintf("\n");
    }
    else
        kPrintf("Read Fail..\n");

    kFreeMemory(pcBuffer);
}

static void kWriteSector(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;

    char vcLBA[50], vcSectorCount[50];
    char* pcBuffer;

    static DWORD s_dwWriteCount = 0;
    DWORD dwLBA;

    int iSectorCount;
    
    BOOL bExit = FALSE;
    
    BYTE bData;

    kInitializeParameter(&stList, pcParameterBuffer);

    if((kGetNextParameter(&stList, vcLBA) == 0) || (kGetNextParameter(&stList, vcSectorCount) == 0))
    {
        kPrintf("usage) writesector 0(LBA) 10(count)\n");

        return;
    }

    dwLBA = kAToI(vcLBA, 10);
    iSectorCount = kAToI(vcSectorCount, 10);

    s_dwWriteCount++;
    pcBuffer = kAllocateMemory(iSectorCount * 512);

    for(int j = 0; j < iSectorCount; j++)
    {
        for(int i = 0; i < 512; i += 8)
        {
            *(DWORD*)&(pcBuffer[j * 512 + i]) = dwLBA + j;
            *(DWORD*)&(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
        }
    }

    if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount)
    {
        kPrintf("Write Fail..\n");

        return;
    }

    kPrintf("LBA [%d], [%d] Sector Read Success!", dwLBA, iSectorCount);

    for(int j = 0; j < iSectorCount; j++)
    {
        for(int i = 0; i < 512; i++)
        {
            if(!((j == 0) && (i == 0)) && ((i % 256) == 0))
            {
                kPrintf("\nPress any key to continue... ('q' is exit) : ");
                if(kGetCh() == 'q')
                {
                    bExit = TRUE;
                    break;
                }
            }

            if((i%16) == 0)
                kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);

            bData = pcBuffer[j * 512 + i] & 0xFF;
            if(bData < 16)
                kPrintf("0");

            kPrintf("%X ", bData);
        }

        if(bExit == TRUE)
            break;
    }

    kPrintf("\n");
    kFreeMemory(pcBuffer);
}

static void kMountHDD(const char* pcParameterBuffer)
{
    if(kMount() == FALSE)
    {
        kPrintf("HDD Mount Fail..\n");

        return;
    }

    kPrintf("HDD Mount Success~!\n");
}

static void kFormatHDD(const char* pcParameterBuffer)
{
    if(kFormat() == FALSE)
    {
        kPrintf("HDD Formatting Fail..\n");

        return;
    }

    kPrintf("HDD Formatting Success~!\n");
}

static void kShowFileSystemInformation(const char* pcParameterBuffer)
{
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation(&stManager);

    kPrintf( "================== File System Information ==================\n");
    kPrintf( "Mouted:\t\t\t\t\t %d\n", stManager.bMounted);
    kPrintf( "Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
    kPrintf( "Cluster Link Table Start Address:\t %d Sector\n", stManager.dwClusterLinkAreaStartAddress);
    kPrintf( "Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
    kPrintf( "Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
    kPrintf( "Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}

static void kCreateFileInRootDirectory(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    DWORD dwCluster;
    DIRECTORYENTRY stEntry;
    int i;
    FILE* pstFile;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);

    vcFileName[iLength] = '\0';

    if((iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) || (iLength == 0))
    {
        kPrintf("Too Long or Too Short File Name\n");

        return;
    }

    pstFile = fopen(vcFileName, "w");
    if(pstFile == NULL)
    {
        kPrintf("File Create Fail..\n");

        return;
    }

    fclose(pstFile);

    kPrintf("File Create Success!\n");
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    DIRECTORYENTRY stEntry;
    int iOffset;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);

    vcFileName[iLength] = '\0';
    
    if(iLength > FILESYSTEM_MAXFILENAMELENGTH - 1 || (iLength == 0))
    {
        kPrintf("Too Long or Too Short File Name\n");

        return;
    }

    if(remove(vcFileName) != 0)
    {
        kPrintf("File Not Found or File Opend\n");
        
        return;
    }

    kPrintf("File Delete Success!\n");
}

static void kShowRootDirectory(const char* pcParameterBuffer)
{
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[400];
    char vcTempValue[50];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation(&stManager);

    pstDirectory = opendir("/");
    if(pstDirectory == NULL)
    {
        kPrintf("Root Directory Read Fail\n");

        return;
    }

    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;

    while(1)
    {
        pstEntry = readdir(pstDirectory);

        if(pstEntry == NULL)
            break;
        
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        if(pstEntry->dwFileSize == 0)
            dwUsedClusterCount++;
        else
            dwUsedClusterCount += (pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE;
    }

    rewinddir(pstDirectory);
    iCount = 0;

    while(TRUE)
    {
        pstEntry = readdir(pstDirectory);

        if(pstEntry == NULL)
            break;
        
        kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
        vcBuffer[sizeof(vcBuffer) - 1] = '\0';
        
        kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));
        
        kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize);
        kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

        kSPrintf(vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex);
        kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
        kPrintf("    %s\n", vcBuffer);

        if((iCount != 0) && ((iCount % 20) == 0))
        {
            kPrintf("Press any key to continue... ('q' is exit) : ");

            if(kGetCh == 'q')
            {
                kPrintf("\n");
                break;
            }
        }

        iCount++;
    }

    kPrintf("\t Total File Count: %d\nTotal File Size: %d KByte (%d Cluster)\n", iTotalCount, dwTotalByte / 1024, dwUsedClusterCount);
    kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n", dwTotalByte / 1024, dwUsedClusterCount);

    closedir(pstDirectory);
}

static void kWriteDataToFile(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    int iEnterCount;
    FILE* fp;
    BYTE bKey;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);

    vcFileName[iLength] = '\0';

    if((iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) || (iLength == 0))
    {
        kPrintf("Too Long or Too Short File Name\n");

        return;
    }

    fp = fopen(vcFileName, "w");

    if(fp == NULL)
    {
        kPrintf("%s File Open Fail..\n", vcFileName);
    }

    iEnterCount = 0;

    while(TRUE)
    {
        bKey = kGetCh();

        if(bKey == KEY_ENTER)
        {
            iEnterCount++;

            if(iEnterCount >= 3)
                break;
        }
        else
        {
            iEnterCount = 0;
        }

        kPrintf("%c", bKey);
        if(fwrite(&bKey, 1, 1, fp) != 1)
        {
            kPrintf("File Write Fail\n");
            break;
        }
    }

    kPrintf("File Create Success\n");
    fclose(fp);
}

static void kReadDataFromFile(const char* pcParameterBuffer)
{
    PARAMETERLIST stList;
    char vcFileName[50];
    int iLength;
    int iEnterCount;
    FILE* fp;
    BYTE bKey;

    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcFileName);

    vcFileName[iLength] = '\0';

    if((iLength > FILESYSTEM_MAXFILENAMELENGTH - 1) || (iLength == 0))
    {
        kPrintf("Too Long or Too Short File Name\n");

        return;
    }

    fp = fopen(vcFileName, "r");

    if(fp == NULL)
    {
        kPrintf("%s File Open Fail..\n", vcFileName);
    }

    iEnterCount = 0;

    while(TRUE)
    {
        if(fread(&bKey, 1, 1, fp) != 1)
            break;
        
        kPrintf("%c", bKey);

        if(bKey == KEY_ENTER)
        {
            iEnterCount++;

            if((iEnterCount != 0) && ((iEnterCount % 20) == 0))
            {
                kPrintf("Press any key to continue... ('q' is exit) : ");

                if(kGetCh() == 'q')
                {
                    kPrintf("\n");
                    break;
                }

                kPrintf("\n");
                iEnterCount = 0;
            }
        }
    }

    fclose(fp);
}

static void kTestFileIO(const char* pcParameterBuffer)
{
    FILE* pstFile;
    BYTE* pbBuffer;

    int i, j;

    DWORD dwRandomOffset;
    DWORD dwByteCount;

    BYTE vbTempBuffer[1024];

    DWORD dwMaxFileSize;

    kPrintf("================== File I/O Function Test ==================\n");

    dwMaxFileSize = 4 * 1024 * 1024;

    pbBuffer = kAllocateMemory(dwMaxFileSize);
    if(pbBuffer == NULL)
    {
        kPrintf("Memory Allocation Fail\n");

        return;
    }

    remove("testfileio.bin");

    kPrintf("1. file open fail test...");
    pstFile = fopen("testfileio.bin", "r");

    if(pstFile == NULL)
        kPrintf("[Pass]\n");
    else
    {
        kPrintf("[Fail]\n");
        fclose(pstFile);
    }

    kPrintf("2. file create test...");
    pstFile = fopen("testfileio.bin", "w");

    if(pstFile != NULL)
    {
        kPrintf("[Pass]\n");
        kPrintf("   File Handle [0x%Q]\n", pstFile);
    }
    else
    {
        kPrintf("[Fail]\n");
    }

    kPrintf("3. sequential write test(cluster size)...");

    for (i = 0; i < 100; i++)
    {
        kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);
        
        if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE)
        {
            kPrintf("[Fail]\n");
            kPrintf("   %d Cluster Error\n", i);
            break;
        }
    }

    if(i >= 100)
        kPrintf("[Pass]\n");

    kPrintf("4. sequential read and verify test(cluster size)...");
    fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

    for (i = 0; i < 100; i++)
    {
        if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE)
        {
            kPrintf("[Fail]\n");

            return;
        }

        for(j = 0; j <FILESYSTEM_CLUSTERSIZE; j++)
        {
            if(pbBuffer[j] != (BYTE)i)
            {
                kPrintf("[Fail]\n");
                kPrintf("   %d Cluster Error. [%X] != [%X]\n", i, pbBuffer[j], (BYTE) i);

                break;
            }
        }
    }

    if(i >= 100)
        kPrintf("[Pass]\n");

    kPrintf("5. random write test...\n");

    kMemSet(pbBuffer, 0, dwMaxFileSize);

    fseek(pstFile, -100 *  FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
    fread(pbBuffer, 1, dwMaxFileSize, pstFile);

    for(i = 0; i < 100; i++)
    {
        dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
        dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);

        kPrintf("   [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);

        fseek(pstFile, dwRandomOffset, SEEK_SET);
        kMemSet(vbTempBuffer, i, dwByteCount);

        if(fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount)
        {
            kPrintf("[Fail]\n");
            break;
        }
        else
        {
            kPrintf("[Pass]\n");
        }

        kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
    }
    
    fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
    fwrite(&i, 1, 1, pstFile);
    pbBuffer[dwMaxFileSize - 1] = (BYTE) i;

    kPrintf("6. random and verify test...\n");

    for(i = 0; i < 100; i++)
    {
        dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
        dwRandomOffset = kRandom() % ((dwMaxFileSize) - dwByteCount);

        kPrintf("   [%d] Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);

        fseek(pstFile, dwRandomOffset, SEEK_SET);

        if(fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount)
        {
            kPrintf("[Fail]\n");
            kPrintf("   Read Fail\n", dwRandomOffset);
            break;
        }

        if(kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0)
        {
            kPrintf("[Fail]\n");
            kPrintf("   CompareFail\n", dwRandomOffset);

            break;
        }

        kPrintf("[Pass]\n");
    }

    kPrintf("7. sequential write, read and verify test(1024byte)...\n");
    fseek(pstFile, -dwMaxFileSize, SEEK_CUR);

    for(i = 0; i < (2 * 1024 * 1024 / 1024); i++)
    {
        kPrintf("   [%d] Offset [%d] Byte [%d] Write...", i, i * 1024, 1024);

        if(fwrite(pbBuffer + (i * 1024), 1, 1024, pstFile) != 1024)
        {
            kPrintf("[Fail]\n");
            return;
        }
        else
            kPrintf("[Pass]\n");
    }

    fseek(pstFile, -dwMaxFileSize, SEEK_SET);

    for(i = 0; i < (dwMaxFileSize / 1024); i++)
    {
        kPrintf("   [%d] Offset [%d] Byte [%d] Read And Verify...", i, i * 1024, 1024);

        if(fread(vbTempBuffer, 1, 1024, pstFile) != 1024)
        {
            kPrintf("[Fail]\n");
            return;
        }

        if(kMemCmp(pbBuffer + (i * 1024), vbTempBuffer, 1024) != 0)
        {
            kPrintf("[Fail]\n");
            break;
        }
        else
        {
            kPrintf("[Pass]\n");
        }
    }

    kPrintf("8. file delete fail test...");

    if(remove("testfileio.bin") != 0)
        kPrintf("[Pass]\n");
    else
        kPrintf("[Fail]\n");
    
    kPrintf("9. file close test...");

    if(fclose(pstFile) == 0)
        kPrintf("[Pass]\n");
    else
        kPrintf("[Fail]\n");

    kPrintf("10. file delete test...");
    if(remove("testfileio.bin") == 0)
        kPrintf("[Pass]\n");
    else
        kPrintf("[Fail]\n"); 
    
    kFreeMemory(pbBuffer);
}