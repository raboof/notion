/*
 * ion/panews/splitfloat.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_PANEWS_SPLITFLOAT_H
#define ION_PANEWS_SPLITFLOAT_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <mod_ionws/split.h>

INTRCLASS(WSplitFloat);

#include "panehandle.h"
#include "panews.h"

DECLCLASS(WSplitFloat){
    WSplitSplit ssplit;
    WPaneHandle *tlpwin, *brpwin;
};


extern bool splitfloat_init(WSplitFloat *split, const WRectangle *geom,
                            WPaneWS *ws, int dir);

extern WSplitFloat *create_splitfloat(const WRectangle *geom, 
                                      WPaneWS *ws, int dir);

extern void splitfloat_deinit(WSplitFloat *split);

extern void splitfloat_update_handles(WSplitFloat *split, 
                                      const WRectangle *tlg,
                                      const WRectangle *brg);
extern void splitfloat_tl_pwin_to_cnt(WSplitFloat *split, WRectangle *g);
extern void splitfloat_br_pwin_to_cnt(WSplitFloat *split, WRectangle *g);
extern void splitfloat_tl_cnt_to_pwin(WSplitFloat *split, WRectangle *g);
extern void splitfloat_br_cnt_to_pwin(WSplitFloat *split, WRectangle *g);

extern void splitfloat_flip(WSplitFloat *split);

#endif /* ION_PANEWS_SPLITFLOAT_H */
