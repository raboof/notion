/*
 * ion/ioncore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_ATTACH_H
#define ION_IONCORE_ATTACH_H

#include "region.h"
#include "reginfo.h"
#include "screen.h"
#include "window.h"

#define REGION_ATTACH_SWITCHTO	0x0001

typedef WRegion *WRegionAddFn(WWindow *parent, WRectangle geom, void *param);

DYNFUN WRegion *region_do_add_managed(WRegion *reg, WRegionAddFn *fn,
									  void *param, int flags,
									  WRectangle *geomrq);

extern bool region_supports_add_managed(WRegion *reg);

extern WRegion *region_add_managed_new_simple(WRegion *reg,
											  WRegionSimpleCreateFn *fn,
											  int flags);
extern bool region_add_managed(WRegion *reg, WRegion *sub, int flags);

/* */

DYNFUN WRegion *region_do_find_new_manager(WRegion *reg);
extern WRegion *default_do_find_new_manager(WRegion *reg);
extern WRegion *region_find_new_manager(WRegion *reg);

extern bool region_move_managed_on_list(WRegion *dest, WRegion *src,
										WRegion *list);
extern bool region_rescue_managed_on_list(WRegion *reg, WRegion *list);

#endif /* ION_IONCORE_ATTACH_H */
