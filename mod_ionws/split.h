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
#include <ioncore/mplex.h>


enum WSplitDir{
    HORIZONTAL,
    VERTICAL
};


enum PrimaryNode{
    ANY,
    TOP_OR_LEFT,
    BOTTOM_OR_RIGHT
};


INTRCLASS(WWsSplit);
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

extern int split_tree_size(Obj *obj, int dir);
extern int split_tree_pos(Obj *obj, int dir);
extern int split_tree_other_size(Obj *obj, int dir);

extern void split_tree_mark_current(WRegion *reg);

extern WWsSplit *split_tree_split_of(Obj *obj);

/* x and y are in relative to parent */
extern WRegion *split_tree_region_at(Obj *obj, int x, int y);
extern WRegion *split_tree_current_tl(Obj *obj, int dir);
extern WRegion *split_tree_current_br(Obj *obj, int dir);
extern WRegion *split_tree_to_tl(WRegion *reg, int dir);
extern WRegion *split_tree_to_br(WRegion *reg, int dir);
extern WMPlex *split_tree_find_mplex(WRegion *from);

extern WRegion *split_tree_split(Obj **root, Obj *obj, int dir, 
                                 int primn, int minsize, int oprimn, 
                                 WRegionSimpleCreateFn *fn,
                                 WWindow *parent);

extern WRegion *split_tree_remove(Obj **root, WRegion *reg,
                                  bool reclaim_space);

extern void split_tree_rqgeom(Obj *root, Obj *obj, int flags, 
                              const WRectangle *geom,
                              WRectangle *geomret);

extern int split_tree_do_calcresize(Obj *node_, int dir, int primn, 
                                    int nsize);
extern void split_tree_resize(Obj *node, int dir, int primn,
                              int npos, int nsize);
extern void split_tree_do_resize(Obj *node, int dir, int primn,
                                 int npos, int nsize);

#endif /* ION_IONWS_SPLIT_H */
