/*
 * ion/ioncore/ioncore.c
 *
 * Copyright (c) The Notion Team 2011. 
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <ctype.h>
#ifndef CF_NO_LOCALE
#include <locale.h>
#include <langinfo.h>
#endif
#ifndef CF_NO_GETTEXT
#include <libintl.h>
#endif
#include <stdarg.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xext.h>

#include <libtu/util.h>
#include <libtu/optparser.h>
#include <libextl/readconfig.h>
#include <libextl/extl.h>
#include <libmainloop/select.h>
#include <libmainloop/signal.h>
#include <libmainloop/hooks.h>
#include <libmainloop/exec.h>

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
#include "activity.h"
#include "group-cw.h"
#include "group-ws.h"
#include "llist.h"
#include "exec.h"
#include "screen-notify.h"
#include "key.h"
#include "log.h"


#include "../version.h"
#include "exports.h"


/*{{{ Variables */


WGlobal ioncore_g;

static const char *progname="notion";

static const char ioncore_copy[]=
    "Notion " NOTION_VERSION ", see the README for copyright details.";

static const char ioncore_license[]=DUMMY_TR(
    "This software is licensed under the GNU Lesser General Public License\n"
    "(LGPL), version 2.1, extended with terms applying to the use of the\n"
    "former name of the project, Ion(tm), unless otherwise indicated in\n"
    "components taken from elsewhere. For details, see the file LICENSE\n"
    "that you should have received with this software.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"); 

static const char *ioncore_about=NULL;

WHook *ioncore_post_layout_setup_hook=NULL;
WHook *ioncore_snapshot_hook=NULL;
WHook *ioncore_deinit_hook=NULL;


/*}}}*/


/*{{{ warn_nolog */


void ioncore_warn_nolog(const char *str, ...)
{
    va_list args;
    
    va_start(args, str);
    
    if(ioncore_g.opmode==IONCORE_OPMODE_INIT){
        fprintf(stderr, "%s: ", libtu_progname());
        vfprintf(stderr, str, args);
        fprintf(stderr, "\n");
    }else{
        warn_v(str, args);
    }
    
    va_end(args);
}

/*}}}*/


/*{{{ init_locale */


#ifndef CF_NO_LOCALE


