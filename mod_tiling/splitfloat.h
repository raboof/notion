/*
 * ion/mod_tiling/splitfloat.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_TILING_SPLITFLOAT_H
#define ION_MOD_TILING_SPLITFLOAT_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include "split.h"
#include "tiling.h"

INTRCLASS(WSplitFloat);

#include "panehandle.h"

DECLCLASS(WSplitFloat){
    WSplitSplit ssplit;
    WPaneHandle *tlpwin, *brpwin;
};


extern bool splitfloat_init(WSplitFloat *split, const WRectangle *geom,
                            WTiling *ws, int dir);

extern WSplitFloat *create_splitfloat(const WRectangle *geom, 
                                      WTiling *ws, int dir);

extern void splitfloat_deinit(WSplitFloat *split);

extern void splitfloat_update_handles(WSplitFloat *split, 
                                      const WRectangle *tlg,
                                      const WRectangle *brg);
extern void splitfloat_tl_pwin_to_cnt(WSplitFloat *split, WRectangle *g);
extern void splitfloat_br_pwin_to_cnt(WSplitFloat *split, WRectangle *g);
extern void splitfloat_tl_cnt_to_pwin(WSplitFloat *split, WRectangle *g);
extern void splitfloat_br_cnt_to_pwin(WSplitFloat *split, WRectangle *g);

extern void splitfloat_flip(WSplitFloat *split);

extern WSplit *load_splitfloat(WTiling *ws, const WRectangle *geom, 
                               ExtlTab tab);

extern WSplitRegion *splittree_split_floating(WSplit *node, int dir, 
                                              int primn, int nmins, 
                                              WRegionSimpleCreateFn *fn, 
                                              WTiling *ws);

#endif /* ION_MOD_TILING_SPLITFLOAT_H */
