/*
 * ion/ioncore/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_RESIZE_H
#define ION_IONCORE_RESIZE_H

#include "common.h"
#include "genframe.h"


/* To make it easier for region_request_managed_geom handlers, the geom
 * parameter contain a complete requested geometry for the region that
 * wants its geometry changed. The REGION_WEAK_* flags are used to
 * indicate that the respective geometry value has not been changed or
 * that the requestor doesn't really care what the result is. In any case,
 * managers are free to give the managed object whatever geometry it wishes.
 */
#define REGION_RQGEOM_WEAK_X	0x0001
#define REGION_RQGEOM_WEAK_Y	0x0002
#define REGION_RQGEOM_WEAK_W	0x0004
#define REGION_RQGEOM_WEAK_H	0x0008
#define REGION_RQGEOM_WEAK_ALL	0x000f
#define REGION_RQGEOM_TRYONLY	0x0010

#define REGION_RQGEOM_NORMAL	0
#define REGION_RQGEOM_VERT_ONLY	(REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_W)
#define REGION_RQGEOM_HORIZ_ONLY (REGION_RQGEOM_WEAK_Y|REGION_RQGEOM_WEAK_H)
#define REGION_RQGEOM_H_ONLY	(REGION_RQGEOM_VERT_ONLY|REGION_RQGEOM_WEAK_Y)
#define REGION_RQGEOM_W_ONLY	(REGION_RQGEOM_HORIZ_ONLY|REGION_RQGEOM_WEAK_X)

typedef void WDrawRubberbandFn(WRootWin *rw, const WRectangle *geom);

extern bool begin_resize(WRegion *reg, WDrawRubberbandFn *rubfn,
						 bool cumulative);
extern bool begin_move(WRegion *reg, WDrawRubberbandFn *rubfn,
					   bool cumulative);
/* dx1/dx2/dy1/dy2: left/right/top/bottom difference to previous values. 
 * left/top negative, bottom/right positive increases size.
 */
extern void delta_resize(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
						 WRectangle *rret);
extern void delta_move(WRegion *reg, int dx, int dy, WRectangle *rret);
extern bool end_resize();
extern bool cancel_resize();
extern bool is_resizing();
extern bool may_resize(WRegion *reg);

/* Note: even if REGION_RQGEOM_(X|Y|W|H) are not all specified, the
 * geom parameter should contain a proper geometry!
 */
DYNFUN void region_request_managed_geom(WRegion *reg, WRegion *sub,int flags, 
										const WRectangle *geom,
										WRectangle *geomret);

extern void region_request_geom(WRegion *reg, int flags, 
								const WRectangle *geom,
								WRectangle *geomret);

/* Implementation for regions that do not allow subregions to resize
 * themselves; default is to give the size the region wants.
 */
extern void region_request_managed_geom_unallow(WRegion *reg, WRegion *sub,
												int flags, 
												const WRectangle *geom,
												WRectangle *geomret);
/* default */
extern void region_request_managed_geom_allow(WRegion *reg, WRegion *sub,
											  int flags, 
											  const WRectangle *geom, 
											  WRectangle *geomret);

/* This function expects a root-relative geometry and the client expects
 * the gravity size hint be taken into account.
 */
DYNFUN void region_request_clientwin_geom(WRegion *reg, WClientWin *cwin,
										  int flags, const WRectangle *geom);

DYNFUN void region_resize_hints(WRegion *reg, XSizeHints *hints_ret,
								uint *relw_ret, uint *relh_ret);

extern uint region_min_h(WRegion *reg);
extern uint region_min_w(WRegion *reg);

extern void genframe_resize_units(WGenFrame *genframe, int *wret, int *hret);

extern void genframe_maximize_vert(WGenFrame *frame);
extern void genframe_maximize_horiz(WGenFrame *frame);

extern void genframe_do_toggle_shade(WGenFrame *frame, int shaded_h);
extern bool genframe_is_shaded(WGenFrame *frame);

extern void region_convert_root_geom(WRegion *reg, WRectangle *geom);

extern void resize_accel(int *wu, int *hu, int mode);

#endif /* ION_IONCORE_RESIZE_H */
