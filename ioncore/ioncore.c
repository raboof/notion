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
#include <langinfo.h>
#include <sys/types.h>

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
#include "xic.h"
#include "netwm.h"
#include "../version.h"


/*{{{ Variables */


WGlobal wglobal;


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
    "Lesser General Public License for more details.\n";


/*}}}*/


/*{{{ ioncore_init */


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
	
	wglobal.enc_utf8=FALSE;
	wglobal.enc_sb=TRUE;
	wglobal.use_mb=FALSE;
}


bool ioncore_init(int argc, char *argv[])
{
	init_global();
	
	wglobal.argc=argc;
	wglobal.argv=argv;

	register_classes();
	init_hooks();

	return init_module_support();
}


/*}}}*/


/*{{{ ioncore_init_i18n */


static bool check_encoding()
{
	int i;
	char chs[8]=" ";
	wchar_t wc;
	const char *langi, *ctype, *a, *b;

	langi=nl_langinfo(CODESET);
	ctype=setlocale(LC_CTYPE, NULL);
	
	if(langi==NULL || ctype==NULL){
		warn("Cannot verify locale encoding setting integrity "
			 "(LC_CTYPE=%s, nl_langinfo(CODESET)=%s).", ctype, langi);
		return FALSE;
	}

	a=langi; 
	b=strchr(ctype, '.');
	if(b!=NULL){
		b=b+1;
		while(*a==*b && *a!='\0'){
			a++;
			b++;
		}
	}
	
	if(b==NULL || (*a!='\0' || (*a=='\0' && *b!='\0' && *b!='@'))){
		warn("Encoding in LC_CTYPE (%s) and encoding reported by "
			 "nl_langinfo(CODESET) (%s) do not match. ", ctype, langi);
		return FALSE;
	}
		
	if(strcmp(langi, "UTF-8")==0 || strcmp(langi, "UTF8")==0){
		wglobal.enc_sb=FALSE;
		wglobal.enc_utf8=TRUE;
		wglobal.use_mb=TRUE;
		return TRUE;
	}
	
	for(i=0; i<256; i++){
		chs[0]=i;
		if(mbtowc(&wc, chs, 8)==-1){
			/* Doesn't look like a single-byte encoding. */
			break;
		}
		
	}
	
	if(i==256){
		/* Seems like a single-byte encoding... */
		wglobal.use_mb=TRUE;
		return TRUE;
	}

	if(mbtowc(NULL, NULL, 0)!=0){
		warn("Statefull encodings are unsupported.");
		return FALSE;
	}
	
	wglobal.enc_sb=FALSE;
	wglobal.use_mb=TRUE;
	
	return TRUE;
}


bool ioncore_init_i18n()
{
	const char *p;
	
	p=setlocale(LC_ALL, "");
	
	if(p==NULL){
		warn("setlocale() call failed.");
		return FALSE;
	}

	if(strcmp(p, "C")==0 || strcmp(p, "POSIX")==0)
		return TRUE;
	
	if(!XSupportsLocale()){
		warn("XSupportsLocale() failed.");
	}else{
		if(check_encoding())
			return TRUE;
	}
	
	warn("Reverting locale settings to \"C\".");
	
	if(setlocale(LC_ALL, "C")==NULL)
		warn("setlocale() call failed.");
		
	return FALSE;
}


/*}}}*/


/*{{{ ioncore_startup */


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
	
	ioncore_set_sessiondir(tmp);
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

	/* Open the display. */
	dpy=XOpenDisplay(display);
	
	if(dpy==NULL){
		warn("Could not connect to X display '%s'", XDisplayName(display));
		return FALSE;
	}

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
	
	init_xim();
	
	wglobal.conn=ConnectionNumber(dpy);
	wglobal.win_context=XUniqueContext();
	
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

	netwm_init();
	
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


bool ioncore_startup(const char *display, const char *cfgfile,
					 int stflags)
{
	if(!extl_init())
		return FALSE;

	ioncore_register_exports();
	
	if(!read_config("ioncorelib"))
		return FALSE;
	
	if(!init_x(display, stflags))
		return FALSE;

	gr_read_config();

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


/*{{{ ioncore_deinit */


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
	
	XSync(dpy, True);
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
 * Is Ion supporting locale-specifically multibyte-encoded strings?
 */
EXTL_EXPORT
bool ioncore_is_i18n()
{
	return wglobal.use_mb;
}


/*EXTL_DOC
 * Returns Ioncore version string.
 */
EXTL_EXPORT
const char *ioncore_version()
{
	return ION_VERSION;
}


/*EXTL_DOC
 * Returns an about message (version, author, copyright notice).
 */
EXTL_EXPORT
const char *ioncore_aboutmsg()
{
	return ioncore_about;
}

/*}}}*/