static bool check_encoding()
{
    int i;
    char chs[8]=" ";
    wchar_t wc;
    const char *langi, *ctype, *a, *b;
    bool enc_check_ok=FALSE;

    langi=nl_langinfo(CODESET);
    ctype=setlocale(LC_CTYPE, NULL);
    
    if(langi==NULL || ctype==NULL)
        goto integr_err;

    if(strcmp(ctype, "C")==0 || strcmp(ctype, "POSIX")==0)
        return TRUE;
    
    /* Compare encodings case-insensitively, ignoring dashes (-) */
    a=langi; 
    b=strchr(ctype, '.');
    if(b!=NULL){
        b=b+1;
        while(1){
            if(*a=='-'){
                a++;
                continue;
            }
            if(*b=='-'){
                b++;
                continue;
            }
            if(*b=='\0' || *b=='@'){
                enc_check_ok=(*a=='\0');
                break;
            }
            if(*a=='\0' || tolower(*a)!=tolower(*b))
                break;
            a++;
            b++;
        }
        if(!enc_check_ok)
            goto integr_err;
    }else{
        ioncore_warn_nolog(TR("No encoding given in LC_CTYPE."));
    }
        
    if(strcasecmp(langi, "UTF-8")==0 || strcasecmp(langi, "UTF8")==0){
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
        warn(TR("Statefull encodings are unsupported."));
        return FALSE;
    }
    
    ioncore_g.enc_sb=FALSE;
    ioncore_g.use_mb=TRUE;

    return TRUE;
    
integr_err:
    warn(TR("Cannot verify locale encoding setting integrity "
            "(LC_CTYPE=%s, nl_langinfo(CODESET)=%s). "
            "The LC_CTYPE environment variable should be of the form "
            "language_REGION.encoding (e.g. en_GB.UTF-8), and encoding "
            "should match the nl_langinfo value above."), ctype, langi);
    return FALSE;
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

#endif

#ifndef CF_NO_GETTEXT

#define TEXTDOMAIN "notion"

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
    if(NM==NULL) return FALSE

#define ADD_HOOK_(NM, FN)                          \
    if(!hook_add(NM, (void (*)())FN)) return FALSE

#define INIT_HOOK(NM, DFLT) INIT_HOOK_(NM); ADD_HOOK_(NM, DFLT)

static bool init_hooks()
{
    INIT_HOOK_(ioncore_post_layout_setup_hook);
    INIT_HOOK_(ioncore_snapshot_hook);
    INIT_HOOK_(ioncore_deinit_hook);
    INIT_HOOK_(screen_managed_changed_hook);
    INIT_HOOK_(frame_managed_changed_hook);
    INIT_HOOK_(clientwin_mapped_hook);
    INIT_HOOK_(clientwin_unmapped_hook);
    INIT_HOOK_(clientwin_property_change_hook);
    INIT_HOOK_(ioncore_submap_ungrab_hook);
    
    INIT_HOOK_(region_notify_hook);
    ADD_HOOK_(region_notify_hook, ioncore_screen_activity_notify);
    ADD_HOOK_(region_notify_hook, ioncore_region_notify);
    
    INIT_HOOK(clientwin_do_manage_alt, clientwin_do_manage_default);
    INIT_HOOK(ioncore_handle_event_alt, ioncore_handle_event);
    INIT_HOOK(region_do_warp_alt, region_do_warp_default);
    INIT_HOOK(ioncore_exec_environ_hook, ioncore_setup_environ);
    
    mainloop_sigchld_hook=mainloop_register_hook("ioncore_sigchld_hook",
                                                 create_hook());
    mainloop_sigusr2_hook=mainloop_register_hook("ioncore_sigusr2_hook",
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
    fail|=!ioncore_register_regclass(&CLASSDESCR(WGroupCW), 
                                     (WRegionLoadCreateFn*)groupcw_load);
    fail|=!ioncore_register_regclass(&CLASSDESCR(WGroupWS), 
                                     (WRegionLoadCreateFn*)groupws_load);
    
    return !fail;
}


#define INITSTR(NM)                             \
    ioncore_g.notifies.NM=stringstore_alloc(#NM); \
    if(ioncore_g.notifies.NM==STRINGID_NONE) return FALSE;
    
static bool init_global()
{
    /* argc, argv must be set be the program */
    ioncore_g.dpy=NULL;
    ioncore_g.display=NULL;

    ioncore_g.sm_client_id=NULL;
    ioncore_g.rootwins=NULL;
    ioncore_g.screens=NULL;
    ioncore_g.focus_next=NULL;
    ioncore_g.warp_next=FALSE;
    ioncore_g.focus_next_source=IONCORE_FOCUSNEXT_OTHER;
    
    ioncore_g.focuslist=NULL;
    ioncore_g.focus_current=NULL;

    ioncore_g.input_mode=IONCORE_INPUTMODE_NORMAL;
    ioncore_g.opmode=IONCORE_OPMODE_INIT;
    ioncore_g.dblclick_delay=CF_DBLCLICK_DELAY;
    ioncore_g.usertime_diff_current=CF_USERTIME_DIFF_CURRENT;
    ioncore_g.usertime_diff_new=CF_USERTIME_DIFF_NEW;
    ioncore_g.opaque_resize=0;
    ioncore_g.warp_enabled=TRUE;
    ioncore_g.warp_margin=5;
    ioncore_g.warp_factor[0]=0.0;
    ioncore_g.warp_factor[1]=0.0;
    ioncore_g.switchto_new=TRUE;
    ioncore_g.no_mousefocus=FALSE;
    ioncore_g.unsqueeze_enabled=TRUE;
    ioncore_g.autoraise=TRUE;
    ioncore_g.autosave_layout=TRUE;
    ioncore_g.window_stacking_request=IONCORE_WINDOWSTACKINGREQUEST_IGNORE;
    ioncore_g.focuslist_insert_delay=CF_FOCUSLIST_INSERT_DELAY;
    ioncore_g.workspace_indicator_timeout=CF_WORKSPACE_INDICATOR_TIMEOUT;
    ioncore_g.activity_notification_on_all_screens=FALSE;

    ioncore_g.enc_utf8=FALSE;
    ioncore_g.enc_sb=TRUE;
    ioncore_g.use_mb=FALSE;
    
    ioncore_g.screen_notify=TRUE;
    
    ioncore_g.frame_default_index=LLIST_INDEX_AFTER_CURRENT_ACT;
    
    ioncore_g.framed_transients=TRUE;
    
    ioncore_g.shape_extension=FALSE;
    ioncore_g.shape_event_basep=0;
    ioncore_g.shape_error_basep=0;

    INITSTR(activated);
    INITSTR(inactivated);
    INITSTR(activity);
    INITSTR(sub_activity);
    INITSTR(name);
    INITSTR(unset_manager);
    INITSTR(set_manager);
    INITSTR(unset_return);
    INITSTR(set_return);
    INITSTR(pseudoactivated);
    INITSTR(pseudoinactivated);
    INITSTR(tag);
    INITSTR(deinit);
    INITSTR(map);
    INITSTR(unmap);
    
    return TRUE;
}


bool ioncore_init(const char *prog, int argc, char *argv[],
                  const char *localedir)
{
    if(!init_global())
        return FALSE;
    
    progname=prog;
    ioncore_g.argc=argc;
    ioncore_g.argv=argv;

#ifndef CF_NO_LOCALE    
    init_locale();
#endif
#ifndef CF_NO_GETTEXT
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
    
    if(XShapeQueryExtension(ioncore_g.dpy, &ioncore_g.shape_event_basep,
                            &ioncore_g.shape_error_basep))
        ioncore_g.shape_extension=TRUE;
    else
        XMissingExtension(ioncore_g.dpy, "SHAPE");

    cloexec_braindamage_fix(ioncore_g.conn);
    
    ioncore_g.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
    ioncore_g.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
    ioncore_g.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
    ioncore_g.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    ioncore_g.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    ioncore_g.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    ioncore_g.atom_wm_window_role=XInternAtom(dpy, "WM_WINDOW_ROLE", False);
    ioncore_g.atom_checkcode=XInternAtom(dpy, "_ION_CWIN_RESTART_CHECKCODE", False);
    ioncore_g.atom_selection=XInternAtom(dpy, "_ION_SELECTION_STRING", False);
    ioncore_g.atom_dockapp_hack=XInternAtom(dpy, "_ION_DOCKAPP_HACK", False);
    ioncore_g.atom_mwm_hints=XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

    ioncore_init_xim();
    ioncore_init_bindings();
    ioncore_init_cursors();

    netwm_init();
    
    ioncore_init_session(XDisplayName(display));

    for(i=drw; i<nrw; i++)
        create_rootwin(i);

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
    
    region_focuslist_push((WRegion*)scr);
    region_do_set_focus((WRegion*)scr, FALSE);
}


bool ioncore_startup(const char *display, const char *cfgfile,
                     int stflags)
{
    WRootWin *rootwin;
    sigset_t inittrap;

    LOG(INFO, GENERAL, TR("Starting Notion"));

    /* Don't trap termination signals just yet. */
    sigemptyset(&inittrap);
    sigaddset(&inittrap, SIGALRM);
    sigaddset(&inittrap, SIGCHLD);
    sigaddset(&inittrap, SIGPIPE);
    sigaddset(&inittrap, SIGUSR2);
    mainloop_trap_signals(&inittrap);

    if(!extl_init())
        return FALSE;

    ioncore_register_exports();
    
    if(!ioncore_init_x(display, stflags))
        return FALSE;

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

    ioncore_deinit_xim();

    stringstore_deinit();

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
 * Is Notion supporting locale-specifically multibyte-encoded strings?
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

