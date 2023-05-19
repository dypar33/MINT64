#include "MINTOSLibrary.h"
#include "Main.h"
#include "HangulInput.h"

int Main(char *pcArgument)
{
    QWORD qwWindowID;
    int iX, iY, iWidth, iHeight;
    EVENT stReceivedEvent;
    KEYEVENT *pstKeyEvent;
    WINDOWEVENT *pstWindowEvent;
    RECT stScreenArea;
    BUFFERMANAGER stBufferManager;
    BOOL bHangulMode;

    if (IsGraphicMode() == FALSE)
    {
        printf("This task can run only GUI mode\n");
        return;
    }

    GetScreenArea(&stScreenArea);

    iWidth = MAXOUTPUTLENGTH * FONT_ENGLISHWIDTH + 5;
    iHeight = 40;
    iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
    iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;

    qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT, "한 줄 메모장(한/영 전환은 [ALT])");

    memset(&stBufferManager, 0, sizeof(stBufferManager));
    bHangulMode = FALSE;

    DrawRect(qwWindowID, 3, 4 + WINDOW_TITLEBAR_HEIGHT, 5, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT, RGB(0, 250, 0), TRUE);
    ShowWindow(qwWindowID, TRUE);
    while(1);

    while (TRUE)
    {
        if (ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
        {
            Sleep(10);
            continue;
        }

        switch (stReceivedEvent.qwType)
        {
        case EVENT_KEY_DOWN:
            pstKeyEvent = &(stReceivedEvent.stKeyEvent);
            switch (pstKeyEvent->bASCIICode)
            {
            case KEY_LALT:
                if (bHangulMode == TRUE)
                {
                    stBufferManager.iInputBufferLength = 0;
                    if ((stBufferManager.vcOutputBufferForProcessing[0] != '\0') && (stBufferManager.iOutputBufferLength + 2 < MAXOUTPUTLENGTH))
                    {
                        memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength, stBufferManager.vcOutputBufferForProcessing, 2);
                        stBufferManager.iOutputBufferLength += 2;

                        stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                    }
                }
                else
                {
                    stBufferManager.iInputBufferLength = 0;
                    stBufferManager.vcOutputBufferForComplete[0] = '\0';
                    stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                }

                bHangulMode = TRUE - bHangulMode;

                break;
            case KEY_BACKSPACE:
                if ((bHangulMode == TRUE) && (stBufferManager.iInputBufferLength > 0))
                {
                    stBufferManager.iInputBufferLength--;
                    ComposeHangul(stBufferManager.vcInputBuffer, &stBufferManager.iInputBufferLength, stBufferManager.vcOutputBufferForProcessing, stBufferManager.vcOutputBufferForComplete);
                }
                else
                {
                    if (stBufferManager.iOutputBufferLength > 0)
                    {
                        if ((stBufferManager.iOutputBufferLength >= 2) && (stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength - 1] & 0x80))
                        {
                            stBufferManager.iOutputBufferLength -= 2;
                            memset(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength, 0, 2);
                        }
                        else
                        {
                            stBufferManager.iOutputBufferLength--;
                            stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = '\0';
                        }
                    }
                }
                break;
            default:
                if (pstKeyEvent->bASCIICode & 0x80)
                    break;

                if ((bHangulMode == TRUE) && (stBufferManager.iOutputBufferLength + 2 <= MAXOUTPUTLENGTH))
                {
                    ConvertJaumMoumToLowerCharactor(&pstKeyEvent->bASCIICode);

                    stBufferManager.vcInputBuffer[stBufferManager.iInputBufferLength] = pstKeyEvent->bASCIICode;
                    stBufferManager.iInputBufferLength++;

                    if (ComposeHangul(stBufferManager.vcInputBuffer, &stBufferManager.iInputBufferLength, stBufferManager.vcOutputBufferForProcessing, stBufferManager.vcOutputBufferForComplete) == TRUE)
                    {
                        if (stBufferManager.vcOutputBufferForComplete[0] != '\0')
                        {
                            memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength, stBufferManager.vcOutputBufferForComplete, 2);
                            stBufferManager.iOutputBufferLength += 2;

                            if (stBufferManager.iOutputBufferLength + 2 > MAXOUTPUTLENGTH)
                            {
                                stBufferManager.iInputBufferLength = 0;
                                stBufferManager.vcOutputBufferForProcessing[0] = '\0';
                            }
                        }
                    }
                    else
                    {
                        if (stBufferManager.vcOutputBufferForComplete[0] != '\0')
                        {
                            memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength, stBufferManager.vcOutputBufferForComplete, 2);
                            stBufferManager.iOutputBufferLength += 2;
                        }

                        if (stBufferManager.iOutputBufferLength < MAXOUTPUTLENGTH)
                        {
                            stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = stBufferManager.vcInputBuffer[0];
                            stBufferManager.iOutputBufferLength++;
                        }

                        stBufferManager.iInputBufferLength = 0;
                    }
                }
                else if ((bHangulMode == FALSE) && (stBufferManager.iOutputBufferLength + 1 <= MAXOUTPUTLENGTH))
                {
                    stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = pstKeyEvent->bASCIICode;
                    stBufferManager.iOutputBufferLength++;
                }

                break;
            }

            DrawText(qwWindowID, 2, WINDOW_TITLEBAR_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255), stBufferManager.vcOutputBuffer, MAXOUTPUTLENGTH);

            if (stBufferManager.vcOutputBufferForProcessing[0] != '\0')
            {
                DrawText(qwWindowID, 2 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, WINDOW_TITLEBAR_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255), stBufferManager.vcOutputBufferForProcessing, 2);
            }

            DrawRect(qwWindowID, 3 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, 4 + WINDOW_TITLEBAR_HEIGHT, 5 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT, RGB(0, 250, 0), TRUE);

            ShowWindow(qwWindowID, TRUE);
            break;
        case EVENT_WINDOW_CLOSE:
            DeleteWindow(qwWindowID);

            return 0;
        default:
            break;
        }
    }

    return 0;
}

void ConvertJaumMoumToLowerCharactor(BYTE *pbInput)
{
    if ((*pbInput < 'A') || (*pbInput > 'Z'))
        return;

    switch (*pbInput)
    {
    case 'Q':
    case 'W':
    case 'E':
    case 'R':
    case 'T':
    case 'O':
    case 'P':
        return;
        break;
    }

    *pbInput = TOLOWER(*pbInput);
}