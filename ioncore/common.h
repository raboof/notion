/*
 * wmcore/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_COMMON_H
#define WMCORE_COMMON_H

#include <libtu/types.h>
#include <libtu/output.h>
#include <libtu/misc.h>
#include <libtu/dlist.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef CF_XFT
#include <X11/Xft/Xft.h>
typedef XftFont WFont;
typedef XftColor WColor;
typedef XftDraw WExtraDrawInfo;
#define set_foreground(dpy, gc, fg) XSetForeground((dpy), (gc), (fg).pixel)
#define set_background(dpy, gc, bg) XSetBackground((dpy), (gc), (bg).pixel)
#define COLOR_PIXEL(p) ((p).pixel)
#else
typedef XFontStruct WFont;
typedef unsigned long WColor;
typedef void *WExtraDrawInfo;
#define set_foreground XSetForeground
#define set_background XSetBackground
#define COLOR_PIXEL(p) (p)
#endif

typedef struct WRectangle_struct{
	int x, y;
	int w, h;
} WRectangle;

#include "obj.h"
#include "../config.h"

extern void pgeom(const char *n, WRectangle g);

#endif /* WMCORE_COMMON_H */
