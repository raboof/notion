/*
 * ion/mod_ionws/split.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_IONWS_SPLIT_H
#define ION_MOD_IONWS_SPLIT_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/reginfo.h>
#include <ioncore/attach.h>
#include <ioncore/rectangle.h>
#include <ioncore/mplex.h>


enum WSplitType{
    SPLIT_REGNODE,
    SPLIT_HORIZONTAL,
    SPLIT_VERTICAL,
    SPLIT_UNUSED,
    SPLIT_STDISPNODE
};


enum WPrimaryNode{
    PRIMN_ANY,
    PRIMN_TL,
    PRIMN_BR
};


enum WSplitCurrent{
    SPLIT_CURRENT_TL,
    SPLIT_CURRENT_BR
};


INTRCLASS(WSplit);
DECLCLASS(WSplit){
    Obj obj;
    int type;
    WRectangle geom;
    WSplit *parent;
    
    int min_w, min_h;
    int max_w, max_h;
    int unused_w, unused_h;

    char *marker;
    
    union{
        struct{
            int current;
            WSplit *tl, *br;
        } s;
        struct{
            WRegion *reg;
            int orientation;
            int corner;
        } d;
        WRegion *reg;
    } u;
};


typedef bool WSplitFilter(WSplit *split);
    

extern WSplit *create_split(const WRectangle *geom, int dir);
extern WSplit *create_split_regnode(const WRectangle *geom, WRegion *reg);
extern WSplit *create_split_unused(const WRectangle *geom);

extern void split_deinit(WSplit *split);

extern int split_size(WSplit *split, int dir);
extern int split_pos(WSplit *split, int dir);
extern int split_other_size(WSplit *split, int dir);
extern int split_other_pos(WSplit *split, int dir);

extern Obj *split_hoist(WSplit *split);

extern void split_mark_current(WSplit *split);

extern WSplit *split_current_tl(WSplit *node, int dir, WSplitFilter *filter);
extern WSplit *split_current_br(WSplit *node, int dir, WSplitFilter *filter);
extern WSplit *split_to_tl(WSplit *node, int dir, WSplitFilter *filter);
extern WSplit *split_to_br(WSplit *node, int dir, WSplitFilter *filter);
extern WSplit *split_closest_leaf(WSplit *node, WSplitFilter *filter);

extern void split_update_bounds(WSplit *node, bool recursive);
extern void split_resize(WSplit *node, const WRectangle *ng, 
                         int hprimn, int vprimn);
extern bool split_do_resize(WSplit *node, const WRectangle *ng, 
                            int hprimn, int vprimn, bool transpose,
                            void (*justcheck)(WSplit *node, 
                                              const WRectangle *g));
extern void split_tree_rqgeom(WSplit *root, WSplit *node, int flags, 
                              const WRectangle *geom, WRectangle *geomret);

extern WSplit *split_tree_split(WSplit **root, WSplit *node, int dir, 
                                int primn, int minsize, 
                                WRegionSimpleCreateFn *fn,
                                WWindow *parent);
extern void split_tree_remove(WSplit **root, WSplit *node,
                              bool reclaim_space, bool lazy);

/* x and y are in relative to parent */
extern WRegion *split_region_at(WSplit *node, int x, int y);

extern WSplit *split_tree_node_of(WRegion *reg);
extern WSplit *split_tree_split_of(WRegion *reg);
extern WMPlex *split_tree_find_mplex(WRegion *from);

extern bool split_tree_set_node_of(WRegion *reg, WSplit *split);

extern void split_transpose(WSplit *split);
extern void split_transpose_to(WSplit *split, const WRectangle *geom);

extern void split_update_geom_from_children(WSplit *node);

extern const char *split_get_marker(WSplit *node);
extern bool split_set_marker(WSplit *node, const char *s);

#define CHKNODE_(NODE)                                             \
    assert(((NODE)->type==SPLIT_REGNODE && (NODE)->u.reg!=NULL) || \
           ((NODE)->type==SPLIT_STDISPNODE) ||                     \
           ((NODE)->type==SPLIT_UNUSED) ||                         \
           (((NODE)->type==SPLIT_VERTICAL ||                       \
             (NODE)->type==SPLIT_HORIZONTAL)                       \
            && ((NODE)->u.s.tl!=NULL && (NODE)->u.s.br!=NULL)))


#define CHKNODE(NODE)                                               \
    assert((NODE)->type==SPLIT_REGNODE ||                           \
           (NODE)->type==SPLIT_STDISPNODE ||                        \
           (NODE)->type==SPLIT_UNUSED ||                            \
           (NODE)->type==SPLIT_VERTICAL ||                          \
           (NODE)->type==SPLIT_HORIZONTAL);                         \
    assert((NODE)->type!=SPLIT_REGNODE || (NODE)->u.reg!=NULL);     \
    /*assert((NODE)->type!=SPLIT_STDISPNODE || (NODE)->u.reg!=NULL);*/  \
    assert((NODE)->type!=SPLIT_VERTICAL || (NODE)->u.s.tl!=NULL);   \
    assert((NODE)->type!=SPLIT_VERTICAL || (NODE)->u.s.br!=NULL);   \
    assert((NODE)->type!=SPLIT_HORIZONTAL || (NODE)->u.s.tl!=NULL); \
    assert((NODE)->type!=SPLIT_HORIZONTAL || (NODE)->u.s.br!=NULL);
            

#endif /* ION_MOD_IONWS_SPLIT_H */
