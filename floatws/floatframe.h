/*
 * ion/floatws/floatframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_FLOATWS_FLOATFRAME_H
#define ION_FLOATWS_FLOATFRAME_H

#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/window.h>
#include <ioncore/extl.h>

INTROBJ(WFloatFrame);

DECLOBJ(WFloatFrame){
	WGenFrame genframe;
	int bar_w;
};


extern WFloatFrame *create_floatframe(WWindow *parent, WRectangle geom,
									  int flags);

extern void floatframe_remove_managed(WFloatFrame *frame, WRegion *reg);

extern WRectangle initial_to_floatframe_geom(WGRData *grdata, WRectangle geom,
											 int gravity);

extern WRegion *floatframe_load(WWindow *par, WRectangle geom, ExtlTab tab);

extern void floatframe_p_move(WFloatFrame *frame);
extern void floatframe_toggle_shade(WFloatFrame *frame);

#endif /* ION_FLOATWS_FLOATFRAME_H */
