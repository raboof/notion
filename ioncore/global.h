/*
 * ion/ioncore/global.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GLOBAL_H
#define ION_IONCORE_GLOBAL_H

#include "common.h"

#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "rootwin.h"
#include "screen.h"
#include "window.h"
#include "clientwin.h"


enum{
	INPUT_NORMAL,
	INPUT_GRAB,
	INPUT_SUBMAPGRAB,
	INPUT_WAITRELEASE
};

enum{
	OPMODE_INIT,
	OPMODE_NORMAL,
	OPMODE_DEINIT
};

INTRSTRUCT(WGlobal);

typedef void WDrawDragwinFn(WRegion *reg);

DECLSTRUCT(WGlobal){
	int argc;
	char **argv;
	
	Display *dpy;
	const char *display;
	int conn;
	
	XContext win_context;
	Atom atom_wm_state;
	Atom atom_wm_change_state;
	Atom atom_wm_protocols;
	Atom atom_wm_delete;
	Atom atom_wm_take_focus;
	Atom atom_wm_colormaps;
	Atom atom_wm_window_role;
	Atom atom_checkcode;
	Atom atom_selection;
	Atom atom_mwm_hints;
	
	Atom atom_net_wm_name;

	WRootWin *rootwins;
	WScreen *screens;
	WClientWin *cwin_list;
	WRegion *focus_next;
	bool warp_next;
	
	WDrawDragwinFn *draw_dragwin;
	WRegion *draw_dragwin_arg;
	
	/* We could have a display WRegion but the screen-link could impose
	 * some problems so these are handled as a special case.
	 */
	WScreen *active_screen;
	
	int input_mode;
	int opmode;
	int previous_protect;
	
	Time resize_delay;
	Time dblclick_delay;
	int opaque_resize;
	bool warp_enabled;
	
	bool ws_save_enabled;
};


extern WGlobal wglobal;

#endif /* ION_IONCORE_GLOBAL_H */
