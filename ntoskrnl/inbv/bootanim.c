/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation
 * COPYRIGHT:   Copyright 2007 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright 2012-2022 Hermès Bélusca-Maïto
 *              Copyright 2017-2018 Stanislav Motylkov
 *              Copyright 2019-2020 Yaroslav Kibysh
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <reactos/arc/arc.h>
#include "inbv/logo.h"
#include "inbv/inbvgop.h"

/* See also mm/ARM3/miarm.h */
#define MM_READONLY     1   // PAGE_READONLY
#define MM_READWRITE    4   // PAGE_WRITECOPY

/* GLOBALS *******************************************************************/

/*
 * BitBltAligned() alignments
 */
typedef enum _BBLT_VERT_ALIGNMENT
{
    AL_VERTICAL_TOP = 0,
    AL_VERTICAL_CENTER,
    AL_VERTICAL_BOTTOM
} BBLT_VERT_ALIGNMENT;

typedef enum _BBLT_HORZ_ALIGNMENT
{
    AL_HORIZONTAL_LEFT = 0,
    AL_HORIZONTAL_CENTER,
    AL_HORIZONTAL_RIGHT
} BBLT_HORZ_ALIGNMENT;

#define INBV_ROTBAR_IMPLEMENTED

extern ULONG ProgressBarLeft, ProgressBarTop;
extern BOOLEAN ShowProgressBar;
extern BOOLEAN Win2KHaxor;

#ifdef INBV_ROTBAR_IMPLEMENTED

/* Specifies on how kernel calls BOOTVID to display a boot screen based on LOGO_TYPE */
typedef enum _LOGO_TYPE
{
    LOGO_UNSPECIFIED,
    LOGO_PETRO,
    LOGO_GUTLAY
} LOGO_TYPE;

/* Windows 8+ boot animation variables */
static LOGO_TYPE LogoSelections = LOGO_UNSPECIFIED;

/* DPCs */
static KDPC BootAnimDpc;
static KTIMER BootAnimTimer;
static LARGE_INTEGER BootAnimBaseTimer;

/* These are variables being renamed from their Windows 2000 counterparts */
static ULONG AnimBasePosX;
static ULONG AnimBasePosY;
static ULONG AnimBaseWidth;
static ULONG AnimBaseHeight;
static ULONG AnimBaseHalfWidth;
static ULONG AnimBaseProgress;
static ULONG InTextMode;
static UCHAR RotLineBuffer[3200];
static BOOLEAN BootSeqEntered = FALSE;

/* Shows a boot screen from ReactOS version 0.2.x, otherwise a Windows 2000-styled boot screen is displayed instead. */
static BOOLEAN Haxor = FALSE;

#endif // INBV_ROTBAR_IMPLEMENTED

/* FADE-IN FUNCTION **********************************************************/

/* RGBQUAD and BITMAPINFOHEADER are now defined in logo.h */

static RGBQUAD MainPalette[16];

#define PALETTE_FADE_STEPS  12
#define PALETTE_FADE_TIME   (15 * 1000) /* 15 ms */

