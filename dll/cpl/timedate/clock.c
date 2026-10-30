/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/clock.c
 * PURPOSE:     Draws the analog clock
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2007 Eric Kohl
 *              Copyright 1997-2028 Republic of CATV 95 <catv95republic@proton.me>
 */

/* Code based on clock.c from DATETIME by Republic of CATV 95 */

#include "timedate.h"
#include "clock.h"

#ifdef WIN32
  #define MoveTo(hDC,x,y)     MoveToEx(hDC,x,y,NULL)
  #define GetWindowPtr(w,o)   GetWindowLongPtrW(w,o)
  #define SetWindowPtr(w,o,p) SetWindowLongPtrW(w,o,(LONG_PTR)(p))
#else
  #define GetWindowPtr(w,o)   GetWindowWord(w,o)
  #define SetWindowPtr(w,o,p) SetWindowWord(w,o,p)
#endif

static const WCHAR szClockWndClass[] = L"ClockWndClass";

LRESULT
CALLBACK
ClockWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void SetClockTimes(ClockHandTime* pt, PTDDATAOBJ PTDate)
{
    if (!PTDate)
        return;

    pt->handSec  = GetDateTime(0);
    pt->handMin  = GetDateTime(1);
    pt->handHour = GetDateTime(2);
}

void CreateClockObject(PTDDATAOBJ PTDate)
{
    if (!PTDate)
        return;
    
    PTDate->hDarkHandColor     =  CreateSolidBrush(DARK_COLOR_CLOCK);
    PTDate->hLightHandColor    =  CreateSolidBrush(LIGHT_COLOR_CLOCK);
    PTDate->hSecondPenColor    =  CreatePen(0, 1, GetSysColor(SECOND_HAND_COLOR_CLOCK));
    PTDate->hDarkBorderColor   =  CreateSolidBrush(DARK_COLOR_CLOCK);
    PTDate->hLightBorderColor  =  CreateSolidBrush(LIGHT_COLOR_CLOCK);
}

void RemoveClockObject(PTDDATAOBJ PTDate)
{
    if (!PTDate)
        return;
    if (PTDate->hDarkHandColor)
        DeleteObject(PTDate->hDarkHandColor);
    if (PTDate->hLightHandColor)
        DeleteObject(PTDate->hLightHandColor);
    if (PTDate->hSecondPenColor)
        DeleteObject(PTDate->hSecondPenColor);
    if (PTDate->hDarkBorderColor)
        DeleteObject(PTDate->hDarkBorderColor);
    if (PTDate->hLightBorderColor)
        DeleteObject(PTDate->hLightBorderColor);
}

void ResizeClock(PTDDATAOBJ PTDate, int newWidth, int newHeight)
{
     if (!PTDate)
         return;

     /* Do nothing only when both dimensions are unchanged */
     if (newWidth == PTDate->ccWidth && newHeight == PTDate->ccHeight)
         return;
     
     /* Set size of clock */
     PTDate->ccWidth = newWidth;
     PTDate->ccHeight = newHeight;
     
     /* Set center of the clock */
     PTDate->ccLocX = newWidth / 2;
     PTDate->ccLocY = newHeight / 2;
}

