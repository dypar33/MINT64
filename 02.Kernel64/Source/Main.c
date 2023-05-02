#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MultiProcessor.h"
#include "VBE.h"
#include "2DGraphics.h"
#include "MPConfigurationTable.h"
#include "Mouse.h"
#include "WindowManagerTask.h"

void MainForApplicationProcessor(void);
BOOL kChangeToMultiCoreMode(void);
void kPrintString(int ix, int iy, const char *pc_string);

void Main(void)
{
    int iCursorx, iCursorY;
    BYTE bButton;
    int iX, iY;

    if (*((BYTE *)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0)
        MainForApplicationProcessor();

    *((BYTE *)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

    kInitializeConsole(0, 10);
    kPrintf("Switch To IA-32e Mode Success!\n");
    kPrintf("IA-32e C Language Kernel Start..............[Pass]\n");
    kPrintf("Initialize Console..........................[Pass]\n");

    kGetCursor(&iCursorx, &iCursorY);
    kPrintf("GDT Initialize And Switch For IA-32e Mode...[    ]");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("TSS Segment Load............................[    ]");
    kLoadTR(GDT_TSSSEGMENT);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("IDT Initialize..............................[    ]");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("Total RAM Size Check........................[    ]");
    kCheckTotalRAMSize();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());

    kPrintf("TCB Pool And Scheduler Initialize...........[Pass]\n");
    iCursorY++;
    kInitializeScheduler();

    kPrintf("Dynamic Memory Initialize...................[Pass]\n");
    iCursorY++;
    kInitializeDynamicMemory();

    kInitializePIT(MSTOCOUNT(1), 1);

    kPrintf("Keyboard Activate And Queue Initialize......[    ]");

    if (kInitializeKeyboard() == TRUE)
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
        while (1);
    }

    kPrintf("Mouse Activate And Queue Initialize.........[    ]");
    if (kInitializeMouse() == TRUE)
    {
        kEnableMouseInterrupt();
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
    }
    else
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
        while (1)
            ;
    }

    kPrintf("PIC Controller And Interrupt Initialize.....[    ]");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("File System Initialize......................[    ]");
    if (kInitializeFileSystem() == TRUE)
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
    }
    else
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
    }

    kPrintf("Serial Port Initialize......................[Pass]\n");
    iCursorY++;

    kInitializeSerialPort();

    kPrintf("Change To MultiCore Processor Mode..........[    ]");
    if (kChangeToMultiCoreMode() == TRUE)
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
    }
    else
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
    }

    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());

    if (*(BYTE *)VBE_STARTGRAPHICMODEFLAGADDRESS == 0)
        kStartConsoleShell();
    else
        kStartWindowManager();
}

void kPrintString(int ix, int iy, const char *pc_string)
{
    CHARACTER *pst_screen = (CHARACTER *)0xB8000;

    int i;

    pst_screen += (iy * 80) + ix;

    for (int i = 0; pc_string[i] != 0; i++)
    {
        pst_screen[i].b_charactor = pc_string[i];
    }
}

void MainForApplicationProcessor()
{
    QWORD qwTickCount;

    kLoadGDTR(GDTR_STARTADDRESS);

    kLoadTR(GDT_TSSSEGMENT + (kGetAPICID() * sizeof(GDTENTRY16)));

    kLoadIDTR(IDTR_STARTADDRESS);

    kInitializeScheduler();

    kEnableSoftwareLocalAPIC();

    kSetTaskPriority(0);

    kInitializeLocalVectorTable();

    kEnableInterrupt();

    // kPrintf("Application Processor[APIC ID: %d] Is Activated\n", kGetAPICID());

    kIdleTask();
}

BOOL kChangeToMultiCoreMode(void)
{
    MPCONFIGURATIONMANAGER *pstMPManager;
    BOOL bInterruptFlag;
    int i;

    if (kStartUpApplicationProcessor() == FALSE)
        return FALSE;

    pstMPManager = kGetMPConfigurationManager();
    if (pstMPManager->bUsePICMode == TRUE)
    {
        kOutPortByte(0x22, 0x70);
        kOutPortByte(0x23, 0x01);
    }

    kMaskPICInterrupt(0xFFFF);

    kEnableGlobalLocalAPIC();

    kEnableSoftwareLocalAPIC();

    bInterruptFlag = kSetInterruptFlag(FALSE);

    kSetTaskPriority(0);
    kInitializeLocalVectorTable();
    kSetSymmetricIOMode(TRUE);

    kInitializeIORedirectionTable();

    kSetInterruptFlag(bInterruptFlag);
    kSetInterruptLoadBalancing(TRUE);

    for (i = 0; i < MAXPROCESSORCOUNT; i++)
    {
        kSetTaskLoadBalancing(i, TRUE);
    }

    return TRUE;
}
