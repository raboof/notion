/*
 * ion/ioncore/init.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libtu/util.h>
#include <libtu/optparser.h>

#include "common.h"
#include "rootwin.h"
#include "event.h"
#include "cursor.h"
#include "signal.h"
#include "readconfig.h"
#include "global.h"
#include "modules.h"
#include "eventh.h"
#include "saveload.h"
#include "ioncore.h"
#include "manage.h"
#include "exec.h"
#include "conf.h"
#include "binding.h"
#include "strings.h"
#include "extl.h"
#include "errorlog.h"
#include "gr.h"
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
	{OPT_ID('o'), 	"oneroot",  0, NULL, "Manage default root window/non-Xinerama screen only"},
	{OPT_ID('c'), 	"confdir", 	OPT_ARG, "dir", "Search directory for configuration files"},
	{OPT_ID('l'), 	"moduledir", OPT_ARG, "dir", "Search directory for modules"},
#ifndef CF_NOXINERAMA	
	{OPT_ID('x'), 	"noxinerama", 0, NULL, "Ignore Xinerama screen information"},
#else
	{OPT_ID('x'), 	"noxinerama", 0, NULL, "Ignored: not compiled with Xinerama support"},
#endif
	{OPT_ID('s'),   "sessionname", OPT_ARG, "session_name", "Name of session (affects savefiles)"},
	END_OPTPARSEROPTS
};


static const char ioncore_usage_tmpl[]=
	"Usage: $p [options]\n\n$o\n";


static const char ioncore_about[]=
    "Ion " ION_VERSION ", copyright (c) Tuomo Valkonen 1999-2003.\n"
    "\n"
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Lesser General Public\n"
    "License as published by the Free Software Foundation; either\n"
    "version 2.1 of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n"
    "Lesser General Public License for more details.\n"
    "\n";


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
	int stflags=0;
	int opt;
	ErrorLog el;
	FILE *ef=NULL;
	char *efnam=NULL;
	bool may_continue=FALSE;

	libtu_init(argv[0]);

	if(!ioncore_init(argc, argv))
		return EXIT_FAILURE;
	
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
			ioncore_add_scriptdir(optparser_get_arg());
			break;
		case OPT_ID('l'):
			ioncore_add_moduledir(optparser_get_arg());
			break;
		case OPT_ID('o'):
			stflags|=IONCORE_STARTUP_ONEROOT;
			break;
		case OPT_ID('x'):
			stflags|=IONCORE_STARTUP_NOXINERAMA;
			break;
		case OPT_ID('s'):
			ioncore_set_sessiondir("ion-devel", optparser_get_arg());
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	/* We may have to pass the file to xmessage so just using tmpfile()
	 * isn't sufficient */
	
	libtu_asprintf(&efnam, "%s/ion-%d-startup-errorlog", P_tmpdir,
				   getpid());
	if(efnam==NULL){
		warn_err("Failed to create error log file");
	}else{
		ef=fopen(efnam, "wt");
		if(ef==NULL){
			warn_err_obj(efnam);
			free(efnam);
			efnam=NULL;
		}
		fprintf(ef, "Ion startup error log:\n");
		begin_errorlog_file(&el, ef);
	}
	
	if(ioncore_startup(display, cfgfile, stflags))
		may_continue=TRUE;

fail:
	if(!may_continue)
		warn("Refusing to start due to encountered errors.");
	
	if(ef!=NULL){
		pid_t pid=-1;
		if(end_errorlog(&el)){
			fclose(ef);
			pid=fork();
			if(pid==0){
				setup_environ(DefaultScreen(wglobal.dpy));
				if(!may_continue)
					XCloseDisplay(wglobal.dpy);
				else
					close(wglobal.conn);
				libtu_asprintf(&cmd, CF_XMESSAGE " %s", efnam);
				if(system(cmd)==-1)
					warn_err_obj(cmd);
				unlink(efnam);
				exit(EXIT_SUCCESS);
			}
			if(!may_continue && pid>0)
				waitpid(pid, NULL, 0);
		}else{
			fclose(ef);
		}
		if(pid<0)
			unlink(efnam);
		free(efnam);
	}

	if(!may_continue)
		return EXIT_FAILURE;
	
	ioncore_mainloop();
	
	/* The code should never return here */
	return EXIT_SUCCESS;
}