void DrawClockFace(HDC hDC, PTDDATAOBJ PTDate)
{
    int cx, cy, radius;
    int i;
    int x1, y1, x2, y2;
    int markerLength;
    double angle;
    HPEN hPen, hOldPen;
    HPEN hHourPen, hMinutePen;
    HBRUSH hBrush, hOldBrush;

    if (!hDC || !PTDate)
        return;

    if (PTDate->ccWidth <= 0 || PTDate->ccHeight <= 0)
        return;

    cx = PTDate->ccLocX;
    cy = PTDate->ccLocY;

    radius = (PTDate->ccWidth < PTDate->ccHeight ?
              PTDate->ccWidth : PTDate->ccHeight) / 2 - 2;

    if (radius <= 0)
        return;

    /* Create the clock face background and border pens. */
    hBrush = CreateSolidBrush(BKG_DARK_COLOR_CLOCK);
    hPen = CreatePen(PS_SOLID, 2, BKG_BORDER_LIGHT_COLOR_CLOCK);
    hHourPen = CreatePen(PS_SOLID, 3, LIGHT_COLOR_CLOCK);
    hMinutePen = CreatePen(PS_SOLID, 2, DARK_COLOR_CLOCK);

    if (!hBrush || !hPen || !hHourPen || !hMinutePen)
    {
        if (hBrush)
            DeleteObject(hBrush);
        if (hPen)
            DeleteObject(hPen);
        if (hHourPen)
            DeleteObject(hHourPen);
        if (hMinutePen)
            DeleteObject(hMinutePen);

        return;
    }

    /*
     * Draw the outer circular border and its background.
     * The middle ring has a separate background color.
     * The inner circle is the main clock face.
     */
    hOldPen = (HPEN)SelectObject(hDC, hPen);
    hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);

    /* Outer circular border and background. */
    Ellipse(hDC,
            cx - radius,
            cy - radius,
            cx + radius + 1,
            cy + radius + 1);

    /* Background between the inner and outer borders. */
    {
        HBRUSH hRingBrush;
        HBRUSH hPreviousBrush;
        HPEN hPreviousPen;

        hRingBrush = PTDate->hLightBorderColor;

        if (hRingBrush)
        {
            hPreviousPen = (HPEN)SelectObject(hDC, hPen);
            hPreviousBrush = (HBRUSH)SelectObject(
                hDC, hRingBrush);

            Ellipse(hDC,
                    cx - radius + 4,
                    cy - radius + 4,
                    cx + radius - 3,
                    cy + radius - 3);

            SelectObject(hDC, hPreviousBrush);
            SelectObject(hDC, hPreviousPen);

            DeleteObject(hRingBrush);
        }
    }

    /* Inner circular border and main clock face. */
    {
        HBRUSH hFaceBrush;
        HBRUSH hPreviousBrush;
        HPEN hPreviousPen;

        hFaceBrush = PTDate->hDarkBorderColor;

        if (hFaceBrush)
        {
            hPreviousPen = (HPEN)SelectObject(hDC, hPen);
            hPreviousBrush = (HBRUSH)SelectObject(
                hDC, hFaceBrush);

            if (radius > 8)
            {
                Ellipse(hDC,
                        cx - radius + 8,
                        cy - radius + 8,
                        cx + radius - 7,
                        cy + radius - 7);
            }

            SelectObject(hDC, hPreviousBrush);
            SelectObject(hDC, hPreviousPen);

            DeleteObject(hFaceBrush);
        }
    }

    /* Twelve hour marks, with 12 at the top. */
    SelectObject(hDC, hHourPen);

    for (i = 0; i < 12; ++i)
    {
        angle = ((double)i * 30.0 - 90.0)
                * CLOCK_PI / 180.0;

        markerLength = radius / 5;

        x1 = cx + (int)((double)(radius - 7)
                        * cos(angle));
        y1 = cy + (int)((double)(radius - 7)
                        * sin(angle));

        x2 = cx + (int)((double)(radius - markerLength)
                        * cos(angle));
        y2 = cy + (int)((double)(radius - markerLength)
                        * sin(angle));

        MoveTo(hDC, x1, y1);
        LineTo(hDC, x2, y2);
    }

    /* Sixty minute marks, with longer marks at each hour. */
    SelectObject(hDC, hMinutePen);

    for (i = 0; i < 60; ++i)
    {
        /* The hour marks are already drawn above. */
        if (i % 5 == 0)
            continue;

        angle = ((double)i * 6.0 - 90.0)
                * CLOCK_PI / 180.0;

        markerLength = radius / 20;

        x1 = cx + (int)((double)(radius - 7)
                        * cos(angle));
        y1 = cy + (int)((double)(radius - markerLength)
                        * sin(angle));

        x2 = cx + (int)((double)(radius - markerLength)
                        * cos(angle));
        y2 = cy + (int)((double)(radius - markerLength)
                        * sin(angle));

        MoveTo(hDC, x1, y1);
        LineTo(hDC, x2, y2);
    }

    /* Draw the central pivot. */
    SelectObject(hDC, hPen);
    SelectObject(hDC, hBrush);

    if (radius >= 4)
    {
        Ellipse(hDC,
                cx - 3,
                cy - 3,
                cx + 4,
                cy + 4);
    }

    /* Restore selected objects before deleting temporary GDI objects. */
    SelectObject(hDC, hOldBrush);
    SelectObject(hDC, hOldPen);

    DeleteObject(hBrush);
    DeleteObject(hPen);
    DeleteObject(hHourPen);
    DeleteObject(hMinutePen);
}

