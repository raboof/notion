/*
 * ion/mod_tiling/tiling.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_TILING_TILING_H
#define ION_MOD_TILING_TILING_H

#include <libtu/ptrlist.h>
#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/rectangle.h>
#include <ioncore/pholder.h>
#include <ioncore/navi.h>
#include "split.h"


INTRCLASS(WTiling);
DECLCLASS(WTiling){
    WRegion reg;
    WSplit *split_tree;
    WSplitST *stdispnode;
    PtrList *managed_list;
    WRegionSimpleCreateFn *create_frame_fn;
    Window dummywin;
};


extern bool tiling_init(WTiling *ws, WWindow *parent, const WFitParams *fp,
                       WRegionSimpleCreateFn *create_frame_fn, bool ci);
extern WTiling *create_tiling(WWindow *parent, const WFitParams *fp, 
                            WRegionSimpleCreateFn *create_frame_fn, bool ci);
extern WTiling *create_tiling_simple(WWindow *parent, const WFitParams *fp);
extern void tiling_deinit(WTiling *ws);

extern ExtlTab tiling_resize_tree(WTiling *ws, WSplit *node, ExtlTab g);

extern WRegion *tiling_current(WTiling *ws);
extern WRegion *tiling_nextto(WTiling *ws, WRegion *reg, const char *str, bool any);
extern WRegion *tiling_farthest(WTiling *ws, const char *str, bool any);
extern WRegion *tiling_region_at(WTiling *ws, int x, int y);

extern WFrame *tiling_split_top(WTiling *ws, const char *dirstr);
extern WFrame *tiling_split_at(WTiling *ws, WFrame *frame, 
                              const char *dirstr, bool attach_current);
extern bool tiling_unsplit_at(WTiling *ws, WFrame *frame);

extern WSplitSplit *tiling_set_floating(WTiling *ws, WSplitSplit *split, 
                                       int sp);

extern WSplit *tiling_split_tree(WTiling *ws);
extern WSplit *tiling_split_of(WTiling *ws, WRegion *reg);

extern void tiling_do_managed_remove(WTiling *ws, WRegion *reg);

DYNFUN bool tiling_managed_add(WTiling *ws, WRegion *reg);
extern bool tiling_managed_add_default(WTiling *ws, WRegion *reg);

extern WRegion *tiling_do_navi_next(WTiling *ws, WRegion *reg, 
                                    WRegionNavi nh, bool nowrap,
                                    bool any);
extern WRegion *tiling_do_navi_first(WTiling *ws, WRegionNavi nh, 
                                     bool any);
extern WRegion *tiling_navi_next(WTiling *ws, WRegion *reg, 
                                 WRegionNavi nh, WRegionNaviData *data);
extern WRegion *tiling_navi_first(WTiling *ws, WRegionNavi nh,
                                  WRegionNaviData *data);

/* Inherited dynfun implementations */

extern bool tiling_fitrep(WTiling *ws, WWindow *par, const WFitParams *fp);
extern void tiling_map(WTiling *ws);
extern void tiling_unmap(WTiling *ws);
extern ExtlTab tiling_get_configuration(WTiling *ws);
extern void tiling_managed_rqgeom(WTiling *ws, WRegion *reg,
                                 int flags, const WRectangle *geom,
                                 WRectangle *geomret);
extern void tiling_managed_remove(WTiling *ws, WRegion *reg);
extern void tiling_managed_activated(WTiling *ws, WRegion *reg);
extern bool tiling_rescue_clientwins(WTiling *ws, WPHolder *ph);
extern WPHolder *tiling_get_rescue_pholder_for(WTiling *ws, WRegion *mgd);
extern void tiling_do_set_focus(WTiling *ws, bool warp);
extern bool tiling_managed_prepare_focus(WTiling *ws, WRegion *reg, 
                                        int flags, WPrepareFocusResult *res);
extern bool tiling_managed_may_destroy(WTiling *ws, WRegion *reg);
extern void tiling_manage_stdisp(WTiling *ws, WRegion *stdisp, 
                                const WMPlexSTDispInfo *di);
extern void tiling_unmanage_stdisp(WTiling *ws, bool permanent, bool nofocus);

extern void tiling_fallback_focus(WTiling *ws, bool warp);

/* Loading */

/* Stupid C can't handle recursive 'WSplitLoadFn *fn' here, so we have
 * to use the void pointer.
 */
typedef WSplit *WSplitLoadFn(WTiling *ws, const WRectangle *geom, ExtlTab tab);

extern WRegion *tiling_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

DYNFUN WSplit *tiling_load_node(WTiling *ws, const WRectangle *geom, ExtlTab tab);
extern WSplit *tiling_load_node_default(WTiling *ws, const WRectangle *geom, ExtlTab tab);

extern WSplit *load_splitregion(WTiling *ws, const WRectangle *geom, ExtlTab tab);
extern WSplit *load_splitsplit(WTiling *ws, const WRectangle *geom, ExtlTab tab);
extern WSplit *load_splitst(WTiling *ws, const WRectangle *geom, ExtlTab tab);

/* Iteration */

typedef PtrListIterTmp WTilingIterTmp;

#define FOR_ALL_MANAGED_BY_TILING(VAR, WS, TMP) \
    FOR_ALL_ON_PTRLIST(WRegion*, VAR, (WS)->managed_list, TMP)
    
#define FOR_ALL_MANAGED_BY_TILING_UNSAFE(VAR, WS) \
    FOR_ALL_ON_PTRLIST_UNSAFE(WRegion*, VAR, (WS)->managed_list)


#endif /* ION_MOD_TILING_TILING_H */
