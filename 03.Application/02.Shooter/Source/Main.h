#ifndef __MAIN_H__
#define __MAIN_H__

#define MAXBUBBLECOUNT 50
#define RADIUS 16
#define DEFAULTSPEED 3
#define MAXLIFE 20

#define WINDOW_WIDTH 250
#define WINDOW_HEIGHT 350
#define INFORMATION_HEIGHT 20

typedef struct BubbleStruct
{
    QWORD qwX;
    QWORD qwY;
    QWORD qwSpeed;

    COLOR stColor;

    BOOL bAlive;
} BUBBLE;

typedef struct GameInfoStruct
{
    BUBBLE *pstBubbleBuffer;

    int iAliveBubbleCount;
    int iLife;

    QWORD qwScore;

    BOOL bGameStart;
} GAMEINFO;

BOOL Initialize(void);
BOOL CreateBubble(void);
void MoveBubble(void);
void DeleteBubbleUnderMouse(POINT *pstMouseXY);
void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID, POINT *pstMouseXY);

#endif