/*
 * ion/ionws/split.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <ioncore/rectangle.h>

#include "ionframe.h"


INTRCLASS(WWsSplit);


enum WSplitDir{
    HORIZONTAL,
    VERTICAL
};


enum PrimaryNode{
    ANY,
    TOP_OR_LEFT,
    BOTTOM_OR_RIGHT
};


DECLCLASS(WWsSplit){
    Obj obj;
    int dir;
    WRectangle geom;
    int current;
    Obj *tl, *br;
    WWsSplit *parent;
    uint max_w, min_w, max_h, min_h;
};


extern WWsSplit *create_split(int dir, Obj *tl, Obj *br, 
                              const WRectangle *geom);
extern int split_tree_do_calcresize(Obj *node_, int dir, int primn, 
                                    int nsize);
extern void split_tree_resize(Obj *node, int dir, int primn,
                              int npos, int nsize);
extern void split_tree_do_resize(Obj *node, int dir, int primn,
                                 int npos, int nsize);

extern int split_tree_size(Obj *obj, int dir);
extern int split_tree_pos(Obj *obj, int dir);
extern int split_tree_other_size(Obj *obj, int dir);

#include "ionws.h"

extern void ionws_add_managed(WIonWS *ws, WRegion *reg);
extern void ionws_managed_activated(WIonWS *ws, WRegion *reg);
extern bool ionws_manage_rescue(WIonWS *ws, WClientWin *cwin, WRegion *from);
extern void ionws_request_managed_geom(WIonWS *ws, WRegion *reg,
                                       int flags, const WRectangle *geom,
                                       WRectangle *geomret);
extern void ionws_remove_managed(WIonWS *ws, WRegion *reg);

extern WRegion *ionws_current(WIonWS *ws);
extern WRegion *ionws_next_to(WIonWS *ws, WRegion *reg, const char *str);
extern WRegion *ionws_farthest(WIonWS *ws, const char *str);
extern WRegion *ionws_goto_dir(WIonWS *ws, const char *str);

extern WRegion *ionws_region_at(WIonWS *ws, int x, int y);

extern WRegion *ionws_do_split_at(WIonWS *ws, Obj *obj, int dir, 
                                  int primn, int minsize, int oprimn,
                                  WRegionSimpleCreateFn *fn);
extern WIonFrame *ionws_split_top(WIonWS *ws, const char *dirstr);
extern WIonFrame *ionws_split_at(WIonWS *ws, WIonFrame *frame, 
                                 const char *dirstr, bool attach_current);

extern void ionws_unsplit_at(WIonWS *ws, WIonFrame *frame);


#endif /* ION_IONWS_SPLIT_H */

