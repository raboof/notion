/*
 * ion/ioncore/rootwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_ROOTWIN_H
#define ION_IONCORE_ROOTWIN_H

#include "common.h"
#include "window.h"
#include "screen.h"
#include "gr.h"
#include "rectangle.h"

#define WROOTWIN_ROOT(X) ((X)->wwin.win)
#define FOR_ALL_ROOTWINS(RW)         \
    for(RW=ioncore_g.rootwins;         \
        RW!=NULL;                    \
        RW=OBJ_CAST(((WRegion*)RW)->p_next, WRootWin))


DECLCLASS(WRootWin){
    WWindow wwin;
    int xscr;
    
    WRegion *screen_list;
    
    Colormap default_cmap;
    
    Window *tmpwins;
    int tmpnwins;
    
    Window dummy_win;
    
    GC xor_gc;
};


extern void rootwin_deinit(WRootWin *rootwin);
extern WScreen *rootwin_current_scr(WRootWin *rootwin);

extern void rootwin_manage_initial_windows(WRootWin *rootwin);
extern WRootWin *ioncore_manage_rootwin(int xscr, bool noxinerama);

extern Window create_xwindow(WRootWin *rootwin, Window par,
                             const WRectangle *geom);

#endif /* ION_IONCORE_ROOTWIN_H */

