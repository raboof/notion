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

extern void correct_size(int *wp, int *hp, const XSizeHints *hints, bool min);
extern void get_sizehints(Window win, XSizeHints *hints);
extern void adjust_size_hints_for_managed(XSizeHints *hints, WRegion *list);
extern int gravity_deltax(int gravity, int left, int right);
extern int gravity_deltay(int gravity, int top, int bottom);

#endif /* ION_IONCORE_SIZEHINT_H */
