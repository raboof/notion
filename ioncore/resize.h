/*
 * ion/ioncore/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_RESIZE_H
#define ION_IONCORE_RESIZE_H

#include "common.h"
#include "genframe.h"

typedef void WDrawRubberbandFn(WRootWin *rw, WRectangle geom);

extern bool begin_resize(WRegion *reg, WDrawRubberbandFn *rubfn,
						 bool cumulative);
extern bool begin_resize_atexit(WRegion *reg, WDrawRubberbandFn *rubfn, 
								bool cumulative, void (*exitfn)());
extern bool begin_move(WRegion *reg, WDrawRubberbandFn *rubfn,
					   bool cumulative);
/* dx1/dx2/dy1/dy2: left/right/top/bottom difference to previous values. 
 * left/top negative, bottom/right positive increases size.
 */
extern void delta_resize(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
						 WRectangle *rret);
extern void delta_move(WRegion *reg, int dx, int dy, WRectangle *rret);
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
extern void region_set_w(WRegion *reg, int w);
extern void region_set_h(WRegion *reg, int h);

extern void genframe_resize_units(WGenFrame *genframe, int *wret, int *hret);

extern void genframe_maximize_vert(WGenFrame *frame);
extern void genframe_maximize_horiz(WGenFrame *frame);
extern void genframe_do_toggle_shade(WGenFrame *frame, int shaded_h);
extern bool genframe_is_shaded(WGenFrame *frame);

#endif /* ION_IONCORE_RESIZE_H */
