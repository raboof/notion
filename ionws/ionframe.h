/*
 * ion/ionws/ionframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_IONFRAME_H
#define ION_IONWS_IONFRAME_H

#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRCLASS(WIonFrame);

DECLCLASS(WIonFrame){
    WFrame frame;
};

extern WIonFrame* create_ionframe(WWindow *parent, const WRectangle *geom);
extern void ionframe_draw_config_updated(WIonFrame *frame);
extern void ionframe_toggle_shade(WIonFrame *frame);
extern WRegion *ionframe_load(WWindow *par, const WRectangle *geom, 
                              ExtlTab tab);

extern void ionframe_close(WIonFrame *frame);
extern void ionframe_close_if_empty(WIonFrame *frame);
extern WIonFrame *ionframe_split(WIonFrame *frame, const char *dirstr);
extern WIonFrame *ionframe_split_empty(WIonFrame *frame, const char *dirstr);

#endif /* ION_IONWS_IONFRAME_H */
