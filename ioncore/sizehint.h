/*
 * ion/ioncore/sizehint.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SIZEHINT_H
#define ION_IONCORE_SIZEHINT_H

#include "common.h"
#include "region.h"

extern void xsizehints_sanity_adjust(XSizeHints *hints);
extern void xsizehints_adjust_for(XSizeHints *hints, WRegion *list);
extern void xsizehints_correct(const XSizeHints *hints, int *wp, int *hp, 
                               bool min);

extern int xgravity_deltax(int gravity, int left, int right);
extern int xgravity_deltay(int gravity, int top, int bottom);

#endif /* ION_IONCORE_SIZEHINT_H */
