/*
 * ion/ionws/ionframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_IONFRAME_H
#define ION_IONWS_IONFRAME_H

#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/extl.h>

INTROBJ(WIonFrame);

DECLOBJ(WIonFrame){
	WGenFrame genframe;
};


extern WIonFrame *create_ionframe(WWindow *parent, WRectangle geom, int flags);
extern WIonFrame* create_ionframe_simple(WWindow *parent, WRectangle geom);
extern void ionframe_draw_config_updated(WIonFrame *frame);
extern void ionframe_draw_bar(const WIonFrame *frame, bool complete);
extern void ionframe_draw(const WIonFrame *frame, bool complete);

extern WRegion *ionframe_load(WWindow *par, WRectangle geom, ExtlTab tab);

extern void ionframe_toggle_shade(WIonFrame *frame);

#endif /* ION_IONWS_IONFRAME_H */
