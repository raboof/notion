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


enum WSplitType{
    SPLIT_REGNODE,
    SPLIT_HORIZONTAL,
    SPLIT_VERTICAL
};


enum WPrimaryNode{
    PRIMN_ANY,
    PRIMN_TL,
    PRIMN_BR
};


INTRCLASS(WSplit);
DECLCLASS(WSplit){
    Obj obj;
    int type;
    WRectangle geom;
    WSplit *parent;
    
    int min_w, min_h;
    int max_w, max_h;
    int used_w, used_h;
    
    union{
        struct{
            int current;
            WSplit *tl, *br;
        } s;
        WRegion *reg;
    } u;
};


extern WSplit *create_split(int dir, WSplit *tl, WSplit *br, 
                            const WRectangle *geom);
extern WSplit *create_split_regnode(WRegion *reg, const WRectangle *geom);
extern void split_deinit(WSplit *split);

extern int split_size(WSplit *split, int dir);
extern int split_pos(WSplit *split, int dir);
extern int split_other_size(WSplit *split, int dir);
extern int split_other_pos(WSplit *split, int dir);

extern void split_mark_current(WSplit *split);

/* x and y are in relative to parent */
extern WSplit *split_current_tl(WSplit *node, int dir);
extern WSplit *split_current_br(WSplit *node, int dir);
extern WSplit *split_to_tl(WSplit *node, int dir);
extern WSplit *split_to_br(WSplit *node, int dir);

extern void split_resize(WSplit *node, const WRectangle *ng, 
                         int hprimn, int vprimn);
extern void split_do_resize(WSplit *node, const WRectangle *ng, 
                            int hprimn, int vprimn);

extern WSplit *split_tree_split(WSplit **root, WSplit *node, int dir, 
                                int primn, int minsize, int oprimn, 
                                WRegionSimpleCreateFn *fn,
                                WWindow *parent);
extern WSplit *split_tree_remove(WSplit **root, WSplit *node,
                                 bool reclaim_space);
extern void split_tree_rqgeom(WSplit *root, WSplit *node, int flags, 
                              const WRectangle *geom, WRectangle *geomret);

extern WRegion *split_region_at(WSplit *node, int x, int y);

extern WSplit *split_tree_node_of(WRegion *reg);
extern WSplit *split_tree_split_of(WRegion *reg);
extern WMPlex *split_tree_find_mplex(WRegion *from);

#endif /* ION_IONWS_SPLIT_H */
