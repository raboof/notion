/*
 * config.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_CONFIG_H
#define ION_CONFIG_H

/* #define CF_NO_LOCK_HACK */

#define CF_FALLBACK_FONT_NAME "fixed"
#define CF_DRAG_TRESHOLD 2
#define CF_DBLCLICK_DELAY 250

#define CF_MAX_MOVERES_STR_SIZE 32
#define CF_MOVERES_WIN_X 5
#define CF_MOVERES_WIN_Y 5

#define CF_TAB_TEXT_ALIGN ALIGN_CENTER
#define CF_RESIZE_DELAY 1500

#define CF_XMESSAGE "xmessage -file "

#define CF_MIN_WIDTH 20
#define CF_MIN_HEIGHT 3
#define CF_STUBBORN_TRESH 1

/*#define CF_EDGE_RESISTANCE 16*/

/* Cursors
 */

#define CF_CURSOR_DEFAULT XC_left_ptr
#define CF_CURSOR_RESIZE XC_sizing
#define CF_CURSOR_MOVE XC_fleur
#define CF_CURSOR_DRAG XC_cross
#define CF_CURSOR_WAITKEY XC_icon

#endif /* ION_CONFIG_H */