void
DrawFatHand(HDC hDC, PTDDATAOBJ PTDate, int X, int Y,
            BOOL IsHourHand)
{
    POINT points[4];
    double angle;
    double length;
    double tailLength;
    double halfWidth;
    double shoulderPos;
    double dx, dy, px, py;
    int hour, minute;
    int curX,curY;
    HBRUSH hBrush, hOldBrush;
    HPEN hOldPen, hPen;
    int radius,cumMinute;

    if (!hDC || !PTDate)
        return;

    radius = (PTDate->ccWidth < PTDate->ccHeight ?
              PTDate->ccWidth : PTDate->ccHeight) / 2;

    if (radius <= 0)
        return;
        
    curX = X;
    curY = Y;

    hour = PTDate->curClockTime.handHour % 12;
    if (hour < 0)
        hour += 12;

    minute = PTDate->curClockTime.handMin;
    if (minute < 0)
        minute = 0;
    if (minute > 59)
        minute = 59;

    cumMinute = 0;
    if ( minute >= 12 ) cumMinute += 12;
    if ( minute >= 24 ) cumMinute += 12;
    if ( minute >= 36 ) cumMinute += 12;
    if ( minute >= 48 ) cumMinute += 12;

    if (IsHourHand)
    {
        /* Hour hand advances gradually as the minutes pass. */
        angle = ((double)hour + (double)cumMinute / 60.0) *
                30.0 * CLOCK_PI / 180.0 - CLOCK_PI / 2.0;

        length = (double)radius * 0.625;
        tailLength = (double)radius * 0.13;
        halfWidth = (double)radius * 0.055;
        shoulderPos = length * 0.15;

        hBrush = PTDate->hDarkHandColor;
        hPen = CreatePen(PS_SOLID, 1, GetSysColor(DARK_COLOR_CLOCK));
    }
    else
    {
        angle = (double)minute * 6.0 *
                CLOCK_PI / 180.0 - CLOCK_PI / 2.0;

        length = (double)radius * 0.75;
        tailLength = (double)radius * 0.18;
        halfWidth = (double)radius * 0.055;
        shoulderPos = length * 0.15;

        hBrush = PTDate->hLightHandColor;
        hPen = CreatePen(PS_SOLID, 1, GetSysColor(LIGHT_COLOR_CLOCK));
    }

    if (!hBrush)
        return;

    /* Unit vector along the hand and perpendicular to it. */
    dx = cos(angle);
    dy = sin(angle);
    px = -dy;
    py = dx;

    /*
     * Four-point kite-shaped polygon:
     * pointed front, broad shoulders, and pointed rear tail.
     */
    points[0].x = curX + (int)(dx * length);
    points[0].y = curY + (int)(dy * length);

    points[1].x = curX + (int)(dx * shoulderPos
                            + px * halfWidth);
    points[1].y = curY + (int)(dy * shoulderPos
                            + py * halfWidth);

    points[2].x = curX - (int)(dx * tailLength);
    points[2].y = curY - (int)(dy * tailLength);

    points[3].x = curX + (int)(dx * shoulderPos
                            - px * halfWidth);
    points[3].y = curY + (int)(dy * shoulderPos
                            - py * halfWidth);

    hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
    hOldPen   = (HPEN)SelectObject(hDC, hPen);
    
    SelectObject(hDC, GetStockObject(NULL_PEN));
    Polygon(hDC, points, 4);

    SelectObject(hDC, hOldBrush);
    SelectObject(hDC, hOldPen);
}

