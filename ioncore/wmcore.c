/*
 * wmcore/init.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libtu/util.h>
#include <X11/Xlib.h>

#include "common.h"
#include "screen.h"
#include "event.h"
#include "cursor.h"
#include "signal.h"
#include "binding.h"
#include "readconfig.h"
#include "global.h"
#include "draw.h"
#include "modules.h"
#include "imports.h"
#include "wsreg.h"
#include "funtabs.h"


/*{{{ Global variables */


WGlobal wglobal;


/*}}}*/

	
/*{{{ Init */

static void init_hooks()
{
	ADD_HOOK(add_clientwin_alt, add_clientwin_default);
}


static void initialize_global()
{
	WRectangle zero_geom={0, 0, 0, 0};
	
	/* argc, argv must be set be the program */
	wglobal.dpy=NULL;
	wglobal.display=NULL;

	wglobal.screens=NULL;
	wglobal.focus_next=NULL;
	wglobal.warp_next=FALSE;
	wglobal.cwin_list=NULL;
	
	wglobal.active_screen=NULL;
	wglobal.previous_screen=NULL;

	wglobal.ggrab_top=NULL;
	
	wglobal.input_mode=INPUT_NORMAL;
	wglobal.opmode=OPMODE_INIT;
	wglobal.previous_protect=0;
	wglobal.dblclick_delay=CF_DBLCLICK_DELAY;
	wglobal.resize_delay=CF_RESIZE_DELAY;
	wglobal.opaque_resize=0;
	wglobal.warp_enabled=TRUE;
}


bool wmcore_init(const char *appname, const char *appetcdir,
				 const char *display, bool onescreen)
{
	Display *dpy;
	WScreen *scr;
	int i, dscr, nscr;
	static bool called=FALSE;
	
	/* Sorry, this function can not be re-entered due to laziness
	 * towards implementing checking of things already initialized.
	 * Nobody would call this twice anyway.
	 */
	assert(!called);
	called=TRUE;
	
	initialize_global();
	wmcore_init_funclists();
	
	if(!wmcore_set_cfgpath(appname, appetcdir))
		return FALSE;
		
	/* Open the display. */
	dpy=XOpenDisplay(display);
	
	if(dpy==NULL){
		warn("Could not connect to X display '%s'", XDisplayName(display));
		return FALSE;
	}

	if(onescreen){
		dscr=DefaultScreen(dpy);
		nscr=dscr+1;
	}else{
		dscr=0;
		nscr=ScreenCount(dpy);
	}

	/* Initialize */
	if(display!=NULL){
		wglobal.display=scopy(display);
		if(wglobal.display==NULL){
			warn_err();
			XCloseDisplay(dpy);
			return FALSE;
		}
	}
	
	wglobal.dpy=dpy;

	wglobal.conn=ConnectionNumber(dpy);
	wglobal.win_context=XUniqueContext();
	
	wglobal.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
	wglobal.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
	wglobal.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
	wglobal.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wglobal.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wglobal.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
	wglobal.atom_frame_id=XInternAtom(dpy, "_ION_FRAME_ID", False);
	/*wglobal.atom_workspace=XInternAtom(dpy, "_ION_WORKSPACE", False);*/
	wglobal.atom_selection=XInternAtom(dpy, "_ION_SELECTION_STRING", False);
	wglobal.atom_mwm_hints=XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
	
	init_hooks();
	trap_signals();
	load_cursors();	
	init_bindings();
	
	for(i=dscr; i<nscr; i++)
		scr=manage_screen(i);
	
	if(wglobal.screens==NULL){
		if(nscr-dscr>1)
			warn("Could not find a screen to manage.");
		return FALSE;
	}
	
	return TRUE;
}


/*}}}*/


/*{{{ Deinit */


void wmcore_deinit()
{
	Display *dpy;
	WScreen *scr;
	WViewport *vp;
	
	wglobal.opmode=OPMODE_DEINIT;
	
	if(wglobal.dpy==NULL)
		return;
	
	unload_modules();
	
	FOR_ALL_SCREENS(scr){
		/* TODO */
		FOR_ALL_TYPED(scr, vp, WViewport){
			write_workspaces(vp);
		}
		deinit_screen(scr);
	}
	
	dpy=wglobal.dpy;
	wglobal.dpy=NULL;
	
	XCloseDisplay(dpy);
}


/*}}}*/


/*{{{ Misc. */


void pgeom(const char *n, WRectangle g)
{
	fprintf(stderr, "%s %d, %d; %d, %d\n", n, g.x, g.y, g.w, g.h);
}

/*}}}*/

