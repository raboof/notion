/*
 * wmcore/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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

#include "../config.h"

#ifdef CF_XFT

#include <X11/Xft/Xft.h>
typedef XftFont* WFontPtr;
typedef XftColor WColor;
typedef XftDraw WExtraDrawInfo;
#define set_foreground(dpy, gc, fg) XSetForeground((dpy), (gc), (fg).pixel)
#define set_background(dpy, gc, bg) XSetBackground((dpy), (gc), (bg).pixel)
#define COLOR_PIXEL(p) ((p).pixel)

#else /* !CF_XFT */

#ifdef CF_UTF8

typedef XFontSet WFontPtr;

#else /* !CF_UTF8 */

typedef XFontStruct* WFontPtr;

#endif /* !CF_UTF8 */

typedef unsigned long WColor;
typedef void *WExtraDrawInfo;
#define set_foreground XSetForeground
#define set_background XSetBackground
#define COLOR_PIXEL(p) (p)

#endif /* !CF_XFT */

typedef struct WRectangle_struct{
	int x, y;
	int w, h;
} WRectangle;

#include "obj.h"

extern void pgeom(const char *n, WRectangle g);

#if 0
#define D(X) X
#else
#define D(X)
#endif

#endif /* WMCORE_COMMON_H */
