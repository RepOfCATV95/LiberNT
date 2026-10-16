/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation header
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 *              Copyright 1997-2028 Republic of CATV 95 (catv95republic@proton.me)
 */

#pragma once

//
// Positions of areas and images
//

#define VID_SCROLL_AREA_LEFT                  8
#define VID_SCROLL_AREA_TOP                 124
#define VID_SCROLL_AREA_RIGHT               631
#define VID_SCROLL_AREA_BOTTOM              396

#define VID_BOTTOM_BOX_WIDTH                637
#define VID_BOTTOM_BOX_HEIGHT               477

#define VID_PROGRESS_BAR_NORMAL_LEFT        274
#define VID_PROGRESS_BAR_NORMAL_TOP         437
#define VID_PROGRESS_BAR_TEXT_LEFT          239
#define VID_PROGRESS_BAR_TEXT_TOP           445

#define VID_PROGRESS_BAR_NORMAL_ROS_LEFT    165
#define VID_PROGRESS_BAR_NORMAL_ROS_TOP     446

#define ROTATION_BAR_GEN_HALFER             320

#define ROTATION_BAR_NORMAL_WIDTH           640
#define ROTATION_BAR_NORMAL_HEIGHT          416
#define ROTATION_BAR_NORMAL_OFFSET          0
#define ROTATION_BAR_NORMAL_SIZE            10

#define ROTATION_BAR_NORMAL_ROS_WIDTH       634
#define ROTATION_BAR_NORMAL_ROS_HEIGHT      480
#define ROTATION_BAR_NORMAL_ROS_OFFSET      0
#define ROTATION_BAR_NORMAL_ROS_SIZE        6

#define ROTATION_BAR_TEXTMODE_WIDTH         636
#define ROTATION_BAR_TEXTMODE_HEIGHT        114
#define ROTATION_BAR_TEXTMODE_OFFSET        2
#define ROTATION_BAR_TEXTMODE_SIZE          6

/* 16px space between shutdown logo and message */
/* Use 156x186x153x166 for displaying Windows 2000-styled safe to shutdown screens (for both client & server) */
#define VID_SHUTDOWN_LOGO_LEFT              156
#define VID_SHUTDOWN_LOGO_TOP               186
#define VID_SHUTDOWN_MSG_LEFT               153
#define VID_SHUTDOWN_MSG_TOP                166