void
DrawSecondHand(HDC hDC, PTDDATAOBJ PTDate, int X, int Y)
{
    int rSecond;
    int radius;
    int x2, y2;
    double angle;
    HPEN hPen, hOldPen;

    if (!hDC || !PTDate)
    {
        return;
    }

    radius = (PTDate->ccWidth < PTDate->ccHeight ?
              PTDate->ccWidth : PTDate->ccHeight) / 2;

    if (radius <= 0)
        return;

    rSecond = PTDate->curClockTime.handSec % 60;
    if (rSecond < 0)
        rSecond += 60;

    angle = (double)rSecond * 6.0 *
            CLOCK_PI / 180.0 - CLOCK_PI / 2.0;

    /* Slightly longer than the minute hand. */
    x2 = X + (int)((double)radius * 0.77 * cos(angle));
    y2 = Y + (int)((double)radius * 0.77 * sin(angle));

    hPen = PTDate->hSecondPenColor;
    if (!hPen)
        return;

    hOldPen = (HPEN)SelectObject(hDC, hPen);

    MoveTo(hDC, X, Y);
    SelectObject(hDC, hPen);
    LineTo(hDC, x2, y2);

    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
}

void
DrawHands(HDC hDC, PTDDATAOBJ PTDate, BOOL bRepainting)
{
    int hourX,hourY,minX,minY,secX,secY;

    if (!PTDate || !hDC)
    {
        return;
    }

    if (bRepainting)
    {
        hourX = (PTDate->ccLocX+4);
        hourY = (PTDate->ccLocY);
        minX  = (PTDate->ccLocX+2);
        minY  = (PTDate->ccLocY);
        secX  = (PTDate->ccLocX);
        secY  = (PTDate->ccLocY);

        DrawSecondHand(hDC, PTDate, secX, secY);
        DrawFatHand(hDC, PTDate, hourX, hourY, TRUE);
        DrawFatHand(hDC, PTDate, minX, minY, FALSE);
        SetBkMode(hDC, TRANSPARENT);
        DrawSecondHand(hDC, PTDate, secX, secY);
    }
    else
    {
        hourX = (PTDate->ccLocX+4);
        hourY = (PTDate->ccLocY);
        minX  = (PTDate->ccLocX+2);
        minY  = (PTDate->ccLocY);
        secX  = (PTDate->ccLocX);
        secY  = (PTDate->ccLocY);
	
        DrawSecondHand(hDC, PTDate, secX, secY);
        DrawFatHand(hDC, PTDate, hourX, hourY, TRUE);
        DrawFatHand(hDC, PTDate, minX, minY, FALSE);
        SetBkMode(hDC, TRANSPARENT);
        DrawSecondHand(hDC, PTDate, secX, secY);
    }
}

void UpdateClocks(HWND hWnd, PTDDATAOBJ PTDate)
{
    HDC hDC;
	
    if (!PTDate)
    {
        return;
    }

    SetClockTimes(&(PTDate->curClockTime), PTDate);
    if ((PTDate->curClockTime.handSec == PTDate->oldClockTime.handSec)
        && (PTDate->curClockTime.handMin == PTDate->oldClockTime.handMin)
        && (PTDate->curClockTime.handHour == PTDate->oldClockTime.handHour))
    {
        return;
    }

    hDC = GetDC(hWnd);
    DrawClockFace(hDC, PTDate);
    DrawHands(hDC, PTDate, FALSE);
    ReleaseDC(hWnd, hDC);
}

