/*
 * ion/ioncore/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_VIEWPORT_H
#define ION_IONCORE_VIEWPORT_H

#include "common.h"
#include "window.h"

INTROBJ(WScreen);

DECLOBJ(WScreen){
#ifdef CF_WINDOWED_SCREENS
	WWindow wwin;
#else
	WRegion reg;
#endif
	int id;
	Atom atom_workspace;
	
	int ws_count;
	WRegion *ws_list;
	WRegion *current_ws;
};

#include "rootwin.h"

extern WScreen *create_screen(WRootWin *rootwin, int id, WRectangle geom);

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

#endif /* ION_IONCORE_VIEWPORT_H */
