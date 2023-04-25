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
#include "MPConfiguationTable.h"
#include "Mouse.h"

void MainForApplicationProcessor(void);
BOOL kChangeToMultiCoreMode(void);
void kStartGraphicModeTest();
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
        kStartGraphicModeTest();
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

#define ABS(x) (((x) >= 0 ? (x) : -(x)))

void kGetRandomXY(int *piX, int *piY)
{
    int iRandom;

    iRandom = kRandom();
    *piX = ABS(iRandom) % 1000;

    iRandom = kRandom();
    *piY = ABS(iRandom) % 700;
}

COLOR kGetRandomColor(void)
{
    int iR, iG, iB;
    int iRandom;

    iRandom = kRandom();
    iR = ABS(iRandom) % 256;

    iRandom = kRandom();
    iG = ABS(iRandom) % 256;

    iRandom = kRandom();
    iB = ABS(iRandom) % 256;

    return RGB(iR, iG, iB);
}

void kDrawWindowFrame(int iX, int iY, int iWidth, int iHeight, const char *pcTitle)
{
    char *pcTestString1 = "This is MINT64 OS's window prototype!!";
    char *pcTestString2 = "Coming soon!!";
    VBEMODEINFOBLOCK *pstVBEMode;
    COLOR *pstVideoMemory;
    RECT stScreenArea;

    pstVBEMode = kGetVBEModeInfoBlock();

    stScreenArea.iX1 = 0;
    stScreenArea.iY1 = 0;
    stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
    stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

    pstVideoMemory = (COLOR *)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

    // 가장자리
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY, iX + iWidth, iY + iHeight, RGB(146, 184, 177), FALSE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + 1, iY + 1, iX + iWidth - 1, iY + iHeight - 1, RGB(146, 184, 177), FALSE);

    // 제목 표시줄
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY + 3, iX + iWidth - 1, iY + 21, RGB(92, 255, 209), TRUE);
    kInternalDrawText(&stScreenArea, pstVideoMemory, iX + 6, iY + 3, RGB(0, 0, 0), RGB(92, 255, 209), pcTitle, kStrLen(pcTitle));

    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 1, iY + 1, iX + iWidth - 1, iY + 1, RGB(140, 189, 184));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 1, iY + 2, iX + iWidth - 1, iY + 2, RGB(148, 176, 183));

    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 1, iY + 2, iX + 1, iY + 20, RGB(140, 189, 184));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 2, iY + 2, iX + 2, iY + 20, RGB(148, 176, 183));

    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 2, iY + 19, iX + iWidth - 2, iY + 19, RGB(46, 59, 30));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + 2, iY + 20, iX + iWidth - 2, iY + 20, RGB(46, 59, 30));

    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2, iY + 19, RGB(255, 255, 255), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2, iY + 1, iX + iWidth - 2, iY + 19 - 1, RGB(86, 86, 86), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 1, iY + 1, iX + iWidth - 2 - 1, iY + 19 - 1, RGB(86, 86, 86), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 1, iY + 19, iX + iWidth - 2, iY + 19, RGB(86, 86, 86), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 1, iY + 19 - 1, iX + iWidth - 2, iY + 19 - 1, RGB(86, 86, 86), TRUE);
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2 - 1, iY + 1, RGB(229, 229, 229));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1 + 1, iX + iWidth - 2 - 2, iY + 1 + 1, RGB(229, 229, 229));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2 - 18, iY + 19, RGB(229, 229, 229));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 1, iY + 1, iX + iWidth - 2 - 18 + 1, iY + 19 - 1, RGB(229, 229, 229));

    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 1 + 4, iX + iWidth - 2 - 4, iY + 19 - 4, RGB(148, 176, 183));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 5, iY + 1 + 4, iX + iWidth - 2 - 4, iY + 19 - 5, RGB(148, 176, 183));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 1 + 5, iX + iWidth - 2 - 5, iY + 19 - 4, RGB(148, 176, 183));

    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 4, RGB(148, 176, 183));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 5, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 5, RGB(148, 176, 183));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, iX + iWidth - 2 - 18 + 4, iY + 19 - 5, iX + iWidth - 2 - 5, iY + 1 + 4, RGB(148, 176, 183));

    kInternalDrawRect(&stScreenArea, pstVideoMemory, iX + 2, iY + 21, iX + iWidth - 2, iY + iHeight - 2, RGB(255, 255, 255), TRUE);

    kInternalDrawText(&stScreenArea, pstVideoMemory, iX + 10, iY + 30, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString1, kStrLen(pcTestString1));
    kInternalDrawText(&stScreenArea, pstVideoMemory, iX + 10, iY + 50, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString2, kStrLen(pcTestString2));
}

