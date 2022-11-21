#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"

void kPrintString(int ix, int iy, const char* pc_string);

void Main(void)
{
    int iCursorx, iCursorY;

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


    kPrintf("Keyboard Activate And Queue Initialize......[    ]");

    if(kInitializeKeyboard() == TRUE)
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else
    {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
        while(1);
    }

    kPrintf("PIC Controller And Interrupt Initialize.....[    ]");
    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kStartConsoleShell();
}

void kPrintString(int ix, int iy, const char* pc_string)
{
    CHARACTER* pst_screen = (CHARACTER*) 0xB8000;

    int i;

    pst_screen += (iy * 80) + ix;

    for (int i = 0; pc_string[i] != 0; i++)
    {
        pst_screen[i].b_charactor = pc_string[i];
    }
    
}

// 그래픽 cpu / 나머지 7부품 해서 6장의 견적서 만들기 (컴퓨존 같은 사이트 이용 or 전화로 요청)
// 재고가 없어도 견적을 할 수 있다면 넣는 것도 방법
// 12세대는 x
// 568, 56으로 전화