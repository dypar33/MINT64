#ifndef __APPLICATIONPANELTASK_H__
#define __APPLICATIONPANELTASK_H__
#include "Types.h"
#include "Window.h"
#include "Font.h"

#define APPLICATIONPANEL_HEIGHT 31

#define APPLICATIONPANEL_TITLE "SYS_APPLICATIONPANNEL"

#define APPLICATIONPANEL_CLOCKWIDTH (8 * FONT_ENGLISHWIDTH)

#define APPLICATIONPANEL_LISTITEMHEIGHT (FONT_ENGLISHHEIGHT + 4)

#define APPLICATIONPANEL_LISTTITLE "SYS_APPLICATIONLIST"

#define APPLICATIONPANEL_COLOR_OUTERLINE RGB(46, 59, 30)
#define APPLICATIONPANEL_COLOR_MIDDLELINE RGB(140, 189, 184)
#define APPLICATIONPANEL_COLOR_INNERLINE RGB(148, 176, 183)
#define APPLICATIONPANEL_COLOR_BACKGROUND RGB(92, 255, 209)
#define APPLICATIONPANEL_COLOR_ACTIVE RGB(229, 229, 229)

typedef struct kApplicationPanelDataStruct
{
    QWORD qwApplicationPanelID;

    QWORD qwApplicationListID;

    RECT stButtonArea;

    int iApplicationListWidth;

    int iPreviousMouseOverIndex;

    BOOL bApplicationWindowVisible;
} APPLICATIONPANELDATA;

typedef struct kApplicationEntryStruct
{
    char *pcApplicationName;

    void *pvEntryPoint;
} APPLICATIONENTRY;

void kApplicationPanelGUITask(void);

static BOOL kCreateApplicationPanelWindow(void);
static void kDrawDigitalClock(QWORD qwWindowID);
static BOOL kCreateApplicationListWindow(void);
static void kDrawApplicationListItem(int iIndex, BOOL bMouseOver);
static BOOL kProcessApplicationPanelWindowEvent(void);
static BOOL kProcessApplicationListWindowEvent(void);
static int kGetMouseOverItemIndex(int iMouseY);

#endif