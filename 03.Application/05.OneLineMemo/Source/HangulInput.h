#ifndef __HANGULINPUT_H__
#define __HANGULINPUT_H__

#define TOLOWER(x) ((('A' <= (x)) && ((x) <= 'Z')) ? ((x) - 'A' + 'a') : (x))

typedef struct HangulInputTableItemStruct
{
    char *pcHangul;

    char *pcInput;
} HANGULINPUTITEM;

typedef struct HangulIndexTableItemStruct
{
    char cStartCharacter;

    DWORD dwIndex;
} HANGULINDEXITEM;

BOOL IsJaum(char cInput);
BOOL IsMoum(char cInput);
BOOL FindLongestHangulInTable(const char *pcInputBuffer, int iBufferCount, int *piMatchIndex, int *piMatchLength);
BOOL ComposeHangul(char *pcInputBuffer, int *piInputBufferLength, char *pcOutputBufferForProcessing, char *pcOutputBufferForComplete);

#endif