/*
 * wmcore/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_RESIZE_H
#define WMCORE_RESIZE_H

#include "common.h"
#include "screen.h"

typedef void WDrawRubberbandFn(WScreen *scr, WRectangle geom);

extern bool begin_resize(WRegion *reg, WDrawRubberbandFn *rubfn);
extern bool begin_resize_atexit(WRegion *reg, WDrawRubberbandFn *rubfn, void (*exitfn)());
/* dx1/dx2/dy1/dy2: left/right/top/bottom difference to previous values. 
 * left/top negative, bottom/right positive increases size.
 */
extern void delta_resize(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
						 WRectangle *rret);
extern void end_resize(WRegion *reg);
extern void cancel_resize(WRegion *reg);
extern void set_resize_timer(WRegion *reg, uint timeout);

extern bool is_resizing();
extern bool may_resize(WRegion *reg);

DYNFUN void region_resize_hints(WRegion *reg, XSizeHints *hints_ret,
								uint *relw_ret, uint *relh_ret);

extern void region_request_geom(WRegion *reg,
								WRectangle geom, WRectangle *geomret,
								bool tryonly);

extern uint region_min_h(WRegion *reg);
extern uint region_min_w(WRegion *reg);

extern void set_width(WRegion *reg, uint w);
extern void set_height(WRegion *reg, uint h);
extern void set_widthq(WRegion *reg, double q);
extern void set_heightq(WRegion *reg, double q);

#endif /* WMCORE_RESIZE_H */