#define MOUSE_CURSOR_WIDTH 20
#define MOUSE_CURSOR_HEIGHT 20

BYTE gs_vmMouseBuffer[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
    0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
    0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
    0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
    0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
    0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};

#define MOUSE_CURSOR_OUTERLINE RGB (0, 0, 0)
#define MOUSE_CURSOR_OUTER RGB (79, 204, 11)
#define MOUSE_CURSOR_INNER RGB (232, 255, 232)

void kDrawCursor(RECT* pstArea, COLOR* pstVideoMemory, int iX, int iY)
{
    int i, j;
    BYTE* pbCurrentPos;

    pbCurrentPos = gs_vmMouseBuffer;

    for(j = 0; j < MOUSE_CURSOR_HEIGHT; j++)
    {
        for(i = 0; i < MOUSE_CURSOR_WIDTH; i++)
        {
            switch(*pbCurrentPos)
            {
                case 0:
                    break;
                case 1:
                    kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY, MOUSE_CURSOR_OUTERLINE);
                    break;
                case 2:
                    kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY, MOUSE_CURSOR_OUTER);
                    break;
                case 3:
                    kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY, MOUSE_CURSOR_INNER);
                    break;
            }

            pbCurrentPos++;
        }
    }
}

void kStartGraphicModeTest()
{
    VBEMODEINFOBLOCK *pstVBEMode;
    int iX, iY;
    COLOR *pstVideoMemory;
    RECT stScreenArea;

    int iRelativeX, iRelativeY;
    BYTE bButton;

    pstVBEMode = kGetVBEModeInfoBlock();

    stScreenArea.iX1 = 0;
    stScreenArea.iY1 = 0;
    stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
    stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

    pstVideoMemory = (COLOR *)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

    iX = pstVBEMode->wXResolution / 2;
    iY = pstVBEMode->wYResolution / 2;

    kInternalDrawRect(&stScreenArea, pstVideoMemory, stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2, RGB(232, 255, 232), TRUE);

    kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);

    while(TRUE)
    {
        if(kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) == FALSE)
        {
            kSleep(0);

            continue;
        }

        kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY, iX + MOUSE_CURSOR_WIDTH, iY + MOUSE_CURSOR_HEIGHT, RGB(232, 255, 232), TRUE);

        iX += iRelativeX;
        iY += iRelativeY;

        if(iX < stScreenArea.iX1)
            iX = stScreenArea.iX1;
        else if(iX > stScreenArea.iX2)
            iX = stScreenArea.iX2;
        
        if(iY < stScreenArea.iY1)
            iY = stScreenArea.iY1;
        else if(iY > stScreenArea.iY2)
            iY = stScreenArea.iY2;
        
        if(bButton & MOUSE_LBUTTONDOWN)
            kDrawWindowFrame(iX - 10, iY - 10, 400, 200, "MINT64 OS Test Window");
        else if(bButton & MOUSE_RBUTTONDOWN)
            kInternalDrawRect(&stScreenArea, pstVideoMemory, stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2, RGB(232, 255, 232), TRUE);
        
        kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);
    }
}