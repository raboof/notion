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

typedef WRegion *WRegionCreateFn(WScreen *scr, WWinGeomParams params,
								 void *fnp);
								 
DYNFUN void region_do_attach_params(const WRegion *reg, WWinGeomParams *params);
DYNFUN void region_do_attach(WRegion *reg, WRegion *sub, int flags);

extern bool region_supports_attach(WRegion *reg);
extern WRegion *region_attach_new(WRegion *reg, WRegionCreateFn *fn, void
								  *fnp, int flags);
extern bool region_attach_sub(WRegion *reg, WRegion *sub, int flags);

DYNFUN WRegion *region_do_find_new_home(WRegion *reg);
extern WRegion *default_do_find_new_home(WRegion *reg);
extern WRegion *region_find_new_home(WRegion *reg);

extern bool region_move_subregions(WRegion *dest, WRegion *src);
extern bool region_move_clientwins(WRegion *dest, WRegion *src);
extern bool region_rescue_subregions(WRegion *reg);
extern bool region_rescue_clientwins(WRegion *reg);

#endif /* WMCORE_ATTACH_H */
