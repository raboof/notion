/*
 * ion/ioncore/rootwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_ROOTWIN_H
#define ION_IONCORE_ROOTWIN_H

#include "common.h"

INTROBJ(WRootWin);

#include "grdata.h"
#include "window.h"
#include "screen.h"

	
#define ROOTWIN_OF(X) region_rootwin_of((WRegion*)X)
#define ROOT_OF(X) region_root_of((WRegion*)X)
#define GRDATA_OF(X) region_grdata_of((WRegion*)X)
#define FOR_ALL_ROOTWINS(RW)         \
	for(RW=wglobal.rootwins;         \
		RW!=NULL;                    \
		RW=NEXT_CHILD(RW, WRootWin))


DECLOBJ(WRootWin){
	WRegion reg;
	Window root;
	int xscr;
	
	WRegion *screen_list;
	WScreen *current_screen;
	
	Colormap default_cmap;
	
	Window *tmpwins;
	int tmpnwins;
	
	WGRData grdata;
};


extern Window create_simple_window_bg(const WGRData *grdata, Window par,
									  WRectangle geom, WColor background);
extern Window create_simple_window(const WGRData *grdata, Window par,
								   WRectangle geom);

extern WRootWin *manage_rootwin(int xscr);
extern void rootwin_deinit(WRootWin *rootwin);

extern WRootWin *region_rootwin_of(const WRegion *reg);
extern WGRData *region_grdata_of(const WRegion *reg);
extern Window region_root_of(const WRegion *reg);
extern bool same_rootwin(const WRegion *reg1, const WRegion *reg2);

extern WScreen *rootwin_current_scr(WRootWin *rootwin);

extern void rootwin_manage_initial_windows(WRootWin *rootwin);
extern bool setup_rootwins();


extern WBindmap ioncore_rootwin_bindmap;


#endif /* ION_IONCORE_ROOTWIN_H */

