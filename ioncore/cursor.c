/*
 * ion/ioncore/cursor.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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


static Cursor cursors[N_CURSORS];

#define LCURS(TYPE) \
	cursors[CURSOR_##TYPE]=XCreateFontCursor(wglobal.dpy, CF_CURSOR_##TYPE)

void load_cursors()
{
	LCURS(DEFAULT);
	LCURS(RESIZE);
	LCURS(MOVE);
	LCURS(DRAG);
	LCURS(WAITKEY);
}


void set_cursor(Window win, int cursor)
{
	XDefineCursor(wglobal.dpy, win, cursors[cursor]);
}


Cursor x_cursor(int cursor)
{
	return cursors[cursor];
}
