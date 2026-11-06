/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation header
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 *              Copyright 1997-2028 Republic of CATV 95 (catv95republic@proton.me)
 */

#pragma once

#define VID_PROGRESS_BAR_GEN_WIDTH          162
#define VID_PROGRESS_BAR_GEN_HEIGHT         7
#define VID_PROGRESS_BAR_EXTRA_SIZE         18
#define VID_PROGRESS_BAR_EXTRA_WIDTH        9

// ReactOS 0.2.x-specific
#define VID_PROGRESS_BAR_ROS_WIDTH          366
#define VID_PROGRESS_BAR_ROS_HEIGHT         19
#define VID_PROGRESS_BAR_ROS_SIZE           (VID_PROGRESS_BAR_ROS_HEIGHT+347)

#define VID_FOOTER_BG_TOP                   414

//
// Hibernation-related
//
#define HIBER_PROGRESS_BAR_WIDTH            303
#define HIBER_PROGRESS_BAR_HEIGHT           282
