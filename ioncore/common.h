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

typedef struct WRectangle_struct{
	int x, y;
	int w, h;
} WRectangle;

typedef ulong Pixel;

#include "obj.h"
#include "../config.h"

extern void pgeom(const char *n, WRectangle g);

#endif /* WMCORE_COMMON_H */
