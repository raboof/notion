/*
 * wmcore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_ATTACH_H
#define WMCORE_ATTACH_H

#include "region.h"
#include "screen.h"

#define REGION_ATTACH_SWITCHTO	0x0001

typedef WRegion *WRegionCreateFn(WRegion *parent, WRectangle geom, void *fnp);
								 
DYNFUN void region_add_managed_params(const WRegion *reg, WRegion **par,
									  WRectangle *geom);
DYNFUN void region_add_managed_doit(WRegion *reg, WRegion *sub, int flags);

extern bool region_supports_add_managed(WRegion *reg);
extern WRegion *region_add_managed_new(WRegion *reg, WRegionCreateFn *fn,
									   void *fnp, int flags);
extern bool region_add_managed(WRegion *reg, WRegion *sub, int flags);

/* */

DYNFUN WRegion *region_do_find_new_manager(WRegion *reg);
extern WRegion *default_do_find_new_manager(WRegion *reg);
extern WRegion *region_find_new_manager(WRegion *reg);

extern bool region_move_managed_on_list(WRegion *dest, WRegion *src,
										WRegion *list);
extern bool region_rescue_managed_on_list(WRegion *reg, WRegion *list);

#endif /* WMCORE_ATTACH_H */
