/*
 * ion/ioncore/ioncore.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifndef CF_NO_LOCALE
#include <locale.h>
#include <langinfo.h>
#include <libintl.h>
#endif

#include <libtu/util.h>
#include <libtu/optparser.h>
#include <libextl/readconfig.h>
#include <libextl/extl.h>
#include <libmainloop/select.h>
#include <libmainloop/signal.h>
#include <libmainloop/hooks.h>

#include "common.h"
#include "rootwin.h"
#include "event.h"
#include "cursor.h"
#include "global.h"
#include "modules.h"
#include "eventh.h"
#include "ioncore.h"
#include "manage.h"
#include "conf.h"
#include "binding.h"
#include "bindmaps.h"
#include "strings.h"
#include "gr.h"
#include "xic.h"
#include "netwm.h"
#include "focus.h"
#include "frame.h"
#include "saveload.h"
#include "infowin.h"
#include "../version.h"
#include "exports.h"


/*{{{ Variables */


WGlobal ioncore_g;

static const char *progname="ion";

static const char ioncore_copy[]=
    "Ion " ION_VERSION ", copyright (c) Tuomo Valkonen 1999-2005.";

static const char ioncore_license[]=DUMMY_TR(
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Lesser General Public\n"
    "License as published by the Free Software Foundation; either\n"
    "version 2.1 of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n"
    "Lesser General Public License for more details.\n");

static const char *ioncore_about=NULL;

WHook *ioncore_post_layout_setup_hook=NULL;
WHook *ioncore_snapshot_hook=NULL;
WHook *ioncore_deinit_hook=NULL;


/*}}}*/


/*{{{ init_locale */


#ifndef CF_NO_LOCALE


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

    if(strcmp(ctype, "C")==0 || strcmp(ctype, "POSIX")==0)
        return TRUE;
    
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
        ioncore_g.enc_sb=FALSE;
        ioncore_g.enc_utf8=TRUE;
        ioncore_g.use_mb=TRUE;
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
        ioncore_g.use_mb=TRUE;
        return TRUE;
    }

    if(mbtowc(NULL, NULL, 0)!=0){
        warn("Statefull encodings are unsupported.");
        return FALSE;
    }
    
    ioncore_g.enc_sb=FALSE;
    ioncore_g.use_mb=TRUE;

    return TRUE;
}


