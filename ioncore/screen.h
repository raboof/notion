/*
 * ion/ioncore/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_SCREEN_H
#define ION_IONCORE_SCREEN_H

#include "common.h"

INTROBJ(WScreen);

#include "grdata.h"
#include "window.h"
#include "viewport.h"

	
#define SCREEN_OF(X) region_screen_of((WRegion*)X)
#define ROOT_OF(X) region_root_of((WRegion*)X)
#define GRDATA_OF(X) region_grdata_of((WRegion*)X)
#define FOR_ALL_SCREENS(SCR)                     \
	for(SCR=wglobal.screens;                     \
		SCR!=NULL;                               \
		SCR=NEXT_CHILD(SCR, WScreen))

#define SCREEN_MAX_STACK 3


DECLOBJ(WScreen){
	WWindow root;
	int xscr;
	WRegion *viewport_list;
	WViewport *default_viewport;
	WViewport *current_viewport;
	
	Colormap default_cmap;
	
	int w_unit, h_unit;
	
	union{
		WWindow *stack_lists[SCREEN_MAX_STACK];
	} u;
	
	Window *tmpwins;
	int tmpnwins;
	
	WGRData grdata;
	
	void *bcount;
	int n_bcount;
};


extern Window create_simple_window_bg(const WScreen *scr, Window par,
									  WRectangle geom, WColor background);
extern Window create_simple_window(const WScreen *scr, Window par,
								   WRectangle geom);

extern WScreen *manage_screen(int xscr);
extern void screen_deinit(WScreen *scr);

extern WScreen *region_screen_of(const WRegion *reg);
extern WGRData *region_grdata_of(const WRegion *reg);
extern Window region_root_of(const WRegion *reg);
extern bool same_screen(const WRegion *reg1, const WRegion *reg2);

extern void screen_switch_nth2(int scrnum, int n);

extern void screen_manage_initial_windows(WScreen *scr);
extern bool setup_screens();


extern WBindmap ioncore_screen_bindmap;


#endif /* ION_IONCORE_SCREEN_H */