/*}}}*/

	
/*{{{ Init */


extern bool ioncore_register_exports();
extern void ioncore_unregister_exports();


static void init_hooks()
{
	ADD_HOOK(add_clientwin_alt, add_clientwin_default);
	ADD_HOOK(handle_event_alt, handle_event_default);
}


static bool register_classes()
{
	if(!register_region_class(&OBJDESCR(WClientWin), NULL,
							  (WRegionLoadCreateFn*) clientwin_load)){
		return FALSE;
	}
	return TRUE;
}


static void init_global()
{
	WRectangle zero_geom={0, 0, 0, 0};
	
	/* argc, argv must be set be the program */
	wglobal.dpy=NULL;
	wglobal.display=NULL;

	wglobal.rootwins=NULL;
	wglobal.screens=NULL;
	wglobal.focus_next=NULL;
	wglobal.warp_next=FALSE;
	wglobal.cwin_list=NULL;
	
	wglobal.active_screen=NULL;

	wglobal.input_mode=INPUT_NORMAL;
	wglobal.opmode=OPMODE_INIT;
	wglobal.previous_protect=0;
	wglobal.dblclick_delay=CF_DBLCLICK_DELAY;
	wglobal.resize_delay=CF_RESIZE_DELAY;
	wglobal.opaque_resize=0;
	wglobal.warp_enabled=TRUE;
	wglobal.ws_save_enabled=TRUE;
}


#ifdef CF_UTF8
static bool test_fallback_font(Display *dpy)
{
	XFontStruct *fnt=XLoadQueryFont(dpy, CF_FALLBACK_FONT_NAME);
	
	if(fnt==NULL){
		warn("Failed to load fallback font \"%s\"", CF_FALLBACK_FONT_NAME);
		return FALSE;
	}
	
	XFreeFont(dpy, fnt);
	return TRUE;
}


static bool set_up_locales(Display *dpy)
{
	bool tryno=0;
	
	if(setlocale(LC_ALL, "")==NULL)
		warn("setlocale() call failed");

	while(1){
		if(XSupportsLocale()){
			if(test_fallback_font(dpy)){
				if(tryno==1){
					warn("This seems to work but support for non-ASCII "
						 "characters will be crippled. Please set up your "
						 "locales properly.");
				}
				return TRUE;
			}
		}else{
			warn("XSupportsLocale() failed%s",
				 tryno==0 ? " for your locale settings." : "");
		}
		
		if(tryno==1)
			return FALSE;
		
		warn("Resetting locale to \"POSIX\".");
		if(setlocale(LC_ALL, "POSIX")==NULL){
			warn("setlocale() call failed");
		}
		tryno++;
	}
}
#endif


static void set_session(const char *display)
{
	const char *dpyend;
	char *tmp, *colon;
	
	dpyend=strchr(display, ':');
	if(dpyend!=NULL)
		dpyend=strchr(dpyend, '.');
	if(dpyend==NULL){	
		libtu_asprintf(&tmp, "default-session-%s", display);
	}else{
		libtu_asprintf(&tmp, "default-session-%.*s",
					   (int)(dpyend-display), display);
	}

	if(tmp==NULL){
		warn_err();
		return;
	}

	colon=tmp;
	while(1){
		colon=strchr(colon, ':');
		if(colon==NULL)
			break;
		*colon='-';
	}
	
	ioncore_set_sessiondir("ion-devel", tmp);
	free(tmp);
}
	

