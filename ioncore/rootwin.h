/*
 * ion/ioncore/rootwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_ROOTWIN_H
#define ION_IONCORE_ROOTWIN_H

#include "common.h"
#include "window.h"
#include "screen.h"
#include "gr.h"
#include "rectangle.h"
#include "screen.h"


#define WROOTWIN_ROOT(X) ((X)->wwin.win)
#define FOR_ALL_ROOTWINS(RW)         \
    for(RW=ioncore_g.rootwins;         \
        RW!=NULL;                    \
        RW=OBJ_CAST(((WRegion*)RW)->p_next, WRootWin))


DECLCLASS(WRootWin){
    WWindow wwin;
    int xscr;
    
    Colormap default_cmap;
    
    Window *tmpwins;
    int tmpnwins;
    
    Window dummy_win;
    
    GC xor_gc;
};


extern void rootwin_deinit(WRootWin *rootwin);
extern WScreen *rootwin_current_scr(WRootWin *rootwin);

extern void rootwin_manage_initial_windows(WRootWin *rootwin);
extern WRootWin *create_rootwin(int xscr);

#endif /* ION_IONCORE_ROOTWIN_H */