static VOID
BootLogoFadeIn(VOID)
{
    UCHAR PaletteBitmapBuffer[sizeof(BITMAPINFOHEADER) + sizeof(MainPalette)];
    PBITMAPINFOHEADER PaletteBitmap = (PBITMAPINFOHEADER)PaletteBitmapBuffer;
    LPRGBQUAD Palette = (LPRGBQUAD)(PaletteBitmapBuffer + sizeof(BITMAPINFOHEADER));
    ULONG Iteration, Index, ClrUsed;

    LARGE_INTEGER Delay;
    Delay.QuadPart = -(PALETTE_FADE_TIME * 10);

    /* Check if we are installed and we own the display */
    if (!InbvBootDriverInstalled ||
        (InbvGetDisplayState() != INBV_DISPLAY_STATE_OWNED))
    {
        return;
    }

    /*
     * Build a bitmap containing the fade-in palette. The palette entries
     * are then processed in a loop and set using VidBitBlt function.
     */
    ClrUsed = RTL_NUMBER_OF(MainPalette);
    RtlZeroMemory(PaletteBitmap, sizeof(BITMAPINFOHEADER));
    PaletteBitmap->biSize = sizeof(BITMAPINFOHEADER);
    PaletteBitmap->biBitCount = 4;
    PaletteBitmap->biClrUsed = ClrUsed;

    /*
     * Main animation loop.
     */
    for (Iteration = 0; Iteration <= PALETTE_FADE_STEPS; ++Iteration)
    {
        for (Index = 0; Index < ClrUsed; Index++)
        {
            Palette[Index].rgbRed = (UCHAR)
                (MainPalette[Index].rgbRed * Iteration / PALETTE_FADE_STEPS);
            Palette[Index].rgbGreen = (UCHAR)
                (MainPalette[Index].rgbGreen * Iteration / PALETTE_FADE_STEPS);
            Palette[Index].rgbBlue = (UCHAR)
                (MainPalette[Index].rgbBlue * Iteration / PALETTE_FADE_STEPS);
        }

        /* Do the animation */
        InbvAcquireLock();
        VidBitBlt(PaletteBitmapBuffer, 0, 0);
        InbvReleaseLock();

        /* Wait for a bit */
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);
    }
}

static VOID
BitBltPalette(
    IN PVOID Image,
    IN BOOLEAN NoPalette,
    IN ULONG X,
    IN ULONG Y)
{
    LPRGBQUAD Palette;
    RGBQUAD OrigPalette[RTL_NUMBER_OF(MainPalette)];

    /* If requested, remove the palette from the image */
    if (NoPalette)
    {
        /* Get bitmap header and palette */
        PBITMAPINFOHEADER BitmapInfoHeader = Image;
        Palette = (LPRGBQUAD)((PUCHAR)Image + BitmapInfoHeader->biSize);

        /* Save the image original palette and remove palette information */
        RtlCopyMemory(OrigPalette, Palette, sizeof(OrigPalette));
        RtlZeroMemory(Palette, sizeof(OrigPalette));
    }

    /* Draw the image */
    InbvBitBlt(Image, X, Y);

    /* Restore the image original palette */
    if (NoPalette)
    {
        RtlCopyMemory(Palette, OrigPalette, sizeof(OrigPalette));
    }
}

static VOID
BitBltAligned(
    IN PVOID Image,
    IN BOOLEAN NoPalette,
    IN BBLT_HORZ_ALIGNMENT HorizontalAlignment,
    IN BBLT_VERT_ALIGNMENT VerticalAlignment,
    IN ULONG MarginLeft,
    IN ULONG MarginTop,
    IN ULONG MarginRight,
    IN ULONG MarginBottom)
{
    PBITMAPINFOHEADER BitmapInfoHeader = Image;
    VID_DISPLAY_INFO DisplayInfo;
    ULONG X, Y;

    InbvQueryDisplayInfo(&DisplayInfo);

    /* Calculate X */
    switch (HorizontalAlignment)
    {
        case AL_HORIZONTAL_LEFT:
            X = MarginLeft - MarginRight;
            break;

        case AL_HORIZONTAL_CENTER:
            X = MarginLeft - MarginRight + (DisplayInfo.Width - BitmapInfoHeader->biWidth + 1) / 2;
            break;

        case AL_HORIZONTAL_RIGHT:
            X = MarginLeft - MarginRight + DisplayInfo.Width - BitmapInfoHeader->biWidth;
            break;

        default:
            /* Unknown */
            return;
    }

    /* Calculate Y */
    switch (VerticalAlignment)
    {
        case AL_VERTICAL_TOP:
            Y = MarginTop - MarginBottom;
            break;

        case AL_VERTICAL_CENTER:
            Y = MarginTop - MarginBottom + (DisplayInfo.Height - BitmapInfoHeader->biHeight + 1) / 2;
            break;

        case AL_VERTICAL_BOTTOM:
            Y = MarginTop - MarginBottom + DisplayInfo.Height - BitmapInfoHeader->biHeight;
            break;

        default:
            /* Unknown */
            return;
    }

    /* Finally draw the image */
    BitBltPalette(Image, NoPalette, X, Y);
}

