/*
 * wmcore/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_SCREEN_H
#define WMCORE_SCREEN_H

#include "common.h"

INTROBJ(WScreen)

#include "grdata.h"
#include "window.h"

	
#define SCREEN_OF(X) screen_of((WRegion*)X)
#define ROOT_OF(X) root_of((WRegion*)X)
#define GRDATA_OF(X) grdata_of((WRegion*)X)
#define FOR_ALL_SCREENS(SCR)                     \
	for(SCR=wglobal.screens;                     \
		SCR!=NULL;                               \
		SCR=(WScreen*)(((WThing*)(SCR))->t_next))

#define SCREEN_MAX_STACK 3

#define IS_SCREEN(REG) ((void*)(REG)==((WRegion*)(REG))->screen)


DECLOBJ(WScreen){
	WWindow root;
	int xscr;
	WRegion *default_viewport;
	
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


extern Window create_simple_window_bg(WScreen *scr, WWinGeomParams params,
									  ulong background);
extern Window create_simple_window(WScreen *scr, WWinGeomParams params);

extern WScreen *manage_screen(int xscr);
extern void deinit_screen(WScreen *scr);

extern WScreen *screen_of(const WRegion *reg);
extern WGRData *grdata_of(const WRegion *reg);
extern Window root_of(const WRegion *reg);
extern bool same_screen(const WRegion *reg1, const WRegion *reg2);

extern void manage_initial_windows(WScreen *scr);

extern void screen_switch_nth2(int scrnum, int n);

#endif /* WMCORE_SCREEN_H */

