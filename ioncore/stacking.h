/*
 * ion/ioncore/stacking.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_STACKING_H
#define ION_IONCORE_STACKING_H

#include <libtu/obj.h>
#include "region.h"
#include "window.h"

/* Functions to set up stacking management */
extern bool region_stack_above(WRegion *reg, WRegion *above);
extern void region_keep_on_top(WRegion *reg);
extern void region_reset_stacking(WRegion *reg);
extern void window_init_sibling_stacking(WWindow *parent, Window win);

/* Functions to restack */
extern void region_raise(WRegion *reg);
extern void region_lower(WRegion *reg);

DYNFUN Window region_restack(WRegion *reg, Window other, int mode);

#endif /* ION_IONCORE_STACKING_H */
