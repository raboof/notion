/*
 * ion/mod_tiling/split.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_TILING_SPLIT_H
#define ION_MOD_TILING_SPLIT_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/reginfo.h>
#include <ioncore/attach.h>
#include <ioncore/rectangle.h>
#include <libextl/extl.h>


INTRCLASS(WSplit);
INTRCLASS(WSplitInner);
INTRCLASS(WSplitSplit);
INTRCLASS(WSplitRegion);
INTRCLASS(WSplitST);


typedef enum{
    SPLIT_HORIZONTAL,
    SPLIT_VERTICAL
} WSplitDir;


typedef enum{
    PRIMN_ANY,
    PRIMN_TL,
    PRIMN_BR,
    PRIMN_NONE
} WPrimn;


typedef enum {
    SPLIT_CURRENT_TL,
    SPLIT_CURRENT_BR
} WSplitCurrent;


DECLCLASS(WSplit){
    Obj obj;
    WRectangle geom;
    WSplitInner *parent;
    void *ws_if_root;
    
    int min_w, min_h;
    int max_w, max_h;
    int unused_w, unused_h;
};


DECLCLASS(WSplitInner){
    WSplit split;
};


DECLCLASS(WSplitSplit){
    WSplitInner isplit;
    int dir;
    WSplit *tl, *br;
    int current;
};


DECLCLASS(WSplitRegion){
    WSplit split;
    WRegion *reg;
};


DECLCLASS(WSplitST){
    WSplitRegion regnode;
    int orientation;
    int corner;
    bool fullsize;
};


typedef struct{
    int tl, br;
    bool any;
} RootwardAmount;


typedef bool WSplitFilter(WSplit *split);
typedef void WSplitFn(WSplit *split);


/* Misc. */

extern int split_size(WSplit *split, int dir);
extern int split_pos(WSplit *split, int dir);
extern int split_other_size(WSplit *split, int dir);
extern int split_other_pos(WSplit *split, int dir);

extern WSplitRegion *splittree_node_of(WRegion *reg);
extern bool splittree_set_node_of(WRegion *reg, WSplitRegion *split);

extern WPrimn primn_invert(WPrimn primn);
extern WPrimn primn_none2any(WPrimn primn);

/* Init/deinit */

extern bool split_init(WSplit *split, const WRectangle *geom);
extern bool splitinner_init(WSplitInner *split, const WRectangle *geom);
extern bool splitsplit_init(WSplitSplit *split, const WRectangle *geom, 
                            int dir);
extern bool splitregion_init(WSplitRegion *split, const WRectangle *geom,
                             WRegion *reg);
extern bool splitst_init(WSplitST *split, const WRectangle *geom,
                         WRegion *reg);


extern WSplitSplit *create_splitsplit(const WRectangle *geom, int dir);
extern WSplitRegion *create_splitregion(const WRectangle *geom, WRegion *reg);
extern WSplitST *create_splitst(const WRectangle *geom, WRegion *reg);


extern void split_deinit(WSplit *split);
extern void splitsplit_deinit(WSplitSplit *split);
extern void splitinner_deinit(WSplitInner *split);
extern void splitregion_deinit(WSplitRegion *split);
extern void splitst_deinit(WSplitST *split);

/* Geometry */

DYNFUN void split_update_bounds(WSplit *node, bool recursive);
extern void splitsplit_update_geom_from_children(WSplitSplit *node);
DYNFUN void split_do_resize(WSplit *node, const WRectangle *ng, 
                            WPrimn hprimn, WPrimn vprimn, bool transpose);
extern void splitsplit_do_resize(WSplitSplit *node, const WRectangle *ng, 
                                 WPrimn hprimn, WPrimn vprimn, bool transpose);
extern void split_resize(WSplit *node, const WRectangle *ng, 
                         WPrimn hprimn, WPrimn vprimn);
extern bool split_maximize(WSplit *node, int dir, int action);
DYNFUN void split_do_maxhelper(WSplit *node, int dir, int action);
DYNFUN bool split_do_restore(WSplit *node, int dir);
DYNFUN bool split_do_verify(WSplit *node, int dir);
DYNFUN void splitinner_do_rqsize(WSplitInner *p, WSplit *node, 
                                 RootwardAmount *ha, RootwardAmount *va, 
                                 WRectangle *rg, bool tryonly);
extern ExtlTab split_rqgeom(WSplit *node, ExtlTab g);

/* Split */

extern void splittree_rqgeom(WSplit *node, int flags, 
                             const WRectangle *geom, WRectangle *geomret);


DYNFUN void splitinner_replace(WSplitInner *node, WSplit *child, WSplit *what);
extern WSplitRegion *splittree_split(WSplit *node, int dir, 
                                     WPrimn primn, int minsize, 
                                     WRegionSimpleCreateFn *fn,
                                     WWindow *parent);

extern void splittree_changeroot(WSplit *root, WSplit *node);

/* Remove */

DYNFUN void splitinner_remove(WSplitInner *node, WSplit *child, 
                              bool reclaim_space);
extern void splittree_remove(WSplit *node, bool reclaim_space);

/* Tree traversal */

extern WSplit *split_find_root(WSplit *split);
DYNFUN WSplit *split_current_todir(WSplit *node, 
                                   WPrimn hprimn, WPrimn vprimn,
                                   WSplitFilter *filter);
DYNFUN WSplit *splitinner_nextto(WSplitInner *node, WSplit *child,
                                 WPrimn hprimn, WPrimn vprimn, 
                                 WSplitFilter *filter);
DYNFUN WSplit *splitinner_current(WSplitInner *node);
DYNFUN void splitinner_mark_current(WSplitInner *split, WSplit *child);
DYNFUN void splitinner_forall(WSplitInner *node, WSplitFn *fn);
extern WSplit *split_nextto(WSplit *node,
                            WPrimn hprimn, WPrimn vprimn,
                            WSplitFilter *filter);

/* X window handling */

void split_restack(WSplit *split, Window win, int mode);
void split_stacking(WSplit *split, Window *bottom_ret, Window *top_ret);
void split_map(WSplit *split);
void split_unmap(WSplit *split);
void split_reparent(WSplit *split, WWindow *wwin);

/* Transpose, flip, rotate */

extern void split_transpose(WSplit *split);
extern bool split_transpose_to(WSplit *split, const WRectangle *geom);

extern void splitsplit_flip_default(WSplitSplit *split);
DYNFUN void splitsplit_flip(WSplitSplit *split);

extern bool split_rotate_to(WSplit *node, const WRectangle *geom, 
                            int rotation);

/* Save support */

extern bool split_get_config(WSplit *node, ExtlTab *ret);
extern ExtlTab split_base_config(WSplit *node);

/* Internal. */

extern void splittree_begin_resize();
extern void splittree_end_resize();
extern void splittree_scan_stdisp_rootward(WSplit *node);

extern void split_do_rqgeom_(WSplit *node, const WRectangle *ng, 
                             bool hany, bool vany, WRectangle *rg, 
                             bool tryonly);


#endif /* ION_MOD_TILING_SPLIT_H */
