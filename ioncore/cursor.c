/*
 * ion/ioncore/cursor.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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


