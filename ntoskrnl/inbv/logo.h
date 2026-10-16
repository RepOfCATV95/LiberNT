/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation header
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#include "logo_root.h"
#include "logo_w2k.h"

/* Shared bitmap structures used by bootanim.c and inbvgop.c */
typedef struct tagRGBQUAD
{
    UCHAR rgbBlue;
    UCHAR rgbGreen;
    UCHAR rgbRed;
    UCHAR rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFOHEADER
{
    ULONG  biSize;
    LONG   biWidth;
    LONG   biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG  biCompression;
    ULONG  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    ULONG  biClrUsed;
    ULONG  biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

//
// Boot Splash-Screen Functions
//
VOID
NTAPI
BootAnimTickProgressBar(
    _In_ ULONG SubPercentTimes100);

CODE_SEG("INIT")
VOID
NTAPI
DisplayBootBitmap(
    _In_ BOOLEAN TextMode);

CODE_SEG("INIT")
VOID
NTAPI
FinalizeBootLogo(VOID);

VOID
NTAPI
DisplayShutdownBitmap(VOID);

VOID
NTAPI
DisplayShutdownText(VOID);
