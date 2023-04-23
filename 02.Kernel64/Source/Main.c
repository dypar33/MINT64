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

void MainForApplicationProcessor(void);
void kStartGraphicModeTest();
void kPrintString(int ix, int iy, const char *pc_string);

void Main(void)
{
    int iCursorx, iCursorY;

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
        kPrintf("Pass\n");
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

    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());

    if (*(BYTE *)VBE_MODEINFOBLOCKADDRESS == 0)
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

    kPrintf("Application Processor[APIC ID: %d] Is Activated\n", kGetAPICID());

    kIdleTask();
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
    VBEMODEINFOBLOCK* pstVBEMode;
    COLOR* pstVideoMemory;
    RECT stScreenArea;

    pstVBEMode = kGetVBEModeInfoBlock();

    stScreenArea.iX1 = 0;
    stScreenArea.iY1 = 0;
    stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
    stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

    pstVideoMemory = (COLOR *) ((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

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

void kStartGraphicModeTest()
{
    VBEMODEINFOBLOCK *pstVBEMode;
    int iX1, iY1, iX2, iY2;
    COLOR stColor1, stColor2;

    int i;
    char *vpcString[] = {"Pixel", "Line", "Rectangle", "Circle", "MINT64 OS!"};
    COLOR* pstVideoMemory;
    RECT stScreenArea;
    
    pstVBEMode = kGetVBEModeInfoBlock();

    stScreenArea.iX1 = 0;
    stScreenArea.iY1 = 0;
    stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
    stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

    pstVideoMemory = (COLOR*) ((QWORD) pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);

    kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 0, RGB(255, 255, 255), RGB(0, 0, 0), vpcString[0], kStrLen(vpcString[0]));
    kInternalDrawPixel(&stScreenArea, pstVideoMemory, 1, 20, RGB(255, 255, 255));
    kInternalDrawPixel(&stScreenArea, pstVideoMemory, 2, 20, RGB(255, 255, 255));

    kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 25, RGB(255, 0, 0), RGB(0, 0, 0), vpcString[1], kStrLen(vpcString[1]));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 100, RGB(255, 0, 0));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 150, RGB(255, 0, 0));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 200, RGB(255, 0, 0));
    kInternalDrawLine(&stScreenArea, pstVideoMemory, 20, 50, 1000, 250, RGB(255, 0, 0));

    kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 180, RGB(0, 255, 0), RGB(0, 0, 0), vpcString[2], kStrLen(vpcString[2]));
    kInternalDrawRect(&stScreenArea, pstVideoMemory, 20, 200, 70, 250, RGB(0, 255, 0), FALSE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, 120, 200, 220, 300, RGB(0, 255, 0), TRUE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, 270, 200, 420, 350, RGB(0, 255, 0), FALSE);
    kInternalDrawRect(&stScreenArea, pstVideoMemory, 470, 200, 670, 400, RGB(0, 255, 0), TRUE);

    kInternalDrawText(&stScreenArea, pstVideoMemory, 0, 550, RGB(0, 0, 255), RGB(0, 0, 0), vpcString[3], kStrLen(vpcString[3]));
    kInternalDrawCircle(&stScreenArea, pstVideoMemory, 45, 600, 25, RGB(0, 0, 255), FALSE);
    kInternalDrawCircle(&stScreenArea, pstVideoMemory, 170, 600, 50, RGB(0, 0, 255), TRUE);
    kInternalDrawCircle(&stScreenArea, pstVideoMemory, 345, 600, 75, RGB(0, 0, 255), FALSE);
    kInternalDrawCircle(&stScreenArea, pstVideoMemory, 570, 600, 100, RGB(0, 0, 255), TRUE);

    kGetCh();

    do
    {
        for(i = 0; i < 100; i++)
        {
            kGetRandomXY(&iX1, &iY1);
            stColor1 = kGetRandomColor();

            kInternalDrawPixel(&stScreenArea, pstVideoMemory, iX1, iY1, stColor1);
        }

        for(i = 0; i < 100; i++)
        {
            kGetRandomXY(&iX1, &iY1);
            kGetRandomXY(&iX2, &iY2);
            stColor1 = kGetRandomColor();

            kInternalDrawLine(&stScreenArea, pstVideoMemory, iX1, iY1, iX2, iY2, stColor1);
        }

        for(i = 0; i < 20; i++)
        {
            kGetRandomXY(&iX1, &iY1);
            kGetRandomXY(&iX2, &iY2);
            stColor1 = kGetRandomColor();

            kInternalDrawRect(&stScreenArea, pstVideoMemory, iX1, iY1, iX2, iY2, stColor1, kRandom() % 2);
        }

        for(i = 0; i < 100; i++)
        {
            kGetRandomXY(&iX1, &iY1);
            stColor1 = kGetRandomColor();

            kInternalDrawCircle(&stScreenArea, pstVideoMemory, iX1, iY1, ABS(kRandom() % 50) + 1, stColor1, kRandom() % 2);
        }

        for(i = 0; i < 100; i++)
        {
            kGetRandomXY(&iX1, &iY1);
            stColor1 = kGetRandomColor();
            stColor2 = kGetRandomColor();

            kInternalDrawText(&stScreenArea, pstVideoMemory, iX1, iY1, stColor1, stColor2, vpcString[4], kStrLen(vpcString[4]));
        }
    } while (kGetCh() != 'q');
    
    while(1)
    {
        kInternalDrawRect(&stScreenArea, pstVideoMemory, stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2, RGB(232, 255, 232), TRUE);

        for(i = 0; i < 3; i++)
        {
            kGetRandomXY(&iX1, &iY1);
            kDrawWindowFrame(iX1, iY1, 400, 200, "MINT64");
        }

        kGetCh();
    }
}