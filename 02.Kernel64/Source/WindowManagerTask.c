#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "GUITask.h"
#include "Font.h"
#include "ApplicationPanelTask.h"

void kStartWindowManager(void)
{
    int iMouseX, iMouseY;
    BOOL bMouseDataResult, bKeyDataResult, bEventQueueResult;

    kInitializeGUISystem();

    kGetCursorPosition(&iMouseX, &iMouseY);
    kMoveCursor(iMouseX, iMouseY);

    kCreateTask(TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kApplicationPanelGUITask, TASK_LOADBALANCINGID);

    while (TRUE)
    {
        bMouseDataResult = kProcessMouseData();
        bKeyDataResult = kProcessKeyData();
        bEventQueueResult = FALSE;

        while (kProcessEventQueueData() == TRUE)
        {
            bEventQueueResult = TRUE;
        }

        if ((bMouseDataResult == FALSE) && (bKeyDataResult == FALSE) && (bEventQueueResult == FALSE))
            kSleep(0);
    }
}

BOOL kProcessMouseData(void)
{
    QWORD qwWindowIDUnderMouse, qwWindowID;
    BYTE bButtonStatus, bChangedButton;

    int iRelativeX, iRelativeY, iMouseX, iMouseY, iPreviousMouseX, iPreviousMouseY, i;

    RECT stWindowArea;
    EVENT stEvent;
    WINDOWMANAGER *pstWindowManager;
    char vcTempTitle[WINDOW_TITLEMAXLENGTH];

    static int iWindowCount = 0;

    pstWindowManager = kGetWindowManager();

    for (i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
    {
        if (kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) == FALSE)
        {
            if (i == 0)
                return FALSE;
            else
                break;
        }

        kGetCursorPosition(&iMouseX, &iMouseY);
        if (i == 0)
        {
            iPreviousMouseX = iMouseX;
            iPreviousMouseY = iMouseY;
        }

        iMouseX += iRelativeX;
        iMouseY += iRelativeY;

        kMoveCursor(iMouseX, iMouseY);
        kGetCursorPosition(&iMouseX, &iMouseY);

        bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;

        if (bChangedButton != 0)
            break;
    }

    qwWindowIDUnderMouse = kFindWindowByPoint(iMouseX, iMouseY);

    bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;
    if (bChangedButton & MOUSE_LBUTTONDOWN)
    {
        if (bButtonStatus & MOUSE_LBUTTONDOWN)
        {
            if (qwWindowIDUnderMouse != pstWindowManager->qwBackgroundWindowID)
                kMoveWindowToTop(qwWindowIDUnderMouse);

            if (kIsInTitleBar(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
            {
                if (kIsInCloseButton(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
                {
                    kSetWindowEvent(qwWindowIDUnderMouse, EVENT_WINDOW_CLOSE, &stEvent);
                    kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);

                    kDeleteWindow(qwWindowIDUnderMouse);
                }
                else
                {
                    pstWindowManager->bWindowMoveMode = TRUE;
                    pstWindowManager->qwMovingWindowID = qwWindowIDUnderMouse;
                }
            }
            else
            {
                kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
                kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
            }
        }
        else
        {
            if (pstWindowManager->bWindowMoveMode == TRUE)
            {
                pstWindowManager->bWindowMoveMode = FALSE;
                pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
            }
            else
            {
                kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONUP, iMouseX, iMouseY, bButtonStatus, &stEvent);
                kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
            }
        }
    }
    else if (bChangedButton & MOUSE_RBUTTONDOWN)
    {
        if (bButtonStatus & MOUSE_RBUTTONDOWN)
        {
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
        else
        {
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONUP, iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
    }
    else if (bChangedButton & MOUSE_MBUTTONDOWN)
    {
        if (bButtonStatus & MOUSE_MBUTTONDOWN)
        {
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
        else
        {
            kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONUP, iMouseX, iMouseY, bButtonStatus, &stEvent);
            kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
        }
    }
    else
    {
        kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MOVE, iMouseX, iMouseY, bButtonStatus, &stEvent);
        kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
    }

    if (pstWindowManager->bWindowMoveMode == TRUE)
    {
        if (kGetWindowArea(pstWindowManager->qwMovingWindowID, &stWindowArea) == TRUE)
        {
            kMoveWindow(pstWindowManager->qwMovingWindowID, stWindowArea.iX1 + iMouseX - iPreviousMouseX, stWindowArea.iY1 + iMouseY - iPreviousMouseY);
        }
        else
        {
            pstWindowManager->bWindowMoveMode = FALSE;
            pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
        }
    }

    pstWindowManager->bPreviousButtonStatus = bButtonStatus;

    return TRUE;
}

BOOL kProcessKeyData(void)
{
    KEYDATA stKeyData;
    EVENT stEvent;
    QWORD qwActiveWindowID;

    if (kGetKeyFromKeyQueue(&stKeyData) == FALSE)
        return FALSE;

    qwActiveWindowID = kGetTopWindowID();
    kSetKeyEvent(qwActiveWindowID, &stKeyData, &stEvent);

    return kSendEventToWindow(qwActiveWindowID, &stEvent);
}

BOOL kProcessEventQueueData(void)
{
    EVENT vstEvent[WINDOWMANAGER_DATAACCUMULATECOUNT];
    int iEventCount, i, j;
    WINDOWEVENT *pstWindowEvent, *pstNextWindowEvent;
    QWORD qwWindowID;
    RECT stArea, stOverlappedArea;

    for (i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
    {
        if (kReceiveEventFromWindowManagerQueue(&(vstEvent[i])) == FALSE)
        {
            if (i == 0)
                return FALSE;
            else
                break;
        }

        pstWindowEvent = &(vstEvent[i].stWindowEvent);
        if (vstEvent[i].qwType == EVENT_WINDOWMANAGER_UPDATESCREENBYID)
        {
            if (kGetWindowArea(pstWindowEvent->qwWindowID, &stArea) == FALSE)
                kSetRectangleData(0, 0, 0, 0, &(pstWindowEvent->stArea));
            else
            {
                kSetRectangleData(0, 0, kGetRectangleWidth(&stArea) - 1, kGetRectangleHeight(&stArea) - 1, &(pstWindowEvent->stArea));
            }
        }
    }

    iEventCount = i;

    for (j = 0; j < iEventCount; j++)
    {
        pstWindowEvent = &(vstEvent[j].stWindowEvent);
        if ((vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) && (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) && (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA))
            continue;

        for (i = j + 1; i < iEventCount; i++)
        {
            pstNextWindowEvent = &(vstEvent[i].stWindowEvent);

            if (((vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) && (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
                 (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA)) ||
                (pstWindowEvent->qwWindowID != pstNextWindowEvent->qwWindowID))
            {
                continue;
            }

            if (kGetOverlappedRectangle(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea), &stOverlappedArea) == FALSE)
                continue;

            if (kMemCmp(&(pstWindowEvent->stArea), &stOverlappedArea, sizeof(RECT)) == 0)
            {
                kMemCpy(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea), sizeof(RECT));
                vstEvent[i].qwType = EVENT_UNKNOWN;
            }
            else if (kMemCmp(&(pstNextWindowEvent->stArea), &stOverlappedArea, sizeof(RECT)) == 0)
            {
                vstEvent[i].qwType = EVENT_UNKNOWN;
            }
        }
    }

    for (i = 0; i < iEventCount; i++)
    {
        pstWindowEvent = &(vstEvent[i].stWindowEvent);

        switch (vstEvent[i].qwType)
        {
        case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
        case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:
            if (kConvertRectClientToScreen(pstWindowEvent->qwWindowID, &(pstWindowEvent->stArea), &stArea) == TRUE)
                kRedrawWindowByArea(&stArea, pstWindowEvent->qwWindowID);
            break;
        case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
            kRedrawWindowByArea(&(pstWindowEvent->stArea), WINDOW_INVALIDID);
            break;
        default:
            break;
        }
    }

    return TRUE;
}