#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"

CONSOLEMANAGER gs_stConsoleManager = {0,};

static CHARACTER gs_vstScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];
static KEYDATA gs_vstKeyQueueBuffer[CONSOLE_GUIKEYQUEUE_MAXCOUNT];

void kInitializeConsole(int iX, int iY)
{
    kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));

    kMemSet(&gs_vstScreenBuffer, 0, sizeof(gs_vstScreenBuffer));

    if(kIsGraphicMode() == FALSE)
        gs_stConsoleManager.pstScreenBuffer = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    else
    {
        gs_stConsoleManager.pstScreenBuffer = gs_vstScreenBuffer;

        kInitializeQueue(&(gs_stConsoleManager.stKeyQueueForGUI), gs_vstKeyQueueBuffer, CONSOLE_GUIKEYQUEUE_MAXCOUNT, sizeof(KEYDATA));
        kInitializeMutex(&(gs_stConsoleManager.stLock));
    }

    kSetCursor(iX, iY);
}

// 커서 위치 설정
void kSetCursor(int iX, int iY)
{
    int iLinearValue;
    int iOldX, iOldY, i;

    iLinearValue = iY * CONSOLE_WIDTH + iX;

    if(kIsGraphicMode() == FALSE)
    {
        kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
        kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);
        kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
        kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);
    }
    else
    {
        for(i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++)
        {
            if((gs_stConsoleManager.pstScreenBuffer[i].b_charactor == '_') && (gs_stConsoleManager.pstScreenBuffer[i].b_attribute == 0x00))
            {
                gs_stConsoleManager.pstScreenBuffer[i].b_charactor = ' ';
                gs_stConsoleManager.pstScreenBuffer[i].b_attribute = CONSOLE_DEFAULTTEXTCOLOR;

                break;
            }
        }

        gs_stConsoleManager.pstScreenBuffer[iLinearValue].b_charactor = '_';
        gs_stConsoleManager.pstScreenBuffer[iLinearValue].b_attribute = 0x00;
    }

    gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

// 커서 위치 반환
void kGetCursor(int* piX, int* piY)
{
    *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
    *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

// printf
void kPrintf(const char* pcFormatString, ...)
{
    va_list ap;
    char vcBuffer[1024];
    int iNextPrintOffset;

    va_start(ap, pcFormatString);
    kVSPrintf(vcBuffer, pcFormatString, ap);
    
    va_end(ap);

    iNextPrintOffset = kConsolePrintString(vcBuffer);

    kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

int kConsolePrintString(const char* pcBuffer)
{
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    int iLength;
    int iPrintOffset;

    pstScreen = gs_stConsoleManager.pstScreenBuffer;

    iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

    iLength = kStrLen(pcBuffer);
    for (int i = 0; i < iLength; i++)
    {
        if(pcBuffer[i] == '\n')
            iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));
        else if(pcBuffer[i] == '\t')
            iPrintOffset += (8 - (iPrintOffset % 8));
        else
        {
            pstScreen[iPrintOffset].b_charactor = pcBuffer[i];
            pstScreen[iPrintOffset].b_attribute = CONSOLE_DEFAULTTEXTCOLOR;
            iPrintOffset++;
        }

        if(iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH))
        {
            kMemCpy(pstScreen, pstScreen + CONSOLE_WIDTH, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

            for (int j = (CONSOLE_HEIGHT -1) * CONSOLE_WIDTH; j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++)
            {
                pstScreen[j].b_charactor = ' ';
                pstScreen[j].b_attribute = CONSOLE_DEFAULTTEXTCOLOR;
            }

            iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;        
        }
    }

    return iPrintOffset;
}

void kClearScreen(void)
{
    CHARACTER* pstScreen;

    pstScreen = gs_stConsoleManager.pstScreenBuffer;
    
    for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++)
    {
        pstScreen[i].b_charactor = ' ';
        pstScreen[i].b_attribute = CONSOLE_DEFAULTTEXTCOLOR;
    }
    
    kSetCursor(0, 0);
}

BYTE kGetCh(void)
{
    KEYDATA stData;

    while(1)
    {
        if(kIsGraphicMode() == FALSE)
        {
            while(kGetKeyFromKeyQueue(&stData) == FALSE)
            {
                kSchedule();
            }
        }
        else
        {
            while(kGetKeyFromGUIKeyQueue(&stData) == FALSE)
            {
                if(gs_stConsoleManager.bExit == TRUE)
                    return 0xFF;
                
                kSchedule();
            }
        }

        if(stData.bFlags & KEY_FLAGS_DOWN)
            return stData.bASCIICode;
    }
}

void kPrintStringXY(int iX, int iY, const char* pcString)
{
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;

    pstScreen = gs_stConsoleManager.pstScreenBuffer;
    pstScreen += (iY * CONSOLE_WIDTH) + iX;

    for (int i = 0; pcString[i] != 0; i++)
    {
        pstScreen[i].b_charactor = pcString[i];
        pstScreen[i].b_attribute = CONSOLE_DEFAULTTEXTCOLOR;
    }
}

CONSOLEMANAGER *kGetConsoleManager(void)
{
    return &gs_stConsoleManager;
}

BOOL kGetKeyFromGUIKeyQueue(KEYDATA *pstData)
{
    BOOL bResult;

    if(kIsQueueEmpty(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE)
        return FALSE;

    kLock(&(gs_stConsoleManager.stLock));

    bResult = kGetQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

    kUnlock(&(gs_stConsoleManager.stLock));

    return bResult;
}

BOOL kPutKeyToGUIKeyQueue(KEYDATA *pstData)
{
    BOOL bResult;

    if(kIsQueueFull(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE)
        return FALSE;

    kLock(&(gs_stConsoleManager.stLock));

    bResult = kPutQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

    kUnlock(&(gs_stConsoleManager.stLock));

    return bResult;
}

void kSetConsoleShellExitFlag(BOOL bFlag)
{
    gs_stConsoleManager.bExit = bFlag;
}