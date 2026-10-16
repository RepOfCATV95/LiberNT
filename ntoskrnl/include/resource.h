/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation - Standard Bitmap Resources
 * COPYRIGHT:   Copyright 2010 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2015-2024 Hermès Bélusca-Maïto
 *              Copyright 1997-2026 Republic of CATV 95
 */

#pragma once

#define IDB_BOOT_SCREEN                         1
#define IDB_HIBERNATE_BAR                       2
#define IDB_SHUTDOWN_MSG                        3
#define IDB_BOOT_SCREEN_SERVER                  4
#define IDB_SHUTDOWN_LOGO_PRO                   5
#define IDB_SHUTDOWN_LOGO_SERVER                6
#define IDB_PROFESSIONAL_HEADER                 7
#define IDB_SERVER_HEADER                       8
#define IDB_BOOT_SCREEN_POWERED                 9
#ifdef BT2_BOOT
#define IDB_BOOT_SCREEN_NATIONAL_GEOGRAPHIC    10
#define IDB_BAR_DEFAULT                        11
#define IDB_REACTOS_GOP_LOGO                   12
#define IDB_MAX_RESOURCES           IDB_REACTOS_GOP_LOGO
#else
#define IDB_MAX_RESOURCES           IDB_BOOT_SCREEN_POWERED
#endif
