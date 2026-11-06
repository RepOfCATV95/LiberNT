/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/dateandtime.c
 * PURPOSE:     Date & Time property page
 * COPYRIGHT:   Copyright 2004-2007 Eric Kohl
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006 Thomas Weidenmueller <w3seek@reactos.com>
 *
 */

#include "timedate.h"

static WNDPROC pOldWndProc = NULL;

static SHORT   wDateTime[7];
static SHORT   wPendingDateTime[7];
static BOOLEAN fDateDirty=FALSE;

static
VOID
FinishedSettingDates(VOID)
{
    int cc;
    
    for (cc=0;cc<6;cc++)
    {
        wPendingDateTime[cc] = -1;
    }
}

VOID
GetTimes(SYSTEMTIME* st)
{
    if ( fDateDirty )
        return;

    st->wMilliseconds =  0;
    st->wSecond      =  (WORD)wDateTime[0];
    st->wMinute      =  (WORD)wDateTime[1];
    st->wHour        =  (WORD)wDateTime[2];
    st->wMonth       =  (WORD)wDateTime[3];
    st->wDay         =  (WORD)wDateTime[4];
    st->wYear        =  (WORD)wDateTime[5];
    st->wDayOfWeek   =  (WORD)wDateTime[6];
}

VOID
SetTimes(SYSTEMTIME* st)
{
    st->wMilliseconds = 0;
    wDateTime[0] = (SHORT)st->wSecond;
    wDateTime[1] = (SHORT)st->wMinute;
    wDateTime[2] = (SHORT)st->wHour;
    wDateTime[3] = (SHORT)st->wMonth;
    wDateTime[4] = (SHORT)st->wDay;
    wDateTime[5] = (SHORT)st->wYear;
    wDateTime[6] = (SHORT)st->wDayOfWeek;
}

int
GetDateTime(UINT Type)
{
    int aaa = -1;
    switch (Type)
    {
        /* Second */
        case 0:
        {
            aaa = (int)wDateTime[0];
            break;
        }

        /* Minute */
        case 1:
        {
            aaa = (int)wDateTime[1];
            break;
        }

        case 2:
        {
            aaa = (int)wDateTime[2];
            break;
        }

        case 3:
        {
            aaa = (int)wDateTime[3];
            break;
        }

        case 4:
        {
            aaa = (int)wDateTime[4];
            break;
        }

        case 5:
        {
            aaa = (int)wDateTime[5];
            break;
        }

        case 6:
        {
            aaa = (int)wDateTime[6];
            break;
        }

        /* Invalid date type */
        default:
        {
            aaa = -1;
            break;
        }
    }
    
    return aaa;
}

BOOL
SystemSetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    HANDLE hToken;
    DWORD PrevSize;
    TOKEN_PRIVILEGES priv, previouspriv;
    BOOL Ret = FALSE;

    /*
     * Enable the SeSystemtimePrivilege privilege
     */

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken))
    {
        priv.PrivilegeCount = 1;
        priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (LookupPrivilegeValueW(NULL,
                                  SE_SYSTEMTIME_NAME,
                                  &priv.Privileges[0].Luid))
        {
            if (AdjustTokenPrivileges(hToken,
                                      FALSE,
                                      &priv,
                                      sizeof(previouspriv),
                                      &previouspriv,
                                      &PrevSize) &&
                GetLastError() == ERROR_SUCCESS)
            {
                /*
                 * We successfully enabled it, we're permitted to change the time.
                 * Call SetLocalTime twice to ensure correct results.
                 */
                Ret = SetLocalTime(lpSystemTime) &&
                      SetLocalTime(lpSystemTime);

                /*
                 * For the sake of security, restore the previous status again
                 */
                if (previouspriv.PrivilegeCount > 0)
                {
                    AdjustTokenPrivileges(hToken,
                                          FALSE,
                                          &previouspriv,
                                          0,
                                          NULL,
                                          0);
                }
            }
        }
        CloseHandle(hToken);
    }

    return Ret;
}

