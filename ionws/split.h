/*
 * ion/ionws/split.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_SPLIT_H
#define ION_IONWS_SPLIT_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/reginfo.h>
#include <ioncore/attach.h>

INTROBJ(WWsSplit);


enum WSplitDir{
	HORIZONTAL,
	VERTICAL
};


enum PrimaryNode{
	ANY,
	TOP_OR_LEFT,
	BOTTOM_OR_RIGHT
};


DECLOBJ(WWsSplit){
	WObj obj;
	int dir;
	WRectangle geom;
	int current;
	WObj *tl, *br;
	WWsSplit *parent;
	uint max_w, min_w, max_h, min_h;
};


extern WWsSplit *create_split(int dir, WObj *tl, WObj *br, 
							  const WRectangle *geom);
extern int split_tree_do_calcresize(WObj *node_, int dir, int primn, 
									int nsize);
extern void split_tree_resize(WObj *node, int dir, int primn,
							  int npos, int nsize);
extern void split_tree_do_resize(WObj *node, int dir, int primn,
								 int npos, int nsize);

extern int split_tree_size(WObj *obj, int dir);
extern int split_tree_pos(WObj *obj, int dir);
extern int split_tree_other_size(WObj *obj, int dir);

#include "ionws.h"

extern void ionws_add_managed(WIonWS *ws, WRegion *reg);
extern void ionws_managed_activated(WIonWS *ws, WRegion *reg);
extern WRegion *ionws_find_rescue_manager_for(WIonWS *ws, WRegion *reg);
extern void ionws_request_managed_geom(WIonWS *ws, WRegion *reg,
									   int flags, const WRectangle *geom,
									   WRectangle *geomret);
extern void ionws_remove_managed(WIonWS *ws, WRegion *reg);

extern WRegion *ionws_current(WIonWS *ws);
extern WRegion *ionws_above(WIonWS *ws, WRegion *reg);
extern WRegion *ionws_below(WIonWS *ws, WRegion *reg);
extern WRegion *ionws_left_of(WIonWS *ws, WRegion *reg);
extern WRegion *ionws_right_of(WIonWS *ws, WRegion *reg);
extern WRegion *ionws_topmost(WIonWS *ws);
extern WRegion *ionws_lowest(WIonWS *ws);
extern WRegion *ionws_rightmost(WIonWS *ws);
extern WRegion *ionws_leftmost(WIonWS *ws);
extern void ionws_goto_above(WIonWS *ws);
extern void ionws_goto_below(WIonWS *ws);
extern void ionws_goto_left(WIonWS *ws);
extern void ionws_goto_right(WIonWS *ws);

extern WRegion *split_reg(WRegion *reg, int dir, int primn,
						  int minsize, WRegionSimpleCreateFn *fn);
extern WRegion *split_toplevel(WIonWS *ws, int dir, int primn,
							   int minsize, WRegionSimpleCreateFn *fn);

#endif /* ION_IONWS_SPLIT_H */