LRESULT
CALLBACK
ClockWndProc(HWND hWnd,
             UINT message,
             WPARAM wParam,
             LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;
    PTDDATAOBJ PTDate;
    
    PTDate = (PTDDATAOBJ)GetWindowPtr(hWnd, GWLP_USERDATA);
    switch (message)
    {
        case (WM_CREATE):
        {
            SYSTEMTIME st = {0};
            
            RtlZeroMemory(&st, sizeof(SYSTEMTIME));
            GetTimes(&st);
            PTDate = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TDDATAOBJ));
            if (!PTDate)
                return (-1);
            SetWindowPtr(hWnd, GWLP_USERDATA, PTDate);
            
            CreateClockObject(PTDate);

            hDC = GetDC(hWnd);
            SetLayout(hDC, LAYOUT_BITMAPORIENTATIONPRESERVED);
            ReleaseDC(hWnd, hDC);

            SetClockTimes(&(PTDate->curClockTime),PTDate);
            do
            {
                 SetClockTimes(&(PTDate->oldClockTime),PTDate);
            } while ((PTDate->curClockTime.handSec == PTDate->oldClockTime.handSec)
                     && (PTDate->curClockTime.handMin == PTDate->oldClockTime.handMin)
                     && (PTDate->curClockTime.handHour == PTDate->oldClockTime.handHour));
            ResizeClock(PTDate,
                        ((LPCREATESTRUCT)lParam)->cx,
                        ((LPCREATESTRUCT)lParam)->cy);
            SetTimer(hWnd, ID_CLOCK_TIMER, ID_CLOCK_MSEC, 0L);
            break;
        }

        case (WM_SIZE):
        {
            if (PTDate)
            {
                ResizeClock(PTDate, (int)LOWORD(lParam), (int)HIWORD(lParam));
            }
            break;
        }

        case (WM_SYSCOLORCHANGE):
        {
            RemoveClockObject(PTDate);
            CreateClockObject(PTDate);
            break;
        }

        case (WM_TIMECHANGE):
        {
            InvalidateRect(hWnd, NULL, TRUE);
        }
        case (WM_TIMER):
        {
            SetClockTimes(&(PTDate->curClockTime), PTDate);
            do
            {
                SetClockTimes(&(PTDate->oldClockTime), PTDate);
            } while ((PTDate->curClockTime.handSec == PTDate->oldClockTime.handSec)
                  && (PTDate->curClockTime.handMin == PTDate->oldClockTime.handMin)
                  && (PTDate->curClockTime.handHour == PTDate->oldClockTime.handHour));
            break;
        }

        case (WM_PAINT):
        {
            RECT rc;
            
            GetClientRect(GetParent(hWnd), &rc);
            InvalidateRect(GetParent(hWnd), &rc, TRUE);
            BeginPaint(hWnd, &ps);
            SetClockTimes(&(PTDate->curClockTime), PTDate);
            DrawClockFace(ps.hdc, PTDate);
            DrawHands(ps.hdc, PTDate, TRUE);
            EndPaint(hWnd, &ps);
            break;
        }

        case (WM_DESTROY):
        {
            if (PTDate)
            {
                if (PTDate->bTimer)
                {
                    KillTimer(hWnd, ID_CLOCK_TIMER);
                    PTDate->bTimer = FALSE;
                }
                RemoveClockObject(PTDate);
            }
            HeapFree(GetProcessHeap(), 0, PTDate);
            SetWindowPtr(hWnd, 0, PTDate);
            break;
        }

        case (CLM_STOPCLOCK):
        {
            if (PTDate->bTimer)
            {
                KillTimer(hWnd, ID_CLOCK_TIMER);
                PTDate->bTimer = FALSE;
            }
            break;
        }

        case (CLM_STARTCLOCK):
        {
            if (!PTDate->bTimer)
            {
                SYSTEMTIME st = {0};
                
                RtlZeroMemory(&st, sizeof(SYSTEMTIME));
                GetTimes(&st);
                SetClockTimes(&(PTDate->curClockTime),PTDate);
                PTDate->bTimer =(SetTimer(hWnd, ID_CLOCK_TIMER, ID_CLOCK_MSEC, 0L) != 0);

                hDC = GetDC(hWnd);
                DrawClockFace(hDC, PTDate);
                DrawHands(hDC, PTDate, FALSE);
                ReleaseDC(hWnd, hDC);
            }
            break;
        }

        case (CLM_UPDATECLOCK):
        {
            SetClockTimes(&(PTDate->curClockTime), PTDate);
            hDC = GetDC(hWnd);
            DrawClockFace(hDC, PTDate);
            DrawHands(hDC, PTDate, FALSE);
            ReleaseDC(hWnd, hDC);
            break;
        }

        default:
        {
            return (DefWindowProcW(hWnd, message, wParam, lParam));
        }
    }
    return (0);
}

BOOL
RegisterClockControl(VOID)
{
    WNDCLASSEXW wc = {0};
    
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PTDDATAOBJ);
    wc.hInstance = hApplet;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szClockWndClass;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = ClockWndProc;
    
    return (RegisterClassExW(&wc) != (ATOM)0);
}

VOID
UnregisterClockControl(VOID)
{
    UnregisterClassW(szClockWndClass,
                     hApplet);
}
