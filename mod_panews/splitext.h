/*
 * ion/panews/splitext.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_PANEWS_SPLITEXT_H
#define ION_PANEWS_SPLITEXT_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <mod_ionws/split.h>

INTRCLASS(WSplitUnused);
INTRCLASS(WSplitPane);
INTRCLASS(WSplitFloat);

#include "panews.h"
#include "panehandle.h"
#include "panews.h"

DECLCLASS(WSplitUnused){
    WSplitRegion regnode;
};

DECLCLASS(WSplitPane){
    WSplitInner isplit;
    WSplit *contents;
    char *marker;
};

DECLCLASS(WSplitFloat){
    WSplitSplit ssplit;
    WPaneHandle *tlpwin, *brpwin;
};

extern bool splitunused_init(WSplitUnused *split, const WRectangle *geom,
                             WPaneWS *ws);
extern bool splitpane_init(WSplitPane *split, const WRectangle *geom,
                           WSplit *cnt);
extern bool splitfloat_init(WSplitFloat *split, const WRectangle *geom,
                            WPaneWS *ws, int dir);

extern WSplitUnused *create_splitunused(const WRectangle *geom,
                                        WPaneWS *ws);
extern WSplitPane *create_splitpane(const WRectangle *geom, WSplit *cnt);
extern WSplitFloat *create_splitfloat(const WRectangle *geom, 
                                      WPaneWS *ws, int dir);

extern void splitunused_deinit(WSplitUnused *split);
extern void splitpane_deinit(WSplitPane *split);
extern void splitfloat_deinit(WSplitFloat *split);

extern const char *splitpane_get_marker(WSplitPane *pane);
extern bool splitpane_set_marker(WSplitPane *pane, const char *s);

extern void splitfloat_update_handles(WSplitFloat *split, 
                                      const WRectangle *tlg,
                                      const WRectangle *brg);
extern void splitfloat_tl_pwin_to_cnt(WSplitFloat *split, WRectangle *g);
extern void splitfloat_br_pwin_to_cnt(WSplitFloat *split, WRectangle *g);
extern void splitfloat_tl_cnt_to_pwin(WSplitFloat *split, WRectangle *g);
extern void splitfloat_br_cnt_to_pwin(WSplitFloat *split, WRectangle *g);

extern WRegion *panews_do_get_nextto(WPaneWS *ws, WRegion *reg,
                                     int dir, int primn, bool any);
extern WRegion *panews_do_get_farthest(WPaneWS *ws,
                                       int dir, int primn, bool any);

extern WSplitRegion *split_tree_find_region_in_pane_of(WSplit *node);

#endif /* ION_PANEWS_SPLITEXT_H */