/*
 * ion/ioncore/viewport.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_VIEWPORT_H
#define ION_IONCORE_VIEWPORT_H

#include "common.h"
#include "region.h"

INTROBJ(WViewport);

#include "screen.h"

DECLOBJ(WViewport){
	WRegion reg;
	int id;
	Atom atom_workspace;
	
	int ws_count;
	WRegion *ws_list;
	WRegion *current_ws;
};


extern WViewport *create_viewport(WScreen *scr, int id, WRectangle geom);

extern bool viewport_initialize_workspaces(WViewport* vp);

extern void viewport_switch_nth(WViewport *vp, uint n);
extern void viewport_switch_next(WViewport *vp);
extern void viewport_switch_prev(WViewport *vp);

extern WViewport *region_viewport_of(WRegion *reg);

extern WRegion *viewport_current(WViewport *vp);

/* For viewports corresponding to Xinerama screens <id> is initially set
 * to the Xinerama screen number. When Xinerama is not enabled, <id> is
 * the X screen number (which is the same for all Xinerama screens).
 * For all other viewports <id> is undefined.
 */
extern WViewport *find_viewport_id(int id);
extern void goto_viewport_id(int id);
extern void goto_next_viewport();
extern void goto_prev_viewport();

#endif /* ION_IONCORE_VIEWPORT_H */
