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

INTROBJ(WFloatFrame)

DECLOBJ(WFloatFrame){
	WGenFrame genframe;
	int bar_w;
};


extern WFloatFrame *create_floatframe(WWindow *parent, WRectangle geom, int id);

extern void floatframe_remove_managed(WFloatFrame *frame, WRegion *reg);

extern WRectangle sub_to_floatframe_geom(WGRData *grdata, WRectangle geom);

/*extern WRegion *ionframe_load(WRegion *par, WRectangle geom, Tokenizer *tokz);*/

extern void floatframe_raise(WFloatFrame *frame);
extern void floatframe_lower(WFloatFrame *frame);

#endif /* ION_FLOATWS_FLOATFRAME_H */
