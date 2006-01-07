/*
 * config.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_CONFIG_H
#define ION_CONFIG_H

/* #define CF_NO_LOCK_HACK */

#define CF_FALLBACK_FONT_NAME "fixed"
/*#define CF_FALLBACK_FONT_NAME "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*"*/
#define CF_DRAG_TRESHOLD 2
#define CF_DBLCLICK_DELAY 250

#define CF_MAX_MOVERES_STR_SIZE 32

#define CF_TAB_TEXT_ALIGN ALIGN_CENTER
#define CF_RESIZE_DELAY 1500

#define CF_XMESSAGE "xmessage -file "

#define CF_EDGE_RESISTANCE 16

#define CF_RAISE_DELAY 500

#define CF_STATUSBAR_SYSTRAY_HEIGHT 24

/* Cursors
 */

#define CF_CURSOR_DEFAULT XC_left_ptr
#define CF_CURSOR_RESIZE XC_sizing
#define CF_CURSOR_MOVE XC_fleur
#define CF_CURSOR_DRAG XC_cross
#define CF_CURSOR_WAITKEY XC_icon

#define CF_STR_EMPTY "<empty frame>"
#define CF_STR_EMPTY_LEN 13

#define CF_STDISP_MIN_SZ 8

#endif /* ION_CONFIG_H */
