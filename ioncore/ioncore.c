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
#include "font.h"
#include "extl.h"
#include "errorlog.h"
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


extern bool ioncore_register_exports();
extern void ioncore_unregister_exports();


int main(int argc, char*argv[])
{
	const char *cfgfile=NULL;
	const char *display=NULL;
	const char *msg=NULL;
	char *cmd=NULL;
	bool oneroot=FALSE;
	int opt;
	ErrorLog el;
	FILE *ef=NULL;
	char *efnam=NULL;
	bool may_continue=FALSE;

	libtu_init(argv[0]);

	ioncore_add_default_dirs();
	
	if(!init_module_support())
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
			oneroot=TRUE;
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	wglobal.argc=argc;
	wglobal.argv=argv;
	
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
	
	if(!extl_init())
		goto fail;

	ioncore_register_exports();
	
	if(!read_config_for("ioncorelib"))
		goto fail;
	
	if(!ioncore_init(display, oneroot))
		goto fail;

	if(!ioncore_read_config(cfgfile)){
		/* Let's not fail, it might be a minor error */
		/*
		warn("Unable to load configuration file. Refusing to start. "
			 "You *must* install proper configuration files to use Ion.");
		goto fail;*/
	}
	
	if(!setup_rootwins()){
		warn("Unable to set up any rootwins.");
		goto fail;
	}
	
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
				system(cmd);
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
	
	mainloop();
	
	/* The code should never return here */
	return EXIT_SUCCESS;
}


/*}}}*/

	
/*{{{ Init */


static void init_hooks()
{
	ADD_HOOK(add_clientwin_alt, add_clientwin_default);
	ADD_HOOK(handle_event_alt, handle_event_default);
}


static bool init_classes()
{
	if(!register_region_class(&OBJDESCR(WClientWin), NULL,
							  (WRegionLoadCreateFn*) clientwin_load)){
		return FALSE;
	}
	return TRUE;
}


static void initialize_global()
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
	WFontPtr fnt=load_font(dpy, CF_FALLBACK_FONT_NAME);
	
	if(fnt==NULL){
		warn("Failed to load fallback font \"%s\"", CF_FALLBACK_FONT_NAME);
		return FALSE;
	}
	
	free_font(dpy, fnt);
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


bool ioncore_init(const char *display, bool oneroot)
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
	
	initialize_global();
	
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

	if(oneroot){
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
	
	init_hooks();
	init_classes();
	init_bindings();
	load_cursors();	
	
	for(i=drw; i<nrw; i++)
		manage_rootwin(i);
	
	if(wglobal.rootwins==NULL){
		if(nrw-drw>1)
			warn("Could not find a screen to manage.");
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
	WScreen *scr;
	
	wglobal.opmode=OPMODE_DEINIT;
	
	if(wglobal.dpy==NULL)
		return;
	
	if(wglobal.ws_save_enabled){
		FOR_ALL_SCREENS(scr){
			save_workspaces(scr);
		}
	}
	
	while(wglobal.rootwins!=NULL)
		destroy_obj((WObj*)wglobal.rootwins);

	unload_modules();
	
	dpy=wglobal.dpy;
	wglobal.dpy=NULL;
	
	XCloseDisplay(dpy);
}


/*}}}*/