static
VOID
ProcessLocalTime(SYSTEMTIME st)
{
    TIME_ZONE_INFORMATION tzi;
    DWORD dwTZI;
    SYSTEMTIME procUTC,utcSt,aTime;
    FILETIME ft;
    BOOLEAN bFail = FALSE;

    RtlZeroMemory(&procUTC, sizeof(SYSTEMTIME));
    RtlZeroMemory(&utcSt, sizeof(SYSTEMTIME));
    RtlZeroMemory(&aTime, sizeof(SYSTEMTIME));
    RtlZeroMemory(&tzi, sizeof(TIME_ZONE_INFORMATION));

    dwTZI = GetTimeZoneInformation(&tzi);
    bFail = (dwTZI == TIME_ZONE_ID_UNKNOWN || dwTZI == TIME_ZONE_ID_INVALID);
    
    /* If time zone is unknown or invalid, then set it as local time without time zone */
    if ( bFail )
    {
        SystemSetLocalTime(&st);
        return;
    }
    
    TzSpecificLocalTimeToSystemTime(&tzi, &st, &procUTC);
    bFail = (!SystemTimeToFileTime(&procUTC, &ft));
    
    if ( bFail )
    {
        SystemSetLocalTime(&st);
        SetTimes(&aTime);
        return;
    }
    
    bFail = (!FileTimeToSystemTime(&ft, &utcSt));
    if ( bFail )
    {
        SystemSetLocalTime(&st);
        SetTimes(&aTime);
        return;
    }
    
    dwTZI = GetTimeZoneInformation(&tzi);
    bFail = (dwTZI == TIME_ZONE_ID_UNKNOWN || dwTZI == TIME_ZONE_ID_INVALID);
    
    /* If time zone is unknown or invalid, then set it as local time without time zone */
    if ( bFail )
    {
        SystemSetLocalTime(&st);
        SetTimes(&aTime);
        return;
    }
    
    SystemTimeToTzSpecificLocalTime(&tzi,&utcSt,&aTime);
    SystemSetLocalTime(&aTime);
    SetTimes(&aTime);
}

static
VOID
SetLocalSystemTime(HWND hwnd)
{
    SYSTEMTIME Time;
    int cc;

    RtlZeroMemory(&Time, sizeof(SYSTEMTIME));
    
    Time.wMilliseconds = 0;
    Time.wSecond = wPendingDateTime[0];
    Time.wMinute = wPendingDateTime[1];
    Time.wHour = wPendingDateTime[2];
    Time.wMonth = wPendingDateTime[3];
    Time.wDay = wPendingDateTime[4];
    Time.wYear = wPendingDateTime[5];
    Time.wDayOfWeek = wPendingDateTime[6];

    ProcessLocalTime(Time);

    SetWindowLongPtrW(hwnd,
                          DWLP_MSGRESULT,
                          PSNRET_NOERROR);

    SendMessageW(GetDlgItem(hwnd,IDC_MONTHCALENDAR),
                 MCCM_RESET,
                 (WPARAM)&Time,
                 0);

    /* Broadcast the time change message */
    SendMessageW(HWND_BROADCAST,
                 WM_TIMECHANGE,
                 0,
                 0);
    
    for (cc=0;cc<6;cc++)
        wPendingDateTime[cc] = -1;
}


static VOID
SetTimeZoneName(HWND hwnd)
{
    TIME_ZONE_INFORMATION TimeZoneInfo;
    WCHAR TimeZoneString[128];
    WCHAR TimeZoneText[128];
    WCHAR TimeZoneName[128];
    DWORD TimeZoneId;

    TimeZoneId = GetTimeZoneInformation(&TimeZoneInfo);

    LoadStringW(hApplet, IDS_TIMEZONETEXT, TimeZoneText, 128);

    switch (TimeZoneId)
    {
        case TIME_ZONE_ID_STANDARD:
        case TIME_ZONE_ID_UNKNOWN:
            wcscpy(TimeZoneName, TimeZoneInfo.StandardName);
            break;

        case TIME_ZONE_ID_DAYLIGHT:
            wcscpy(TimeZoneName, TimeZoneInfo.DaylightName);
            break;

        case TIME_ZONE_ID_INVALID:
        default:
            LoadStringW(hApplet, IDS_TIMEZONEINVALID, TimeZoneName, 128);
            break;
    }

    wsprintfW(TimeZoneString, TimeZoneText, TimeZoneName);
    SendDlgItemMessageW(hwnd, IDC_TIMEZONE, WM_SETTEXT, 0, (LPARAM)TimeZoneString);
}


