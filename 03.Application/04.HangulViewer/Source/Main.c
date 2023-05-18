#include "MINTOSLibrary.h"
#include "Main.h"

int Main(char *pcArgument)
{
    QWORD qwWindowID;
    DWORD dwFileOffset;
    int iX, iY, iWidth, iHeight, iMoveLine;
    TEXTINFO stInfo;
    EVENT stReceivedEvent;
    KEYEVENT *pstKeyEvent;
    WINDOWEVENT *pstWindowEvent;
    RECT stScreenArea;

    if (IsGraphicMode() == FALSE)
    {
        printf("This task can run only GUI mode\n");
        return;
    }

    if (strlen(pcArgument) == 0)
    {
        printf("usage) exec hanviwer.elf a.txt\n");
        return 0;
    }

    if (ReadFileToBuffer(pcArgument, &stInfo) == FALSE)
    {
        printf("%s file is not found\n", pcArgument);
        return 0;
    }

    GetScreenArea(&stScreenArea);

    iWidth = 500;
    iHeight = 500;
    iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
    iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;

    qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "한글 뷰어(Hangul Viewer)");
    CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
    stInfo.iCurrentLineIndex = 0;

    DrawTextBuffer(qwWindowID, &stInfo);

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
            if (pstKeyEvent->bFlags & KEY_FLAGS_DOWN)
            {
                switch (pstKeyEvent->bASCIICode)
                {
                case KEY_PAGEUP:
                    iMoveLine = -stInfo.iRowCount;
                    break;
                case KEY_PAGEDOWN:
                    iMoveLine = stInfo.iRowCount;
                    break;
                case KEY_UP:
                    iMoveLine = -1;
                    break;
                case KEY_DOWN:
                    iMoveLine = 1;
                    break;
                default:
                    iMoveLine = 0;
                    break;
                }

                if (stInfo.iCurrentLineIndex + iMoveLine < 0)
                    iMoveLine = -stInfo.iCurrentLineIndex;
                else if (stInfo.iCurrentLineIndex + iMoveLine >= stInfo.iMaxLineCount)
                    iMoveLine = stInfo.iMaxLineCount - stInfo.iCurrentLineIndex - 1;

                if (iMoveLine == 0)
                    break;

                stInfo.iCurrentLineIndex += iMoveLine;
                DrawTextBuffer(qwWindowID, &stInfo);
            }
            break;
        case EVENT_WINDOW_RESIZE:
            pstWindowEvent = &(stReceivedEvent.stWindowEvent);
            iWidth = GetRectangleWidth(&(pstWindowEvent->stArea));
            iHeight = GetRectangleHeight(&(pstWindowEvent->stArea));

            dwFileOffset = stInfo.pdwFileOffsetOfLine[stInfo.iCurrentLineIndex];

            CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
            stInfo.iCurrentLineIndex = dwFileOffset / stInfo.iColumnCount;

            DrawTextBuffer(qwWindowID, &stInfo);
            break;
        case EVENT_WINDOW_CLOSE:
            DeleteWindow(qwWindowID);
            free(stInfo.pbFileBuffer);
            free(stInfo.pdwFileOffsetOfLine);

            return 0;
        default:
            break;
        }
    }

    return 0;
}

BOOL ReadFileToBuffer(const char *pcFileName, TEXTINFO *pstInfo)
{
    DIR *pstDirectory;
    struct dirent *pstEntry;
    DWORD dwFileSize, dwReadSize;
    FILE *pstFile;

    pstDirectory = opendir("/");
    dwFileSize = 0;

    while (TRUE)
    {
        pstEntry = readdir(pstDirectory);
        if (pstEntry == NULL)
            break;

        if ((strlen(pstEntry->d_name) == strlen(pcFileName)) && (memcmp(pstEntry->d_name, pcFileName, strlen(pcFileName)) == 0))
        {
            dwFileSize = pstEntry->dwFileSize;
            break;
        }
    }

    closedir(pstDirectory);
    if (dwFileSize == 0)
    {
        printf("%s file doesn't exist or size is zero\n", pcFileName);
        return FALSE;
    }

    memcpy(&(pstInfo->vcFileName), pcFileName, sizeof(pstInfo->vcFileName));
    pstInfo->vcFileName[sizeof(pstInfo->vcFileName) - 1] = '\0';

    pstInfo->pdwFileOffsetOfLine = malloc(MAXLINECOUNT * sizeof(DWORD));
    if (pstInfo->pdwFileOffsetOfLine == NULL)
    {
        printf("Memory allocation fail\n");
        return FALSE;
    }

    pstInfo->pbFileBuffer = (BYTE *)malloc(dwFileSize);
    if (pstInfo->pbFileBuffer == NULL)
    {
        printf("Memory %dbytes allocate fail\n", dwFileSize);
        free(pstInfo->pdwFileOffsetOfLine);
        return FALSE;
    }

    pstFile = fopen(pcFileName, "r");
    if ((pstFile != NULL) && (fread(pstInfo->pbFileBuffer, 1, dwFileSize, pstFile) == dwFileSize))
    {
        fclose(pstFile);
        printf("%s file read success\n", pcFileName);
    }
    else
    {
        printf("%s file read fail\n", pcFileName);
        free(pstInfo->pdwFileOffsetOfLine);
        free(pstInfo->pbFileBuffer);
        fclose(pstFile);

        return FALSE;
    }

    pstInfo->dwFileSize = dwFileSize;

    return TRUE;
}

