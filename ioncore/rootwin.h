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

INTROBJ(WRootWin);

#include "window.h"
#include "screen.h"
#include "gr.h"

#define WROOTWIN_ROOT(X) ((X)->wwin.win)

#define ROOTWIN_OF(X) region_rootwin_of((WRegion*)X)
#define ROOT_OF(X) region_root_of((WRegion*)X)
#define GRDATA_OF(X) region_grdata_of((WRegion*)X)
#define FOR_ALL_ROOTWINS(RW)         \
	for(RW=wglobal.rootwins;         \
		RW!=NULL;                    \
		RW=NEXT_CHILD(RW, WRootWin))


DECLOBJ(WRootWin){
	WWindow wwin;
	int xscr;
	
	WRegion *screen_list;
	
	Colormap default_cmap;
	
	Window *tmpwins;
	int tmpnwins;
	
	Window dummy_win;
	
	GC xor_gc;
};


extern WRootWin *manage_rootwin(int xscr, bool noxinerama);
extern void rootwin_deinit(WRootWin *rootwin);

extern WRootWin *region_rootwin_of(const WRegion *reg);
extern Window region_root_of(const WRegion *reg);
extern bool same_rootwin(const WRegion *reg1, const WRegion *reg2);

extern WScreen *rootwin_current_scr(WRootWin *rootwin);

extern void rootwin_manage_initial_windows(WRootWin *rootwin);
extern bool setup_rootwins();

extern Window create_simple_window(WRootWin *rootwin, Window par,
								   const WRectangle *geom);

extern WRootWin *find_rootwin_for_root(Window root);

#endif /* ION_IONCORE_ROOTWIN_H */