static bool init_locale()
{
    const char *p;
    
    p=setlocale(LC_ALL, "");
    
    if(p==NULL){
        warn("setlocale() call failed.");
        return FALSE;
    }

    /*if(strcmp(p, "C")==0 || strcmp(p, "POSIX")==0)
        return TRUE;*/

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

#define TEXTDOMAIN "ion3"

static bool init_messages(const char *localedir)
{
    if(bindtextdomain(TEXTDOMAIN, localedir)==NULL){
        warn_err_obj("bindtextdomain");
        return FALSE;
    }else if(textdomain(TEXTDOMAIN)==NULL){
        warn_err_obj("textdomain");
        return FALSE;
    }
    return TRUE;
}


#endif


/*}}}*/


/*{{{ ioncore_init */


#define INIT_HOOK_(NM)                             \
    NM=mainloop_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE;

#define INIT_HOOK(NM, DFLT)                           \
    INIT_HOOK_(NM)                                    \
    if(!hook_add(NM, (void (*)())DFLT)) return FALSE;


static bool init_hooks()
{
    INIT_HOOK_(ioncore_post_layout_setup_hook);
    INIT_HOOK_(ioncore_snapshot_hook);
    INIT_HOOK_(ioncore_deinit_hook);
    INIT_HOOK_(screen_managed_changed_hook);
    INIT_HOOK_(frame_managed_changed_hook);
    INIT_HOOK_(region_activated_hook);
    INIT_HOOK_(region_inactivated_hook);
    INIT_HOOK_(clientwin_mapped_hook);
    INIT_HOOK_(clientwin_unmapped_hook);
    INIT_HOOK(clientwin_do_manage_alt, clientwin_do_manage_default);
    INIT_HOOK(ioncore_handle_event_alt, ioncore_handle_event);
    INIT_HOOK(region_do_warp_alt, region_do_warp_default);
    
    mainloop_sigchld_hook=mainloop_register_hook("ioncore_sigchld_hook",
                                                 create_hook());
    
    return TRUE;
}


static bool register_classes()
{
    int fail=0;
    
    fail|=!ioncore_register_regclass(&CLASSDESCR(WClientWin), 
                                     (WRegionLoadCreateFn*)clientwin_load);
    fail|=!ioncore_register_regclass(&CLASSDESCR(WMPlex), 
                                     (WRegionLoadCreateFn*)mplex_load);
    fail|=!ioncore_register_regclass(&CLASSDESCR(WFrame), 
                                     (WRegionLoadCreateFn*)frame_load);
    fail|=!ioncore_register_regclass(&CLASSDESCR(WInfoWin), 
                                     (WRegionLoadCreateFn*)infowin_load);
    
    return !fail;
}


static void init_global()
{
    /* argc, argv must be set be the program */
    ioncore_g.dpy=NULL;
    ioncore_g.display=NULL;

    ioncore_g.sm_client_id=NULL;
    ioncore_g.rootwins=NULL;
    ioncore_g.screens=NULL;
    ioncore_g.focus_next=NULL;
    ioncore_g.warp_next=FALSE;
    
    ioncore_g.active_screen=NULL;

    ioncore_g.input_mode=IONCORE_INPUTMODE_NORMAL;
    ioncore_g.opmode=IONCORE_OPMODE_INIT;
    ioncore_g.previous_protect=0;
    ioncore_g.dblclick_delay=CF_DBLCLICK_DELAY;
    ioncore_g.opaque_resize=0;
    ioncore_g.warp_enabled=TRUE;
    ioncore_g.switchto_new=TRUE;
    
    ioncore_g.enc_utf8=FALSE;
    ioncore_g.enc_sb=TRUE;
    ioncore_g.use_mb=FALSE;
    
    ioncore_g.screen_notify=TRUE;
}


bool ioncore_init(const char *prog, int argc, char *argv[],
                  const char *localedir)
{
    init_global();
    
    progname=prog;
    ioncore_g.argc=argc;
    ioncore_g.argv=argv;

#ifndef CF_NO_LOCALE    
    init_locale();
    init_messages(localedir);
#endif

    ioncore_about=scat3(ioncore_copy, "\n\n", TR(ioncore_license));
    
    if(!ioncore_init_bindmaps())
        return FALSE;
    
    if(!register_classes())
        return FALSE;

    if(!init_hooks())
        return FALSE;

    if(!ioncore_init_module_support())
        return FALSE;

    return TRUE;
}


/*}}}*/


/*{{{ ioncore_startup */


static void ioncore_init_session(const char *display)
{
    const char *dpyend=NULL;
    char *tmp=NULL, *colon=NULL;
    const char *sm=getenv("SESSION_MANAGER");
    
    if(sm!=NULL)
        ioncore_load_module("mod_sm");

    if(extl_sessiondir()!=NULL)
        return;
    
    /* Not running under SM; use display-specific directory */
    dpyend=strchr(display, ':');
    if(dpyend!=NULL)
        dpyend=strchr(dpyend, '.');
    if(dpyend==NULL){    
        libtu_asprintf(&tmp, "default-session-%s", display);
    }else{
        libtu_asprintf(&tmp, "default-session-%.*s",
                       (int)(dpyend-display), display);
    }
    
    if(tmp==NULL)
        return;
    
    colon=tmp;
    while(1){
        colon=strchr(colon, ':');
        if(colon==NULL)
            break;
        *colon='-';
    }
    
    extl_set_sessiondir(tmp);
    free(tmp);
}
    

static bool ioncore_init_x(const char *display, int stflags)
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
        warn(TR("Could not connect to X display '%s'"), 
             XDisplayName(display));
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
        ioncore_g.display=scopy(display);
        if(ioncore_g.display==NULL){
            XCloseDisplay(dpy);
            return FALSE;
        }
    }
    
    ioncore_g.dpy=dpy;
    ioncore_g.win_context=XUniqueContext();
    ioncore_g.conn=ConnectionNumber(dpy);
    
    fcntl(ioncore_g.conn, F_SETFD, FD_CLOEXEC);
    
    ioncore_g.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
    ioncore_g.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
    ioncore_g.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
    ioncore_g.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    ioncore_g.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    ioncore_g.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    ioncore_g.atom_wm_window_role=XInternAtom(dpy, "WM_WINDOW_ROLE", False);
    ioncore_g.atom_checkcode=XInternAtom(dpy, "_ION_CWIN_RESTART_CHECKCODE", False);
    ioncore_g.atom_selection=XInternAtom(dpy, "_ION_SELECTION_STRING", False);
    ioncore_g.atom_mwm_hints=XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

    ioncore_init_xim();
    ioncore_init_bindings();
    ioncore_init_cursors();

    netwm_init();
    
    ioncore_init_session(XDisplayName(display));

    for(i=drw; i<nrw; i++)
        ioncore_manage_rootwin(i, stflags&IONCORE_STARTUP_NOXINERAMA);

    if(ioncore_g.rootwins==NULL){
        if(nrw-drw>1)
            warn(TR("Could not find a screen to manage."));
        return FALSE;
    }

    if(!mainloop_register_input_fd(ioncore_g.conn, NULL,
                                   ioncore_x_connection_handler)){
        return FALSE;
    }
    
    return TRUE;
}


