/*
 * ion/ioncore/common.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_COMMON_H
#define ION_IONCORE_COMMON_H

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

#if 0
#define D(X) X
#else
#define D(X)
#endif

#define EXTL_EXPORT
#define EXTL_EXPORT_AS(X)

#endif /* ION_IONCORE_COMMON_H */
