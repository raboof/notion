/*
 * wmcore/global.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_GLOBAL_H
#define WMCORE_GLOBAL_H

#include "common.h"

#include <X11/Xutil.h>
#include <X11/Xresource.h>

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

INTRSTRUCT(WGlobal)

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
	Atom atom_frame_id;
	Atom atom_workspace;
	Atom atom_selection;
#ifndef CF_NO_MWM_HINTS	
	Atom atom_mwm_hints;
#endif

	WClientWin *cwin_list;
	WScreen *screens;
	WRegion *focus_next;
	bool warp_next;
	
	WDrawDragwinFn *draw_dragwin;
	WRegion *draw_dragwin_arg;
	
	/* We could have a display WRegion but the screen-link could impose
	 * some problems so these are handled as a special case.
	 */
	WScreen *active_screen, *previous_screen;
	
	int input_mode;
	int opmode;
	int previous_protect;

	Time resize_delay;
	Time dblclick_delay;
	int opaque_resize;
	bool warp_enabled;
};

extern WGlobal wglobal;

#endif /* WMCORE_GLOBAL_H */