static void set_initial_focus()
{
    Window root=None, win=None;
    int x, y, wx, wy;
    uint mask;
    WScreen *scr;
    WWindow *wwin;
    
    XQueryPointer(ioncore_g.dpy, None, &root, &win,
                  &x, &y, &wx, &wy, &mask);
    
    FOR_ALL_SCREENS(scr){
        Window scrroot=region_root_of((WRegion*)scr);
        if(scrroot==root && rectangle_contains(&REGION_GEOM(scr), x, y)){
            break;
        }
    }
    
    if(scr==NULL)
        scr=ioncore_g.screens;
    
    ioncore_g.active_screen=scr;
    region_do_set_focus((WRegion*)scr, FALSE);
}


bool ioncore_startup(const char *display, const char *cfgfile,
                     int stflags)
{
    WRootWin *rootwin;

    if(!extl_init())
        return FALSE;

    ioncore_register_exports();
    
    if(!ioncore_init_x(display, stflags))
        return FALSE;

    mainloop_trap_timer();
    
    gr_read_config();

    if(!extl_read_config("ioncore_ext", NULL, TRUE))
        return FALSE;
    
    ioncore_read_main_config(cfgfile);
    
    if(!ioncore_init_layout())
        return FALSE;
    
    hook_call_v(ioncore_post_layout_setup_hook);
    
    FOR_ALL_ROOTWINS(rootwin)
        rootwin_manage_initial_windows(rootwin);
    
    set_initial_focus();
    
    return TRUE;
}


/*}}}*/


/*{{{ ioncore_deinit */


void ioncore_deinit()
{
    Display *dpy;
    WRootWin *rootwin;
    
    ioncore_g.opmode=IONCORE_OPMODE_DEINIT;
    
    if(ioncore_g.dpy==NULL)
        return;

    hook_call_v(ioncore_deinit_hook);

    while(ioncore_g.screens!=NULL)
        destroy_obj((Obj*)ioncore_g.screens);

    /*ioncore_unload_modules();*/

    while(ioncore_g.rootwins!=NULL)
        destroy_obj((Obj*)ioncore_g.rootwins);

    ioncore_deinit_bindmaps();

    mainloop_unregister_input_fd(ioncore_g.conn);
    
    dpy=ioncore_g.dpy;
    ioncore_g.dpy=NULL;
    
    XSync(dpy, True);
    XCloseDisplay(dpy);
    
    extl_deinit();
}


/*}}}*/


/*{{{ Miscellaneous */


/*EXTL_DOC
 * Is Ion supporting locale-specifically multibyte-encoded strings?
 */
EXTL_SAFE
EXTL_EXPORT
bool ioncore_is_i18n()
{
    return ioncore_g.use_mb;
}


/*EXTL_DOC
 * Returns Ioncore version string.
 */
EXTL_SAFE
EXTL_EXPORT
const char *ioncore_version()
{
    return ION_VERSION;
}

/*EXTL_DOC
 * Returns the name of program using Ioncore.
 */
EXTL_SAFE
EXTL_EXPORT
const char *ioncore_progname()
{
    return progname;
}


/*EXTL_DOC
 * Returns an about message (version, author, copyright notice).
 */
EXTL_SAFE
EXTL_EXPORT
const char *ioncore_aboutmsg()
{
    return ioncore_about;
}


/*}}}*/

