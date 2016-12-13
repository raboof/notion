/*
 * ion/ioncore/cursor.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "event.h"
#include "cursor.h"
#include "global.h"


static Cursor cursors[IONCORE_N_CURSORS];

#define LCURS(TYPE) \
    cursors[IONCORE_CURSOR_##TYPE]=XCreateFontCursor(ioncore_g.dpy, CF_CURSOR_##TYPE)

void ioncore_init_cursors()
{
    LCURS(DEFAULT);
    LCURS(RESIZE);
    LCURS(MOVE);
    LCURS(DRAG);
    LCURS(WAITKEY);
}


Cursor ioncore_xcursor(int cursor)
{
    return cursors[cursor];
}


