/*
 * ion/ioncore/init.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <sys/types.h>
#ifndef CF_NO_MB_SUPPORT
#include <locale.h>
#include <langinfo.h>
#endif

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
#include "mainloop.h"
#include "eventh.h"
#include "saveload.h"
#include "ioncore.h"
#include "manage.h"
#include "conf.h"
#include "binding.h"
#include "bindmaps.h"
#include "strings.h"
#include "extl.h"
#include "gr.h"
#include "xic.h"
#include "netwm.h"
#include "focus.h"
#include "frame.h"
#include "../version.h"


/*{{{ Variables */


WGlobal ioncore_g;


static const char ioncore_about[]=
    "Ion " ION_VERSION ", copyright (c) Tuomo Valkonen 1999-2004.\n"
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


WHooklist *ioncore_post_layout_setup_hook=NULL;


/*}}}*/


/*{{{ ioncore_init */


extern bool ioncore_register_exports();
extern void ioncore_unregister_exports();


static void init_hooks()
{
    ADD_HOOK(clientwin_do_manage_alt, clientwin_do_manage_default);
    ADD_HOOK(ioncore_handle_event_alt, ioncore_handle_event);
    ADD_HOOK(region_do_warp_alt, region_do_warp_default);
}


static bool register_classes()
{
    int fail=0;
    
    fail|=!ioncore_register_regclass(&CLASSDESCR(WClientWin), NULL,
                                     (WRegionLoadCreateFn*)clientwin_load);
    fail|=!ioncore_register_regclass(&CLASSDESCR(WMPlex), NULL,
                                     (WRegionLoadCreateFn*)mplex_load);
    fail|=!ioncore_register_regclass(&CLASSDESCR(WFrame), NULL,
                                     (WRegionLoadCreateFn*)frame_load);
    
    return !fail;
}


static void init_global()
{
    WRectangle zero_geom={0, 0, 0, 0};
    
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
    ioncore_g.resize_delay=CF_RESIZE_DELAY;
    ioncore_g.opaque_resize=0;
    ioncore_g.warp_enabled=TRUE;
    ioncore_g.save_enabled=TRUE;
    ioncore_g.switchto_new=TRUE;
    
    ioncore_g.enc_utf8=FALSE;
    ioncore_g.enc_sb=TRUE;
    ioncore_g.use_mb=FALSE;
}


bool ioncore_init(int argc, char *argv[])
{
    init_global();
    
    ioncore_g.argc=argc;
    ioncore_g.argv=argv;

    if(!ioncore_init_bindmaps())
        return FALSE;
    if(!register_classes())
        return FALSE;
    init_hooks();

    return ioncore_init_module_support();
}


/*}}}*/


/*{{{ ioncore_init_i18n */


static bool check_encoding()
{
#ifdef CF_NO_MB_SUPPORT
    return TRUE;
#else
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
#endif    
    return TRUE;
}


bool ioncore_init_i18n()
{
#ifdef CF_NO_MB_SUPPORT
    warn("Can't enable i18n support: compiled with CF_NO_MB_SUPPORT.");
    return FALSE;
#else
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
#endif
}


/*}}}*/


/*{{{ ioncore_startup */


static void set_session(const char *display)
{
    const char *dpyend;
    char *tmp, *colon;
    const char *smdir, *sm;
    
    sm=getenv("SESSION_MANAGER");
    smdir=getenv("SM_SAVE_DIR");
    
    if(sm!=NULL && smdir!=NULL){
        /* Probably running under a session manager; use its
         * save file directory. (User should also load mod_sm.)
         */
        libtu_asprintf(&tmp, "%s/ion3", smdir); /* !!! pwm<=>ion */
        if(tmp==NULL){
            warn_err();
            return;
        }
    }else{
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
    }
    
    ioncore_set_sessiondir(tmp);
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
        ioncore_g.display=scopy(display);
        if(ioncore_g.display==NULL){
            warn_err();
            XCloseDisplay(dpy);
            return FALSE;
        }
    }
    
    if(ioncore_sessiondir()==NULL)
        set_session(XDisplayName(display));
    
    ioncore_g.dpy=dpy;
    
    ioncore_g.conn=ConnectionNumber(dpy);
    ioncore_g.win_context=XUniqueContext();
    
    ioncore_g.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
    ioncore_g.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
    ioncore_g.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
    ioncore_g.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    ioncore_g.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    ioncore_g.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    ioncore_g.atom_wm_window_role=XInternAtom(dpy, "WM_WINDOW_ROLE", False);
    ioncore_g.atom_checkcode=XInternAtom(dpy, "_ION_CWIN_RESTART_CHECKCODE", False);
    ioncore_g.atom_selection=XInternAtom(dpy, "_ION_SELECTION_STRING", False);
    ioncore_g.atom_kludges=XInternAtom(dpy, "_ION_KLUDGES", False);
    ioncore_g.atom_mwm_hints=XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

    ioncore_init_xim();
    ioncore_init_bindings();
    ioncore_init_cursors();

    netwm_init();
    
    for(i=drw; i<nrw; i++)
        ioncore_manage_rootwin(i, stflags&IONCORE_STARTUP_NOXINERAMA);

    if(ioncore_g.rootwins==NULL){
        if(nrw-drw>1)
            warn("Could not find a screen to manage.");
        return FALSE;
    }

    if(!ioncore_register_input_fd(ioncore_g.conn, NULL,
                                  ioncore_x_connection_handler)){
        warn("Unable to register X connection FD");
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
    
    ioncore_trap_signals();

    if(!ioncore_init_x(display, stflags))
        return FALSE;

    gr_read_config();

    if(!ioncore_read_config("ioncorelib", NULL, TRUE))
        return FALSE;
    
    ioncore_read_main_config(cfgfile);
    
    if(!ioncore_init_layout()){
        warn("Unable to set up layout on any screen.");
        return FALSE;
    }
    
    CALL_HOOKS(ioncore_post_layout_setup_hook, ());
    
    FOR_ALL_ROOTWINS(rootwin)
        rootwin_manage_initial_windows(rootwin);
    
    set_initial_focus();
    
    return TRUE;
}


/*}}}*/


/*{{{ ioncore_save_session */


WHooklist *ioncore_save_session_hook=NULL;


/*EXTL_DOC
 * Save session state.
 */
EXTL_EXPORT
bool ioncore_save_session()
{
    if(!ioncore_save_layout())
        return FALSE;

    CALL_HOOKS(ioncore_save_session_hook, ());
              
    extl_call_named("call_hook", "s", NULL, "save_session");
    
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

    if(ioncore_g.save_enabled)
        ioncore_save_session();

    extl_call_named("call_hook", "s", NULL, "deinit");
    
    while(ioncore_g.screens!=NULL)
        destroy_obj((Obj*)ioncore_g.screens);

    ioncore_unload_modules();

    while(ioncore_g.rootwins!=NULL)
        destroy_obj((Obj*)ioncore_g.rootwins);

    ioncore_deinit_bindmaps();

    ioncore_unregister_input_fd(ioncore_g.conn);
    
    dpy=ioncore_g.dpy;
    ioncore_g.dpy=NULL;
    
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
EXTL_EXPORT
void ioncore_warn(const char *str)
{
    warn("%s", str);
}


/*EXTL_DOC
 * Is Ion supporting locale-specifically multibyte-encoded strings?
 */
EXTL_EXPORT
bool ioncore_is_i18n()
{
    return ioncore_g.use_mb;
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