void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO *pstInfo)
{
    DWORD i;
    int iLineIndex, iColumnIndex;
    BOOL bHangul;

    pstInfo->iColumnCount = (iWidth - MARGIN * 2) / FONT_ENGLISHWIDTH;
    pstInfo->iRowCount = (iHeight - (WINDOW_TITLEBAR_HEIGHT * 2) - (MARGIN * 2)) / FONT_ENGLISHHEIGHT;

    iLineIndex = 0;
    iColumnIndex = 0;
    pstInfo->pdwFileOffsetOfLine[0] = 0;
    for (i = 0; i < pstInfo->dwFileSize; i++)
    {
        if (pstInfo->pbFileBuffer[i] == '\r')
            continue;
        else if (pstInfo->pbFileBuffer[i] == '\t')
        {
            iColumnIndex = iColumnIndex + TABSPACE;
            iColumnIndex -= iColumnIndex % TABSPACE;
        }
        else if(pstInfo->pbFileBuffer[i] & 0x80 == 1)
        {
            bHangul = TRUE;
            iColumnIndex += 2;
            i++;
        }
        else
        {
            bHangul = FALSE;
            iColumnIndex++;
        }

        if ((iColumnIndex >= pstInfo->iColumnCount) || (pstInfo->pbFileBuffer[i] == '\n'))
        {
            iLineIndex++;
            iColumnIndex = 0;

            if(bHangul == TRUE)
                i -= 2;

            if (i + 1 < pstInfo->dwFileSize)
                pstInfo->pdwFileOffsetOfLine[iLineIndex] = i + 1;

            if (iLineIndex >= MAXLINECOUNT)
                break;
        }
    }

    pstInfo->iMaxLineCount = iLineIndex;
}

BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO *pstInfo)
{
    DWORD i, j, dwBaseOffset;
    BYTE bTemp;
    int iXOffset, iYOffset, iLineCountToPrint, iColumnCountToPrint, iLength, iWidth, iColumnIndex;
    char vcBuffer[100];
    RECT stWindowArea;

    iXOffset = MARGIN;
    iYOffset = WINDOW_TITLEBAR_HEIGHT;
    GetWindowArea(qwWindowID, &stWindowArea);

    iWidth = GetRectangleWidth(&stWindowArea);
    DrawRect(qwWindowID, 2, iYOffset, iWidth - 3, WINDOW_TITLEBAR_HEIGHT * 2, RGB(146, 184, 177), TRUE);

    sprintf(vcBuffer, "파일: %s, 행 번호: %d/%d\n", pstInfo->vcFileName, pstInfo->iCurrentLineIndex + 1, pstInfo->iMaxLineCount);
    iLength = strlen(vcBuffer);

    DrawText(qwWindowID, (iWidth - iLength * FONT_ENGLISHWIDTH) / 2, WINDOW_TITLEBAR_HEIGHT + 2, RGB(0, 0, 0), RGB(146, 184, 177), vcBuffer, strlen(vcBuffer));

    iYOffset = (WINDOW_TITLEBAR_HEIGHT * 2) + MARGIN;
    DrawRect(qwWindowID, iXOffset, iYOffset, iXOffset + FONT_ENGLISHWIDTH * pstInfo->iColumnCount, iYOffset + FONT_ENGLISHHEIGHT * pstInfo->iRowCount, RGB(255, 255, 255), TRUE);

    iLineCountToPrint = MIN(pstInfo->iRowCount, (pstInfo->iMaxLineCount - pstInfo->iCurrentLineIndex));
    for (j = 0; j < iLineCountToPrint; j++)
    {
        dwBaseOffset = pstInfo->pdwFileOffsetOfLine[pstInfo->iCurrentLineIndex + j];

        iColumnCountToPrint = MIN(pstInfo->iColumnCount, (pstInfo->dwFileSize - dwBaseOffset));
        iColumnIndex = 0;
        for (i = 0; (i < iColumnCountToPrint) && (iColumnIndex < pstInfo->iColumnCount); i++)
        {
            bTemp = pstInfo->pbFileBuffer[i + dwBaseOffset];
            if (bTemp == '\n')
                break;
            else if (bTemp == '\t')
            {
                iColumnIndex = iColumnIndex + TABSPACE;
                iColumnIndex -= iColumnIndex % TABSPACE;
            }
            else if (bTemp == '\r')
                ;
            else
            {
                if(bTemp & 0x80 == 0)
                {
                    DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset, iYOffset + (j * FONT_ENGLISHHEIGHT), RGB(0, 0, 0), RGB(255, 255, 255), &bTemp, 1);
                    iColumnIndex++;
                }
                else if((iColumnIndex + 2) < pstInfo->iColumnCount)
                {
                    DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset, iYOffset + (j * FONT_ENGLISHHEIGHT), RGB(0, 0, 0), RGB(255, 255, 255), &pstInfo->pbFileBuffer[i + dwBaseOffset], 2);
                    iColumnIndex += 2;
                    i++;
                }
            }
        }
    }

    ShowWindow(qwWindowID, TRUE);

    return TRUE;
}