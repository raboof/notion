/*
 * ion/floatws/floatws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_FLOATWS_FLOATWS_H
#define ION_FLOATWS_FLOATWS_H

#include <ioncore/common.h>
#include <libtu/tokenizer.h>

#include <ioncore/region.h>
#include <ioncore/viewport.h>
#include <ioncore/genws.h>

INTROBJ(WFloatWS)

DECLOBJ(WFloatWS){
	WGenWS genws;
	Window dummywin;
	WRegion *managed_list;
	WRegion *current_managed;
};


extern WFloatWS *create_floatws(WWindow *parent, WRectangle bounds);

extern void floatws_circulate(WFloatWS *ws);
extern void floatws_backcirculate(WFloatWS *ws);

extern WRegion *floatws_load(WWindow *par, WRectangle geom, Tokenizer *tokz);

#endif /* ION_FLOATWS_FLOATWS_H */
