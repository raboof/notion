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
#define EXTL_EXPORT_MEMBER
#define EXTL_EXPORT_MEMBER_AS(X, Y)

#endif /* ION_IONCORE_COMMON_H */
