/*
 * ion/ioncore/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SCREEN_H
#define ION_IONCORE_SCREEN_H

#include "common.h"

INTROBJ(WScreen);

#include "window.h"

#define FOR_ALL_SCREENS(SCR)   \
	for((SCR)=wglobal.screens; \
		(SCR)!=NULL;           \
		(SCR)=(SCR)->next_scr)


DECLOBJ(WScreen){
	WWindow wwin;
	int id;
	Atom atom_workspace;
	bool uses_root;
	
	int ws_count;
	WRegion *ws_list;
	WRegion *current_ws;
	
	WScreen *next_scr, *prev_scr;
};

#include "rootwin.h"

extern WScreen *create_screen(WRootWin *rootwin, int id, WRectangle geom,
							  bool useroot);

extern bool screen_initialize_workspaces(WScreen *scr);

extern void screen_switch_nth(WScreen *scr, uint n);
extern void screen_switch_next(WScreen *scr);
extern void screen_switch_prev(WScreen *scr);

extern WScreen *region_screen_of(WRegion *reg);

extern WRegion *screen_current(WScreen *scr);

/* For viewports corresponding to Xinerama rootwins <id> is initially set
 * to the Xinerama screen number. When Xinerama is not enabled, <id> is
 * the X screen number (which is the same for all Xinerama rootwins).
 * For all other viewports <id> is undefined.
 */
extern WScreen *find_screen_id(int id);
extern void goto_screen_id(int id);
extern void goto_next_screen();
extern void goto_prev_screen();

#endif /* ION_IONCORE_SCREEN_H */
