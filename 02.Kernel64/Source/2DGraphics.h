#ifndef __2DGRAPHICS_H__
#define __2DGRAPHICS_H__

#include "Types.h"

typedef WORD COLOR;

#define RGB(r, g, b) (((BYTE)(r) >> 3) << 11 | ((BYTE)(g) >> 2) << 5 | ((BYTE)(b) >> 3))

typedef struct kRectangleStruct
{
    int iX1;
    int iY1;
    int iX2;
    int iY2;
} RECT;

typedef struct kPointStruct
{
    int iX;
    int iY;
} POINT;

extern inline void kInternalDrawPixel(const RECT *pstMemoryArea, COLOR *pstMemoryAddress, int iX, int iY, COLOR stColor);

void kInternalDrawLine(const RECT *pstMemoryArea, COLOR *pstMemoryAddress, int iX1, int iY1, int iX2, int iY2, COLOR stColor);
void kInternalDrawRect(const RECT *pstMemoryArea, COLOR *pstMemoryAddress, int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill);
void kInternalDrawCircle(const RECT *pstMemoryArea, COLOR *pstMemoryAddress, int iX, int iY, int iRadius, COLOR stColor, BOOL bFill);
void kInternalDrawEnglishText(const RECT *pstMemoryArea, COLOR *pstMemoryAddress, int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char *pcString, int iLength);
void kInternalDrawHangulText(const RECT *pstMemoryArea, COLOR *pstMemoryAddress, int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char *pcString, int iLength);

extern inline BOOL kIsInRectangle(const RECT *pstArea, int iX, int iY);
extern inline int kGetRectangleWidth(const RECT *pstArea);
extern inline int kGetRectangleHeight(const RECT *pstArea);
extern inline void kSetRectangleData(int iX1, int iY1, int iX2, int iY2, RECT *pstRect);
extern inline BOOL kGetOverlappedRectangle(const RECT *pstArea1, const RECT *pstArea2, RECT *pstIntersection);
extern inline BOOL kIsRectangleOverlapped(const RECT *pstArea1, const RECT *pstArea2);

#endif