/* FUNCTIONS *****************************************************************/

#if 0 // temporarily ifdef'd out
CODE_SEG("INIT")
BOOLEAN
NTAPI
BootAnimInitialize(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG Count)
{
    /* Return install state */
    return TRUE;
}
#endif

static
VOID
DrawProgressBars(
    _In_ ULONG   SubPercentTimes100,
    _In_ BOOLEAN InROS,
    _In_ ULONG   FillCount
)
{
    UCHAR PBColor;
    int i;

    if ( InROS && !InTextMode )
    {
		i = 0;

        /* Set PBColor to white for ReactOS 0.2.x boot screen */
        PBColor = BV_COLOR_WHITE;

        /* If 0%, draw also the left round corner */
        if (SubPercentTimes100 <= 10*100)
        {
            VidSolidColorFill(ProgressBarLeft - 6,
                              ProgressBarTop + 6,
                              ProgressBarLeft - 6,
                              ProgressBarTop - 6 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft - 5,
                              ProgressBarTop + 4,
                              ProgressBarLeft - 5,
                              ProgressBarTop - 4 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft - 4,
                              ProgressBarTop + 3,
                              ProgressBarLeft - 4,
                              ProgressBarTop - 3 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft - 3,
                              ProgressBarTop + 2 ,
                              ProgressBarLeft - 3,
                              ProgressBarTop - 2 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft - 2,
                              ProgressBarTop + 1,
                              ProgressBarLeft - 2,
                              ProgressBarTop - 1 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft - 1,
                              ProgressBarTop + 1,
                              ProgressBarLeft - 1,
                              ProgressBarTop - 1 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);
        }

        /* Fill the progress bar */
        VidSolidColorFill(ProgressBarLeft,
                          ProgressBarTop,
                          ProgressBarLeft + FillCount,
                          ProgressBarTop + VID_PROGRESS_BAR_ROS_HEIGHT,
                          PBColor);

        /* If 100%, draw also the right round corner */
        if (SubPercentTimes100 >= 100*100)
        {
            VidSolidColorFill(ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 1,
                              ProgressBarTop + 1,
                              ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 1,
                              ProgressBarTop - 1 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 2,
                              ProgressBarTop + 1,
                              ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 2,
                              ProgressBarTop - 1 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 3,
                              ProgressBarTop + 2 ,
                              ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 3,
                              ProgressBarTop - 2 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 4,
                              ProgressBarTop + 3,
                              ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 4,
                              ProgressBarTop - 3 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 5,
                              ProgressBarTop + 4,
                              ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 5,
                              ProgressBarTop - 4 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);

            VidSolidColorFill(ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 6,
                              ProgressBarTop + 6,
                              ProgressBarLeft + VID_PROGRESS_BAR_ROS_WIDTH + 6,
                              ProgressBarTop - 6 + VID_PROGRESS_BAR_ROS_HEIGHT,
                              PBColor);
        }
    }
    else
    {
        // TODO: Use shade of yellow for AUTOCHK screen (RISC-V 64-bit builds only)
        if ( InTextMode )
        {
            /* Set PBColor for /SOS boot screen */
            PBColor = BV_COLOR_LIGHT_BLUE;
        }
        else
        {
            /* Set PBColor depending on different architecture (IA-32, AMD64, ARM64) */
            if (SharedUserData->NtProductType == NtProductWinNt)
            {
#if defined(_M_AMD64) || defined(_M_X64) || defined(_M_ARM64)
                PBColor = BV_COLOR_LIGHT_GREEN;
#else
                PBColor = BV_COLOR_RED;
#endif
            }
            else
            {
#if defined(_M_AMD64) || defined(_M_X64) || defined(_M_ARM64)
                PBColor = BV_COLOR_YELLOW;
#else
                PBColor = BV_COLOR_RED;
#endif
            }
        }

        i = 0;
        while (i < FillCount)
        {
            VidSolidColorFill(ProgressBarLeft + (i * VID_PROGRESS_BAR_EXTRA_WIDTH),
                              ProgressBarTop,
                              ProgressBarLeft + ((i + 1) * VID_PROGRESS_EXTRA_WIDTH) - 2,
                              ProgressBarTop + VID_PROGRESS_BAR_GEN_HEIGHT,
                              PBColor);
            i+=1;
        }
    }
}

