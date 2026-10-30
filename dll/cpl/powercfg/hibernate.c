/*
 * PROJECT:     ReactOS Power Configuration Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hibernate tab
 * COPYRIGHT:   Copyright 2006 Alexander Wurzinger <lohnegrim@gmx.net>
 *              Copyright 2006 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2006 Martin Rottensteiner <2005only@pianonote.at>
 */

#include "powercfg.h"

static DWORD
Hib_ReadPowerDword(LPCTSTR ValueName, DWORD DefaultValue)
{
    HKEY hKey;
    DWORD Type = REG_DWORD;
    DWORD Value = DefaultValue;
    DWORD Size = sizeof(Value);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\Power"),
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, ValueName, NULL, &Type, (LPBYTE)&Value, &Size) != ERROR_SUCCESS ||
            Type != REG_DWORD || Size != sizeof(Value))
        {
            Value = DefaultValue;
        }
        RegCloseKey(hKey);
    }

    return Value;
}

static VOID
Hib_InitDialog(HWND hwndDlg)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    MEMORYSTATUSEX msex;
    TCHAR szTemp[MAX_PATH];
    LPTSTR lpRoot;
    ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
    BOOLEAN bHibernate;
    DWORD HibernateEnabled;
    DWORD HiberFileSizePercent;
    ULONGLONG RequiredHiberFileSize;

    HibernateEnabled = Hib_ReadPowerDword(TEXT("HibernateEnabled"), 1);
    HiberFileSizePercent = Hib_ReadPowerDword(TEXT("HiberFileSizePercent"), 40);
    if (HiberFileSizePercent == 0 || HiberFileSizePercent > 100)
        HiberFileSizePercent = 40;

    if (GetPwrCapabilities(&PowerCaps))
    {
        CheckDlgButton(hwndDlg,
                       IDC_HIBERNATEFILE,
                       PowerCaps.HiberFilePresent ? BST_CHECKED : BST_UNCHECKED);

        msex.dwLength = sizeof(msex);
        if (!GlobalMemoryStatusEx(&msex))
        {
            return; // FIXME
        }

        if (GetWindowsDirectory(szTemp, _countof(szTemp)))
            lpRoot = szTemp;
        else
            lpRoot = NULL;

        // Get available space and size of selected volume.
        if (!GetDiskFreeSpaceEx(lpRoot, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
            TotalNumberOfFreeBytes.QuadPart = 0;

        // Print the free available space into selected volume.
        StrFormatByteSize(TotalNumberOfFreeBytes.QuadPart, szTemp, _countof(szTemp));
        SetDlgItemText(hwndDlg, IDC_FREESPACE, szTemp);

        /* The kernel reserves HiberFileSizePercent of RAM, not necessarily all RAM. */
        RequiredHiberFileSize = (msex.ullTotalPhys / 100) * HiberFileSizePercent +
                                ((msex.ullTotalPhys % 100) * HiberFileSizePercent) / 100;
        RequiredHiberFileSize = (RequiredHiberFileSize + (1024 * 1024 - 1)) &
                                ~((ULONGLONG)(1024 * 1024 - 1));

        // Display the estimated hibernation-file size.
        StrFormatByteSize(RequiredHiberFileSize, szTemp, _countof(szTemp));
        SetDlgItemText(hwndDlg, IDC_SPACEFORHIBERNATEFILE, szTemp);
        if (TotalNumberOfFreeBytes.QuadPart < RequiredHiberFileSize && !PowerCaps.HiberFilePresent)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), SW_SHOW);
        }
        else
        {
            ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), SW_HIDE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), TRUE);
        }

        if (HibernateEnabled == 0)
        {
            CheckDlgButton(hwndDlg, IDC_HIBERNATEFILE, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), FALSE);
        }

        bHibernate = !!PowerCaps.HiberFilePresent;
        if (CallNtPowerInformation(SystemReserveHiberFile, &bHibernate, sizeof(bHibernate), NULL, 0) != STATUS_SUCCESS)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), FALSE);
        }
    }
}

INT_PTR
Hib_SaveData(HWND hwndDlg)
{
    BOOLEAN bHibernate;

    bHibernate = (IsDlgButtonChecked(hwndDlg, IDC_HIBERNATEFILE) == BST_CHECKED);
    if (CallNtPowerInformation(SystemReserveHiberFile, &bHibernate, sizeof(bHibernate), NULL, 0) == STATUS_SUCCESS)
    {
        Hib_InitDialog(hwndDlg);
        return TRUE;
    }

    return FALSE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
HibernateDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Hib_InitDialog(hwndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_HIBERNATEFILE:
                    if (HIWORD(wParam) == BN_CLICKED)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
                return Hib_SaveData(hwndDlg);
            break;
    }

    return FALSE;
}
