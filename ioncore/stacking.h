/*
 * ion/ioncore/stacking.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_STACKING_H
#define ION_IONCORE_STACKING_H

#include "obj.h"
#include "region.h"

/* Functions to set up stacking management */
extern void region_stack_above(WRegion *reg, WRegion *above);
extern void region_keep_on_top(WRegion *reg);
extern void region_reset_stacking(WRegion *reg);

/* Functions to restack */
extern void region_raise(WRegion *reg);
extern void region_lower(WRegion *reg);

DYNFUN Window region_restack(WRegion *reg, Window other, int mode);

/* Misc */
extern WRegion *region_topmost_stacked_above(WRegion *reg);

#endif /* ION_IONCORE_STACKING_H */
