/*
 * ion/ioncore/init.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <libtu/util.h>
#include <libtu/optparser.h>

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
#include "wsreg.h"
#include "funtabs.h"
#include "eventh.h"
#include "saveload.h"
#include "ioncore.h"
#include "exec.h"
#include "conf.h"
#include "../version.h"


/*{{{ Global variables */


WGlobal wglobal;


/*}}}*/


/*{{{ Optparser data */


/* Options. Getopt is not used because getopt_long is quite gnu-specific
 * and they don't know of '-display foo' -style args anyway.
 * Instead, I've reinvented the wheel in libtu :(.
 */
static OptParserOpt ioncore_opts[]={
	{OPT_ID('d'), 	"display", 	OPT_ARG, "host:dpy.scr", "X display to use"},
	{'c', 			"conffile", OPT_ARG, "config_file", "Configuration file"},
	{OPT_ID('o'), 	"onescreen", 0, NULL, "Manage default screen only"},
	{OPT_ID('c'), 	"confdir", 	OPT_ARG, "dir", "Search directory for configuration files"},
	{OPT_ID('l'), 	"libdir", 	OPT_ARG, "dir", "Search directory for modules"},
	END_OPTPARSEROPTS
};


static const char ioncore_usage_tmpl[]=
	"Usage: $p [options]\n\n$o\n";


static const char ioncore_about[]=
	"Ioncore " ION_VERSION ", copyright (c) Tuomo Valkonen 1999-2003.\n"
	"This program may be copied and modified under the terms of the "
	"Clarified Artistic License.\n";


static OptParserCommonInfo ioncore_cinfo={
	ION_VERSION,
	ioncore_usage_tmpl,
	ioncore_about
};


/*}}}*/


/*{{{ Main */


int main(int argc, char*argv[])
{
	const char *cfgfile=NULL;
	const char *display=NULL;
	const char *msg=NULL;
	char *cmd=NULL;
	bool onescreen=FALSE;
	int opt;
	
	libtu_init(argv[0]);
	
	optparser_init(argc, argv, OPTP_MIDLONG, ioncore_opts, &ioncore_cinfo);
	
	while((opt=optparser_get_opt())){
		switch(opt){
		case OPT_ID('d'):
			display=optparser_get_arg();
			break;
		case 'c':
			cfgfile=optparser_get_arg();
			break;
		case OPT_ID('c'):
			ioncore_add_confdir(optparser_get_arg());
			break;
		case OPT_ID('l'):
			ioncore_add_libdir(optparser_get_arg());
			break;
		case OPT_ID('o'):
			onescreen=TRUE;
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	wglobal.argc=argc;
	wglobal.argv=argv;
	
	/*ioncore_add_default_dirs("ion-devel", ETCDIR, LIBDIR);*/
	
	if(!ioncore_init(display, onescreen))
		return EXIT_FAILURE;
	
	if(!ioncore_read_config(cfgfile)){
		msg="Unable to load configuration file. Refusing to start. "
			"You *must* install proper configuration files "
			"either in ~/.ion-devel or "ETCDIR"/ion-devel to use Ion.";
		goto fail;
	}
	
	if(!setup_screens()){
		msg="Unable to set up any screens. Refusing to start.";
		goto fail;
	}
	
	mainloop();
	
	/* The code should never return here */
	return EXIT_SUCCESS;
	
fail:
	warn(msg);
	setup_environ(DefaultScreen(wglobal.dpy));
	XCloseDisplay(wglobal.dpy);
	libtu_asprintf(&cmd, "xmessage '%s'", msg);
	if(cmd!=NULL)
		wm_do_exec(cmd);
	return EXIT_FAILURE;
}


/*}}}*/

	
/*{{{ Init */


static void init_hooks()
{
	ADD_HOOK(add_clientwin_alt, add_clientwin_default);
	ADD_HOOK(handle_event_alt, handle_event_default);
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

	wglobal.input_mode=INPUT_NORMAL;
	wglobal.opmode=OPMODE_INIT;
	wglobal.previous_protect=0;
	wglobal.dblclick_delay=CF_DBLCLICK_DELAY;
	wglobal.resize_delay=CF_RESIZE_DELAY;
	wglobal.opaque_resize=0;
	wglobal.warp_enabled=TRUE;
	
	wglobal.grab_released=FALSE;
}


bool ioncore_init(const char *display, bool onescreen)
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
    
	/* Try to set up locales. If X does not support user-specified locale,
	 * reset to POSIX so that at least fonts will be loadable if not all
	 * characters supported.
	 */
#ifdef CF_UTF8
	if(setlocale(LC_ALL, "")==NULL)
		warn("setlocale() call failed");
	if(!XSupportsLocale()){
		warn("XSupportLocale() failed. Resetting locale to \"POSIX\".");
		if(setlocale(LC_ALL, "POSIX")==NULL){
			warn("setlocale() call failed");
			return FALSE;
		}
		if(!XSupportsLocale()){
			warn("XSupportLocale still failed. Refusing to resume.");
			return FALSE;
		}
	}
#endif
	
	initialize_global();
	ioncore_init_funclists();
	
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
	wglobal.atom_wm_window_role=XInternAtom(dpy, "WM_WINDOW_ROLE", False);
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


void ioncore_deinit()
{
	Display *dpy;
	WScreen *scr;
	WViewport *vp;
	
	wglobal.opmode=OPMODE_DEINIT;
	
	if(wglobal.dpy==NULL)
		return;
	
	FOR_ALL_SCREENS(scr){
		FOR_ALL_TYPED(scr, vp, WViewport){
			save_workspaces(vp);
		}
		destroy_thing((WThing*)scr);
	}

	/*unload_modules();*/
	
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

