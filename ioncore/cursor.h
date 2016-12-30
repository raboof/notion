/*
 * ion/ioncore/cursor.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_CURSOR_H
#define ION_IONCORE_CURSOR_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#define IONCORE_CURSOR_DEFAULT 0
#define IONCORE_CURSOR_RESIZE  1
#define IONCORE_CURSOR_MOVE    2
#define IONCORE_CURSOR_DRAG    3
#define IONCORE_CURSOR_WAITKEY 4
#define IONCORE_N_CURSORS      5

extern void ioncore_init_cursors();
extern Cursor ioncore_xcursor(int cursor);

#endif /* ION_IONCORE_CURSOR_H */
