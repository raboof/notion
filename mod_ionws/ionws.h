/*
 * ion/ionws/ionws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_IONWS_H
#define ION_IONWS_IONWS_H

#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>
#include "split.h"
#include "ionframe.h"


INTRCLASS(WIonWS);
DECLCLASS(WIonWS){
    WGenWS genws;
    WSplit *split_tree;
    WRegion *managed_list;
    ExtlTab managed_splits;
};


extern WIonWS *create_ionws(WWindow *parent, const WFitParams *fp, bool ci);
extern WIonWS *create_ionws_simple(WWindow *parent, const WFitParams *fp);
extern WRegion *ionws_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

extern bool ionws_rescue_clientwins(WIonWS *ws);

extern void ionws_managed_rqgeom(WIonWS *ws, WRegion *reg,
                                 int flags, const WRectangle *geom,
                                 WRectangle *geomret);
extern void ionws_managed_remove(WIonWS *ws, WRegion *reg);
extern void ionws_managed_activated(WIonWS *ws, WRegion *reg);
extern bool ionws_managed_rescue(WIonWS *ws, WClientWin *cwin, WRegion *from);

extern ExtlTab ionws_resize_tree(WIonWS *ws, Obj *node, ExtlTab g);

extern WRegion *ionws_current(WIonWS *ws);
extern WRegion *ionws_next_to(WIonWS *ws, WRegion *reg, const char *str);
extern WRegion *ionws_farthest(WIonWS *ws, const char *str);
extern WRegion *ionws_goto_dir(WIonWS *ws, const char *str);
extern WRegion *ionws_region_at(WIonWS *ws, int x, int y);

extern WIonFrame *ionws_split_top(WIonWS *ws, const char *dirstr);
extern WIonFrame *ionws_split_at(WIonWS *ws, WIonFrame *frame, 
                                 const char *dirstr, bool attach_current);
extern void ionws_unsplit_at(WIonWS *ws, WIonFrame *frame);

extern WSplit *ionws_split_of(WIonWS *ws, WRegion *reg);


#endif /* ION_IONWS_IONWS_H */