static VOID
FillMonthsComboBox(HWND hCombo)
{
    SYSTEMTIME LocalDate = {0};
    WCHAR szBuf[64];
    INT i;
    UINT Month;

    GetLocalTime(&LocalDate);

    SendMessageW(hCombo,
                 CB_RESETCONTENT,
                 0,
                 0);

    for (Month = 1;
         Month <= 13;
         Month++)
    {
        i = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                           ((Month < 13) ? LOCALE_SMONTHNAME1 + Month - 1 : LOCALE_SMONTHNAME13),
                           szBuf,
                           sizeof(szBuf) / sizeof(szBuf[0]));
        if (i > 1)
        {
            i = (INT)SendMessageW(hCombo,
                                  CB_ADDSTRING,
                                  0,
                                  (LPARAM)szBuf);
            if (i != CB_ERR)
            {
                SendMessageW(hCombo,
                             CB_SETITEMDATA,
                             (WPARAM)i,
                             Month);

                if (Month == (UINT)LocalDate.wMonth)
                {
                    SendMessageW(hCombo,
                                 CB_SETCURSEL,
                                 (WPARAM)i,
                                 0);
                }
            }
        }
    }
}


static WORD
GetCBSelectedMonth(HWND hCombo)
{
    INT i;
    WORD Ret = (WORD)-1;

    i = (INT)SendMessageW(hCombo,
                          CB_GETCURSEL,
                          0,
                          0);
    if (i != CB_ERR)
    {
        i = (INT)SendMessageW(hCombo,
                              CB_GETITEMDATA,
                              (WPARAM)i,
                              0);

        if (i >= 1 && i <= 13)
            Ret = (WORD)i;
    }

    return Ret;
}


static VOID
ChangeMonthCalDate(HWND hMonthCal,
                   WORD Day,
                   WORD Month,
                   WORD Year)
{
    SendMessageW(hMonthCal,
                 MCCM_SETDATE,
                 MAKEWPARAM(Day,
                            Month),
                 MAKELPARAM(Year,
                            0));
}

static VOID
AutoUpdateMonthCal(HWND hwndDlg,
                   PNMMCCAUTOUPDATE lpAutoUpdate)
{
    UNREFERENCED_PARAMETER(lpAutoUpdate);

    /* Update the controls */
    FillMonthsComboBox(GetDlgItem(hwndDlg,
                                  IDC_MONTHCB));
}


static INT_PTR CALLBACK
DTPProc(HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            /* Stop the timer when the user is about to change the time */
            if ((wParam != VK_LEFT) & (wParam != VK_RIGHT))
                KillTimer(GetParent(hwnd), ID_TIMER);
            break;
    }

    return CallWindowProcW(pOldWndProc, hwnd, uMsg, wParam, lParam);
}