static bool init_x(const char *display, int stflags)
{
	Display *dpy;
	int i, drw, nrw;
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
	
	/* Open the display. */
	dpy=XOpenDisplay(display);
	
	if(dpy==NULL){
		warn("Could not connect to X display '%s'", XDisplayName(display));
		return FALSE;
	}

#ifdef CF_UTF8
	if(!set_up_locales(dpy)){
		warn("There's something wrong with your system. Attempting to "
			 "continue nevertheless, but expect problems.");
	}
#endif

	if(stflags&IONCORE_STARTUP_ONEROOT){
		drw=DefaultScreen(dpy);
		nrw=drw+1;
	}else{
		drw=0;
		nrw=ScreenCount(dpy);
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
	
	set_session(XDisplayName(display));
	
	wglobal.dpy=dpy;
	
	wglobal.conn=ConnectionNumber(dpy);
	wglobal.win_context=XUniqueContext();
	
	wglobal.atom_net_wm_name=XInternAtom(dpy, "_NET_WM_NAME", False);
	wglobal.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
	wglobal.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
	wglobal.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
	wglobal.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wglobal.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wglobal.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
	wglobal.atom_wm_window_role=XInternAtom(dpy, "WM_WINDOW_ROLE", False);
	wglobal.atom_checkcode=XInternAtom(dpy, "_ION_CWIN_RESTART_CHECKCODE", False);
	wglobal.atom_selection=XInternAtom(dpy, "_ION_SELECTION_STRING", False);
	wglobal.atom_mwm_hints=XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
	
	init_bindings();
	load_cursors();	
	
	for(i=drw; i<nrw; i++)
		manage_rootwin(i, stflags&IONCORE_STARTUP_NOXINERAMA);

	if(wglobal.rootwins==NULL){
		if(nrw-drw>1)
			warn("Could not find a screen to manage.");
		return FALSE;
	}
	
	return TRUE;
}


bool ioncore_init(int argc, char *argv[])
{
	init_global();
	
	wglobal.argc=argc;
	wglobal.argv=argv;

	register_classes();
	init_hooks();

	if(!init_module_support())
		return FALSE;

	ioncore_add_default_dirs();
	
	return TRUE;
}


bool ioncore_startup(const char *display, const char *cfgfile,
					 int stflags)
{
	if(!extl_init())
		return FALSE;

	ioncore_register_exports();
	
	if(!read_config_for("ioncorelib"))
		return FALSE;
	
	if(!init_x(display, stflags))
		return FALSE;

	reread_draw_config();

	if(!ioncore_read_config(cfgfile)){
		/* Let's not fail, it might be a minor error */
	}
	
	if(!setup_rootwins()){
		warn("Unable to set up any rootwins.");
		return FALSE;
	}
	
	trap_signals();
	
	return TRUE;
}


/*}}}*/


/*{{{ Deinit */


void ioncore_deinit()
{
	Display *dpy;
	WRootWin *rootwin;
	
	wglobal.opmode=OPMODE_DEINIT;
	
	if(wglobal.dpy==NULL)
		return;
	
	extl_call_named("call_hook", "s", NULL, "deinit");
					
	if(wglobal.ws_save_enabled){
		save_workspaces();
	}else{
		warn("Not saving workspace layout.");
	}
	
	while(wglobal.screens!=NULL)
		destroy_obj((WObj*)wglobal.screens);

	unload_modules();

	while(wglobal.rootwins!=NULL)
		destroy_obj((WObj*)wglobal.rootwins);
	
	dpy=wglobal.dpy;
	wglobal.dpy=NULL;
	
	XCloseDisplay(dpy);
	
	extl_deinit();
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Issue a warning. How the message is displayed depends on the current
 * warning handler.
 */
EXTL_EXPORT_AS(warn)
void exported_warn(const char *str)
{
	warn("%s", str);
}


/*EXTL_DOC
 * Was Ioncore compiled to use UTF8 strings internally?
 */
EXTL_EXPORT
bool ioncore_is_utf8()
{
#ifdef CF_UTF8
	return TRUE;
#else
	return FALSE;
#endif
}


/*}}}*/