/**
 * @brief
 * Ticks the progress bar. Used by InbvUpdateProgressBar() and related.
 *
 * @param[in]   SubPercentTimes100
 * The progress percentage, scaled up by 100.
 *
 * @return None.
 **/
VOID
NTAPI
BootAnimTickProgressBar(
    _In_ ULONG SubPercentTimes100)
{
    ULONG FillCount;

    /* Make sure the progress bar is enabled, that we own and are installed */
    ASSERT(ShowProgressBar &&
           InbvBootDriverInstalled &&
           (InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED));

    /* Compute fill count */
    if ( Haxor )
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Calculate and draw progress bar indicator */
        FillCount = VID_PROGRESS_BAR_ROS_SIZE * SubPercentTimes100 / (100 * 100);
        DrawProgressBars(SubPercentTimes100, TRUE, FillCount);

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Calculate and draw progress bar indicator */
        FillCount = VID_PROGRESS_BAR_EXTRA_SIZE * SubPercentTimes100 / (100 * 100);
        DrawProgressBars(SubPercentTimes100, FALSE, FillCount);

        /* Release the lock */
        InbvReleaseLock();
    }   
}

#ifdef INBV_ROTBAR_IMPLEMENTED

static
VOID
NTAPI
InbvRotationThread(
    _In_ PKDPC Dpc,
    _In_ PVOID Context,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (LogoSelections == LOGO_GUTLAY)
	{
		KeCancelTimer(&BootAnimTimer);
		return;
	}

    InbvCheckDisplayOwnership();
    if (InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED)
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Left side of bar */
        InbvBufferToScreenBlt(RotLineBuffer,
                              AnimBasePosX + AnimBaseProgress,
                              AnimBasePosY,
                              AnimBaseWidth - AnimBaseProgress,
                              AnimBaseHeight,
                              AnimBaseHalfWidth);

        /* Right side of bar */
        InbvBufferToScreenBlt((PUCHAR)RotLineBuffer + ((ULONG)(AnimBaseWidth - AnimBaseProgress) >> 1),
                              AnimBasePosX,
                              AnimBasePosY,
                              AnimBaseProgress,
                              AnimBaseHeight,
                              AnimBaseHalfWidth);

        /* Increment the anim index */
        AnimBaseProgress+=4;
        if (AnimBaseProgress>=AnimBaseWidth)
            AnimBaseProgress=0;

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        KeCancelTimer(&BootAnimTimer);
    }
}

#endif // INBV_ROTBAR_IMPLEMENTED

CODE_SEG("INIT")
static
VOID
NTAPI
DisplayFilter(
    _Inout_ PCSTR* String)
{
    /* Windows hack to skip first dots displayed by AUTOCHK */
    static BOOLEAN NoNT4 = FALSE;

    /* If "." is given set *String to empty string */
    if (NoNT4==FALSE && strcmp(*String, ".") == 0)
        *String = "";

    if (**String)
    {
        /* Remove the filter */
        InbvInstallDisplayStringFilter(NULL);

        /* Turn off hacks */
        NoNT4 = TRUE;
		
#ifdef INBV_ROTBAR_IMPLEMENTED
        /*
         * If Windows 8+ GOP-styled boot screen is shown in NT6+ builds without BOOTVID fallback, then
         * halt the boot animation, change logo type to Windows 2000 and do the rest before entering an AUTOCHK (text-mode) screen
         */
		if ( LogoSelections == LOGO_GUTLAY )
		{
			LogoSelections = LOGO_PETRO;
		}
#endif

		/* Avoid drawing progress bar while in process of entering text-mode boot screen */
		if ( !BootSeqEntered && !Win2KHaxor )
			BootSeqEntered = TRUE;

        /* If ReactOS 0.2.x boot screen is shown in Embedded edition or RISC-V version of CATV 95 LiberNT, then clear the flag first */
		if ( Haxor )
			Haxor = FALSE;

        /* Stop the DPC timer before entering AUTOCHK boot screen */
		if ( BootAnimTimer )
            KeCancelTimer(&BootAnimTimer);
		
		/* Draw text screen */
		DisplayBootBitmap(TRUE);
	}
}

