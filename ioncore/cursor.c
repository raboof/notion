/*
 * ion/ioncore/cursor.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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


void change_grab_cursor(int cursor)
{
	XChangeActivePointerGrab(wglobal.dpy, GRAB_POINTER_MASK,
							 cursors[cursor], CurrentTime);
}


void set_cursor(Window win, int cursor)
{
	XDefineCursor(wglobal.dpy, win, cursors[cursor]);
}


Cursor x_cursor(int cursor)
{
	return cursors[cursor];
}