/* Property page dialog callback */
INT_PTR CALLBACK
DateTimePageProc(HWND hwndDlg,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    if (!fDateDirty)
        SetTimes(&st);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            int cc;
            
            for (cc=0;cc<6;cc++)
                wPendingDateTime[cc] = -1;
            FillMonthsComboBox(GetDlgItem(hwndDlg,
                                          IDC_MONTHCB));

            /* Set range and current year */
            SendMessageW(GetDlgItem(hwndDlg, IDC_YEAR), UDM_SETRANGE, 0, MAKELONG ((short) 9999, (short) 1900));
            SendMessageW(GetDlgItem(hwndDlg, IDC_YEAR), UDM_SETPOS, 0, MAKELONG( (short) st.wYear, 0));

            pOldWndProc = (WNDPROC)SetWindowLongPtrW(GetDlgItem(hwndDlg, IDC_TIMEPICKER), GWLP_WNDPROC, (LONG_PTR)DTPProc);

            SetTimer(hwndDlg, ID_TIMER, 450, 0L);
            break;

        case WM_TIMER:
            SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPICKER), DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);

            // Reset timeout.
            SetTimer(hwndDlg, ID_TIMER, 450, 0L);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MONTHCB:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeMonthCalDate(GetDlgItem(hwndDlg,
                                                      IDC_MONTHCALENDAR),
                                                      (WORD) -1,
                                                      GetCBSelectedMonth((HWND)lParam),
                                                      (WORD) -1);
                        wPendingDateTime[0] = wDateTime[0];
                        wPendingDateTime[1] = wDateTime[1];
                        wPendingDateTime[2] = wDateTime[2];
                        wPendingDateTime[3] = (GetCBSelectedMonth((HWND)lParam) + 1);
                        wPendingDateTime[4] = wDateTime[4];
                        wPendingDateTime[5] = wDateTime[5];
                        wPendingDateTime[6] = wDateTime[6];
                        fDateDirty = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_CTLCOLORSTATIC:
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_YEARTEXT))
                return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            LPNMDATETIMECHANGE lpDteChange = (LPNMDATETIMECHANGE)lParam;

            switch (lpnm->idFrom)
            {
                case IDC_YEAR:
                    switch (lpnm->code)
                    {
                        case UDN_DELTAPOS:
                        {
                            SHORT wYear;
                            LPNMUPDOWN updown = (LPNMUPDOWN)lpnm;
                            wYear = (SHORT)SendMessageW(GetDlgItem(hwndDlg, IDC_YEAR), UDM_GETPOS, 0, 0);
                            
                            wPendingDateTime[0] = wDateTime[0];
                            wPendingDateTime[1] = wDateTime[1];
                            wPendingDateTime[2] = wDateTime[2];
                            wPendingDateTime[3] = wDateTime[3];
                            wPendingDateTime[4] = wDateTime[4];
                            wPendingDateTime[5] = wYear;
                            wPendingDateTime[6] = wDateTime[6];
                            if ( wDateTime[6] != wPendingDateTime[6] )
                                wDateTime[6] = wPendingDateTime[6];
                            /* Enable the 'Apply' button */
                            fDateDirty = TRUE;
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            ChangeMonthCalDate(GetDlgItem(hwndDlg,
                                               IDC_MONTHCALENDAR),
                                               (WORD) -1,
                                               (WORD) -1,
                                               (WORD) (wYear + updown->iDelta));
                        }
                        break;
                    }
                    break;

                case IDC_TIMEPICKER:
                    switch (lpnm->code)
                    {
                        case DTN_DATETIMECHANGE:
                            /* Stop the timer */
                            KillTimer(hwndDlg, ID_TIMER);

                            /* Tell the clock to stop ticking */
                            SendDlgItemMessageW(hwndDlg, IDC_CLOCKWND, CLM_STOPCLOCK,
                                                0, 0);

                            // Set the clock to the input time.
                            if (lpDteChange->dwFlags == GDT_VALID)
                            {
                                SYSTEMTIME datePart = {0};
                                int cc;
                                
                                SendMessageW(GetDlgItem(hwndDlg, IDC_MONTHCALENDAR),
                                             MCCM_GETDATE,
                                             (WPARAM)&datePart,
                                             0);
                                datePart.wSecond = lpDteChange->st.wSecond;
                                datePart.wMinute = lpDteChange->st.wMinute;
                                datePart.wHour   = lpDteChange->st.wHour;
                                
                                wPendingDateTime[0] = datePart.wSecond;
                                wPendingDateTime[1] = datePart.wMinute;
                                wPendingDateTime[2] = datePart.wHour;
                                wPendingDateTime[3] = datePart.wMonth;
                                wPendingDateTime[4] = datePart.wDay;
                                wPendingDateTime[5] = datePart.wYear;
                                wPendingDateTime[6] = datePart.wDayOfWeek;
                                
                                for (cc=0;cc<7;cc++)
                                {
                                    if (wDateTime[cc] != wPendingDateTime[cc])
                                        wDateTime[cc] = wPendingDateTime[cc];
                                }
                                
                                SendMessageW(GetDlgItem(hwndDlg, IDC_CLOCKWND), CLM_UPDATECLOCK,
                                             0, 
                                             0);
                            }

                            /* Enable the 'Apply' button */
                            fDateDirty = TRUE;
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                    }
                    break;

                case IDC_MONTHCALENDAR:
                    switch (lpnm->code)
                    {
                        case MCCN_SELCHANGE:
                            /* Enable the 'Apply' button */
                            fDateDirty = TRUE;
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;

                        case MCCN_AUTOUPDATE:
                            AutoUpdateMonthCal(hwndDlg,
                                                 (PNMMCCAUTOUPDATE)lpnm);
                            break;
                    }
                    break;

                default:
                    switch (lpnm->code)
                    {
                        case PSN_SETACTIVE:
                            SetTimeZoneName(hwndDlg);
                            break;

                        case PSN_APPLY:
                            SetLocalSystemTime(hwndDlg);
                            fDateDirty = FALSE;
                            /* Tell the clock to start ticking */
                            SendDlgItemMessageW(hwndDlg, IDC_CLOCKWND, CLM_STARTCLOCK,
                                                0, 0);

                            SetTimer(hwndDlg, ID_TIMER, 450, 0L);
                            FinishedSettingDates();
                            return TRUE;
                    }
                    break;
            }
        }
        break;

        case WM_TIMECHANGE:
            /* FIXME: We don't get this message as we're not a top-level window... */
            SendMessageW(GetDlgItem(hwndDlg,
                                    IDC_MONTHCALENDAR),
                         MCCM_RESET,
                         0,
                         0);
            if (fDateDirty)
            {
                FinishedSettingDates();
                fDateDirty = FALSE;
            }
            break;

        case WM_DESTROY:
            KillTimer(hwndDlg, ID_TIMER);
            break;
    }

    return FALSE;
}