CODE_SEG("INIT")
VOID
NTAPI
DisplayBootBitmap(
    _In_ BOOLEAN TextMode)
{
    VID_DISPLAY_INFO DisplayInfo;
    static BOOLEAN FirstCall = TRUE;
    static BOOLEAN HasGOP = FALSE;
#ifdef INBV_ROTBAR_IMPLEMENTED
    LOGO_TYPE TempLogoSel = LOGO_UNSPECIFIED;
#endif

    InbvQueryDisplayInfo(&DisplayInfo);

    HasGOP = InbvGopHandleBootBitmap();

    if ( HasGOP )
    {
#ifdef INBV_ROTBAR_IMPLEMENTED
        TempLogoSel = LOGO_GUTLAY;
		LogoSelections = TempLogoSel;
#endif
        return;
    }

#ifdef INBV_ROTBAR_IMPLEMENTED
    TempLogoSel = LOGO_PETRO;
    LogoSelections = TempLogoSel;
#endif

    if (TextMode && Win2KHaxor)
    {
        PUCHAR Header = NULL;

        InTextMode = TRUE;
        if ( Haxor )
            Haxor = FALSE;

        /* Is the boot driver installed? */
        if (!InbvBootDriverInstalled) return;
		
        /* Emulate Windows 2000-styled design of text-mode boot screen */
        InbvSetTextColor(BV_COLOR_WHITE);
		/* NOTE: GOP-specific display info cannot be used for Windows 2000 AUTOCHK screen */
        InbvSolidColorFill(0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1,BV_COLOR_BLACK);
        /* Draw a bottom gray box */
        InbvSolidColorFill(ROTATION_BAR_TEXTMODE_OFFSET, VID_FOOTER_BG_TOP, VID_BOTTOM_BOX_WIDTH, VID_BOTTOM_BOX_HEIGHT, BV_COLOR_LIGHT_RED);
		
        if (SharedUserData->NtProductType == NtProductWinNt)
		{
            /* Professional - Get resources */
            Header = InbvGetResourceAddress(IDB_PROFESSIONAL_HEADER);
		}
        else
        {
            /* Server - Get resources */
			Header = InbvGetResourceAddress(IDB_SERVER_HEADER);
        }

        /* Set the scrolling region */
        InbvSetScrollRegion(VID_SCROLL_AREA_LEFT,
                            VID_SCROLL_AREA_TOP,
                            VID_SCROLL_AREA_RIGHT,
                            VID_SCROLL_AREA_BOTTOM);

        /* Set the progress bar coordinates */
        InbvSetProgressBarCoordinates(VID_PROGRESS_BAR_TEXT_LEFT,
                                      VID_PROGRESS_BAR_TEXT_TOP);

        /* Make sure we have an AUTOCHK-style boot screen bitmap */
        if (Header)
        {
            /* BitBlt this bitmap on the screen */
            InbvBitBlt(Header, 0, 0);
        }

#ifdef INBV_ROTBAR_IMPLEMENTED
        /* Set the boot-time animation offsets */
        AnimBasePosX       =   ROTATION_BAR_TEXTMODE_OFFSET;
		AnimBasePosY       =   ROTATION_BAR_TEXTMODE_HEIGHT;
		AnimBaseWidth      =   ROTATION_BAR_TEXTMODE_WIDTH;
		AnimBaseHeight     =   ROTATION_BAR_TEXTMODE_SIZE;
		AnimBaseHalfWidth  =   ROTATION_BAR_GEN_HALFER;
#endif

        /* Draw four blocks of color representing the progress bar if kernel transitions directly into a verbose (AUTOCHK) boot screen */
        if ( FirstCall )
        {
            /* Fill the color blocks representing the progress bar */
            InbvSolidColorFill(237,443,401,443,BV_COLOR_BLUE);
			InbvSolidColorFill(237,454,401,454,BV_COLOR_WHITE);
			InbvSolidColorFill(237,443,237,454,BV_COLOR_BLUE);
			InbvSolidColorFill(401,443,401,454,BV_COLOR_WHITE);
        }
    }
    else
    {
        PUCHAR Screen = NULL;

        InTextMode = FALSE;

        /* Is the boot driver installed? */
        if (!InbvBootDriverInstalled) return;

        /* Fill the screen with shade of blue color before drawing the boot screen. */
        InbvSolidColorFill(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BV_COLOR_GREEN);

        /* Drop into ReactOS 0.2.x boot screen if running Embedded edition */
        if (ExVerifySuite(EmbeddedNT))
            Haxor = TRUE;
        else
            Haxor = FALSE;

        if (Haxor)
        {
            /* Load the embedded edition boot screen */
            Screen = InbvGetResourceAddress(IDB_BOOT_SCREEN_POWERED);

            /* Set progress bar coordinates */
            InbvSetProgressBarCoordinates(VID_PROGRESS_BAR_NORMAL_ROS_LEFT,
                                          VID_PROGRESS_BAR_NORMAL_ROS_TOP);
        }
        else
        {
            /* Load the client/server boot screen */
            if (SharedUserData->NtProductType == NtProductWinNt)
                Screen = InbvGetResourceAddress(IDB_BOOT_SCREEN);
            else
                Screen = InbvGetResourceAddress(IDB_BOOT_SCREEN_SERVER);

            /* Set progress bar coordinates */
            InbvSetProgressBarCoordinates(VID_PROGRESS_BAR_NORMAL_LEFT,
                                          VID_PROGRESS_BAR_NORMAL_TOP);
        }

        /* Make sure we have a boot screen bitmap */
		if ( Screen )
        {
            InbvBitBlt(Screen, 0, 0);
        }

#ifdef INBV_ROTBAR_IMPLEMENTED
        if ( Haxor )
        {
            /* Set the boot-time animation offsets */
            AnimBasePosX      =  ROTATION_BAR_NORMAL_ROS_OFFSET;
            AnimBasePosY      =  ROTATION_BAR_NORMAL_ROS_HEIGHT;
            AnimBaseWidth     =  ROTATION_BAR_NORMAL_ROS_WIDTH;
            AnimBaseHeight    =  ROTATION_BAR_NORMAL_ROS_SIZE;
            AnimBaseHalfWidth =  ROTATION_BAR_GEN_HALFER;
        }
        else
        {
            /* Set animation offsets */
            AnimBasePosX      =  ROTATION_BAR_NORMAL_OFFSET;
            AnimBasePosY      =  ROTATION_BAR_NORMAL_HEIGHT;
            AnimBaseWidth     =  ROTATION_BAR_NORMAL_WIDTH;
            AnimBaseHeight    =  ROTATION_BAR_NORMAL_SIZE;
            AnimBaseHalfWidth =  ROTATION_BAR_GEN_HALFER;
        }
#endif

        if (Win2KHaxor)
        {
            /* Set filter which will draw text display if needed */
            InbvInstallDisplayStringFilter(DisplayFilter);
        }
    }

    if ( HasGOP && !Win2KHaxor )
        return;

    /* Initialize the animation, store the line region to rotate in a global buffer, do the rest and set FirstCall to FALSE */
#ifdef INBV_ROTBAR_IMPLEMENTED
    AnimBaseProgress = 0;
    InbvScreenToBufferBlt(RotLineBuffer,AnimBasePosX,AnimBasePosY,AnimBaseWidth,AnimBaseHeight,AnimBaseHalfWidth);
	KeInitializeDpc(&BootAnimDpc, InbvRotationThread, NULL);
	KeInitializeTimer(&BootAnimTimer);
	BootAnimBaseTimer.QuadPart = 50 * 10000LL;
	KeSetTimerEx(&BootAnimTimer, BootAnimBaseTimer, &BootAnimDpc);
#endif
    if ( !BootSeqEntered && FirstCall )
    {
        BootSeqEntered = TRUE;
        FirstCall = FALSE;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
FinalizeBootLogo(VOID)
{
    VID_DISPLAY_INFO DisplayInfo;

    InbvQueryDisplayInfo(&DisplayInfo);

    /*
     * Leave whatever is on screen alone: the boot logo stays up until the
     * display driver takes the framebuffer over and InbvNotifyDisplayOwnershipLost
     * fades it out.
     */
    InbvGopLogoComplete();

    InbvAcquireLock();
    if (!InbvBootFadeEnabled && InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED)
    {
        VidSolidColorFill(0, 0, DisplayInfo.Width - 1, DisplayInfo.Height - 1, BV_COLOR_BLACK);
    }

    InbvReleaseLock();
}

VOID
NTAPI
DisplayShutdownBitmap(VOID)
{
    PUCHAR Logo1, Logo2;
	UCHAR PBColor;
    VID_DISPLAY_INFO DisplayInfo;
    ULONG CanvasLeft, CanvasTop;

    InbvQueryDisplayInfo(&DisplayInfo);
    CanvasLeft = (DisplayInfo.Width - SCREEN_WIDTH) / 2;
    CanvasTop = (DisplayInfo.Height - SCREEN_HEIGHT) / 2;

    /* Assign a color */
    PBColor = BV_COLOR_DARK_GRAY;

    /* Yes we do, cleanup for shutdown screen */
    // InbvResetDisplay();
    InbvInstallDisplayStringFilter(NULL);
    InbvEnableDisplayString(TRUE);
    InbvSolidColorFill(0, 0, DisplayInfo.Width - 1, DisplayInfo.Height - 1, PBColor);
    InbvSetScrollRegion(0, 0, DisplayInfo.Width - 1, DisplayInfo.Height - 1);

    /* Display shutdown logo and message */
    Logo1 = InbvGetResourceAddress(IDB_SHUTDOWN_MSG);
    if (SharedUserData->NtProductType == NtProductWinNt)
    {
        /* Professional */
        Logo2 = InbvGetResourceAddress(IDB_SHUTDOWN_LOGO_PRO);
    }
    {
        /* Server */
        Logo2 = InbvGetResourceAddress(IDB_SHUTDOWN_LOGO_SERVER);
    }

    /* Draw the logo and shutdown message at the center of the screen */
    if ((Logo1) && (Logo2))
    {
        InbvBitBlt(Logo1, CanvasLeft + VID_SHUTDOWN_MSG_LEFT, CanvasTop + VID_SHUTDOWN_MSG_TOP);
        InbvBitBlt(Logo2, CanvasLeft + VID_SHUTDOWN_LOGO_LEFT, CanvasTop + VID_SHUTDOWN_LOGO_TOP);
    }
}

VOID
NTAPI
DisplayShutdownText(VOID)
{
    static const CHAR Message[] = "The system may be powered off now.";
    CHAR Padding[81];
    VID_DISPLAY_INFO DisplayInfo;
    ULONG Columns, Rows, PaddingLength, Count, i;

    InbvQueryDisplayInfo(&DisplayInfo);
    Columns = DisplayInfo.CharacterWidth ? DisplayInfo.Width / DisplayInfo.CharacterWidth : 80;
    Rows = DisplayInfo.CharacterHeight ? DisplayInfo.Height / DisplayInfo.CharacterHeight : 34;

    for (i = 0; i < (Rows > 2 ? (Rows - 2) / 2 : 0); ++i)
        InbvDisplayString("\r\n");

    PaddingLength = (Columns > sizeof(Message) - 1) ? (Columns - (sizeof(Message) - 1)) / 2 : 0;
    RtlFillMemory(Padding, sizeof(Padding) - 1, ' ');
    while (PaddingLength)
    {
        Count = min(PaddingLength, (ULONG)sizeof(Padding) - 1);
        Padding[Count] = ANSI_NULL;
        InbvDisplayString(Padding);
        PaddingLength -= Count;
    }
    InbvDisplayString(Message);
    InbvDisplayString("\r\n");
}
