/*
 * config.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_CONFIG_H
#define INCLUDED_CONFIG_H

#ifndef ETCDIR
#define ETCDIR "/etc/"
#endif

/* #define CF_MODULE_SUPPORT */
/* #define CF_NO_LOCK_HACK */
#define CF_SWITCH_NEW_CLIENTS


/* Configurable
 */

#define CF_FALLBACK_FONT_NAME "fixed"
#define CF_DRAG_TRESHOLD 2
#define CF_DBLCLICK_DELAY 250

#define CF_MAX_MOVERES_STR_SIZE 32
#define CF_MOVERES_WIN_X 5
#define CF_MOVERES_WIN_Y 5

#define CF_TAB_TEXT_ALIGN ALIGN_CENTER
#define CF_RESIZE_DELAY 1500

/* Cursors
 */

#define CF_CURSOR_DEFAULT XC_left_ptr
#define CF_CURSOR_RESIZE XC_sizing
#define CF_CURSOR_MOVE XC_fleur
#define CF_CURSOR_DRAG XC_cross
#define CF_CURSOR_WAITKEY XC_icon


#define CF_MAX_COMMAND_SQ_NEST 32

/* PWM
 */
#define CF_EDGE_RESISTANCE 16
#define CF_STEP_SIZE 16
#define CF_CORNER_SIZE (16+8)


/* Ion
 */
#define CF_MIN_WIDTH 20
#define CF_MIN_HEIGHT 3
#define CF_STUBBORN_TRESH 1

#endif /* INCLUDED_CONFIG_H */
