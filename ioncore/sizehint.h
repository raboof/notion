/*
 * ion/ioncore/sizehint.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_SIZEHINT_H
#define ION_IONCORE_SIZEHINT_H

#include "common.h"
#include "region.h"
#include "screen.h"

extern void correct_size(int *wp, int *hp, const XSizeHints *hints, bool min);
extern void get_sizehints(WScreen *scr, Window win, XSizeHints *hints);
extern void adjust_size_hints_for_managed(XSizeHints *hints, WRegion *list);

#endif /* ION_IONCORE_SIZEHINT_H */
