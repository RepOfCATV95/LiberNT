/*
 * PROJECT:     LiberNT Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/clock.h
 * PURPOSE:     Header for analog clock functionality
 * COPYRIGHT:   Copyright 1997-2028 Republic of CATV 95 <catv95republic@proton.me>
 */

#pragma once

#include <math.h>

#define CLOCK_PI                       3.14159265358979323846
#define ID_CLOCK_TIMER                 1
#define ID_CLOCK_MSEC                  450

#define DARK_COLOR_CLOCK               RGB(83,29,56)
#define LIGHT_COLOR_CLOCK              RGB(165,58,110)
#define SECOND_HAND_COLOR_CLOCK        RGB(255,3,3)

#define BKG_DARK_COLOR_CLOCK           RGB(0,128,128)
#define BKG_BORDER_DARK_COLOR_CLOCK    RGB(64,64,64)
#define BKG_BORDER_LIGHT_COLOR_CLOCK   RGB(128,128,128)

/* Struct needed for handling of three values representing the clock time */
typedef struct
{
   int handSec;
   int handMin;
   int handHour;
} ClockHandTime;

typedef struct _TDDATAOBJ
{
    ClockHandTime curClockTime;
    ClockHandTime oldClockTime;
    
    /* Size of analog clock itself */
    int ccWidth;
    int ccHeight;
    
    /* Location of clock hands */
    int ccLocX;
    int ccLocY;
    
    /* Two fat hands' colors and second hand's color */
    HBRUSH hDarkHandColor;
    HBRUSH hLightHandColor;
    HPEN   hSecondPenColor;
    
    /* Clock Face colors */
    HBRUSH hDarkBorderColor;
    HBRUSH hLightBorderColor;
    
    /* Timer handle */
    BOOL bTimer;
} TDDATAOBJ, *PTDDATAOBJ;

void SetClockTimes(ClockHandTime* pt, PTDDATAOBJ PTDate);
void CreateClockObject(PTDDATAOBJ PTDate);
void RemoveClockObject(PTDDATAOBJ PTDate);
void ResizeClock(PTDDATAOBJ PTDate, int newWidth, int newHeight);
void DrawClockFace(HDC hDC, PTDDATAOBJ PTDate);
void DrawFatHand(HDC hDC, PTDDATAOBJ PTDate, int X, int Y, BOOL IsHourHand);
void DrawSecondHand(HDC hDC, PTDDATAOBJ PTDate, int X, int Y);
void DrawHands(HDC hDC, PTDDATAOBJ PTDate, BOOL bRepainting);
void UpdateClocks(HWND hWnd, PTDDATAOBJ PTDate);
 

