/*
 * ion/ioncore/rootwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include <libtu/objp.h>
#include "common.h"
#include "rootwin.h"
#include "binding.h"
#include "cursor.h"
#include "global.h"
#include "event.h"
#include "gr.h"
#include "clientwin.h"
#include "property.h"
#include "focus.h"
#include "regbind.h"
#include "screen.h"
#include "bindmaps.h"
#include <libextl/readconfig.h>
#include "resize.h"
#include "saveload.h"
#include "netwm.h"
#include "xwindow.h"
#include "log.h"


/*{{{ Error handling */


static bool redirect_error=FALSE;
static bool ignore_badwindow=TRUE;


static int my_redirect_error_handler(Display *UNUSED(dpy), XErrorEvent *UNUSED(ev))
{
    redirect_error=TRUE;
    return 0;
}

static void modifiers_to_string(char* target, const uint modifiers)
{
    if ((modifiers & ShiftMask) != 0) {
      strcpy(target, "Shift-");
      target += strlen("Shift-");
    }
    if ((modifiers & LockMask) != 0) {
      strcpy(target, "Lock-");
      target += strlen("Lock-");
    }
    if ((modifiers & ControlMask) != 0) {
      strcpy(target, "Control-");
      target += strlen("Control-");
    }
    if ((modifiers & Mod1Mask) != 0) {
      strcpy(target, "Mod1-");
      target += strlen("Mod1-");
    }
    if ((modifiers & Mod2Mask) != 0) {
      strcpy(target, "Mod2-");
      target += strlen("Mod2-");
    }
    if ((modifiers & Mod3Mask) != 0) {
      strcpy(target, "Mod3-");
      target += strlen("Mod3-");
    }
    if ((modifiers & Mod4Mask) != 0) {
      strcpy(target, "Mod4-");
      target += strlen("Mod4-");
    }
    if ((modifiers & Mod5Mask) != 0) {
      strcpy(target, "Mod5-");
      target += strlen("Mod5-");
    }
    *target = '\0';
}


static int my_error_handler(Display *dpy, XErrorEvent *ev)
{
    static char msg[128], request[64], num[32], mod_str[255];

    if (ev->error_code==BadAccess && ev->request_code==X_GrabKey) {
        int ksb = ioncore_ksb(ev->serial);
        int modifiers = ioncore_modifiers(ev->serial);

        if (ksb == -1)
            LOG(WARN, GENERAL, "Failed to grab some key. Moving on without it.");
        else{
            char* key = XKeysymToString(ksb);
            if(key == NULL)
                LOG(WARN, GENERAL, "Failed to grab key with keysym %d. Moving on without it.", ksb);
            else{
                modifiers_to_string(mod_str, modifiers);
                LOG(WARN, GENERAL, "Failed to grab key %s%s. Moving on without it.", mod_str, key);
            }
        }
        return 0;
    }

    /* Just ignore bad window and similar errors; makes the rest of
     * the code simpler.
     */
    if((ev->error_code==BadWindow ||
        (ev->error_code==BadMatch && ev->request_code==X_SetInputFocus) ||
        (ev->error_code==BadDrawable && ev->request_code==X_GetGeometry)) &&
       ignore_badwindow)
        return 0;

#if 0
    XmuPrintDefaultErrorMessage(dpy, ev, stderr);
#else
    XGetErrorText(dpy, ev->error_code, msg, 128);
    snprintf(num, 32, "%d", ev->request_code);
    XGetErrorDatabaseText(dpy, "XRequest", num, "", request, 64);

    if(request[0]=='\0')
        snprintf(request, 64, "<unknown request>");

    if(ev->minor_code!=0){
        warn("[%d] %s (%d.%d) %#lx: %s", ev->serial, request,
             ev->request_code, ev->minor_code, ev->resourceid,msg);
    }else{
        warn("[%d] %s (%d) %#lx: %s", ev->serial, request,
             ev->request_code, ev->resourceid,msg);
    }
#endif

    kill(getpid(), SIGTRAP);

    return 0;
}


/*}}}*/


/*{{{ Init/deinit */


static void scan_initial_windows(WRootWin *rootwin)
{
    Window dummy_root, dummy_parent, *wins=NULL;
    uint nwins=0, i, j;
    XWMHints *hints;

    XQueryTree(ioncore_g.dpy, WROOTWIN_ROOT(rootwin), &dummy_root, &dummy_parent,
               &wins, &nwins);

    for(i=0; i<nwins; i++){
        if(wins[i]==None)
            continue;
        hints=XGetWMHints(ioncore_g.dpy, wins[i]);
        if(hints!=NULL && hints->flags&IconWindowHint){
            for(j=0; j<nwins; j++){
                if(wins[j]==hints->icon_window){
                    wins[j]=None;
                    break;
                }
            }
        }
        if(hints!=NULL)
            XFree((void*)hints);
    }

    rootwin->tmpwins=wins;
    rootwin->tmpnwins=nwins;
}


void rootwin_manage_initial_windows(WRootWin *rootwin)
{
    Window *wins=rootwin->tmpwins;
    Window tfor=None;
    int i, nwins=rootwin->tmpnwins;

    rootwin->tmpwins=NULL;
    rootwin->tmpnwins=0;

    for(i=0; i<nwins; i++){
        if(XWINDOW_REGION_OF(wins[i])!=NULL)
            wins[i]=None;
        if(wins[i]==None)
            continue;
        if(XGetTransientForHint(ioncore_g.dpy, wins[i], &tfor))
            continue;
        ioncore_manage_clientwin(wins[i], FALSE);
        wins[i]=None;
    }

    for(i=0; i<nwins; i++){
        if(wins[i]==None)
            continue;
        ioncore_manage_clientwin(wins[i], FALSE);
    }

    XFree((void*)wins);
}


static void create_wm_windows(WRootWin *rootwin)
{
    const char *p[1];

    rootwin->dummy_win=XCreateWindow(ioncore_g.dpy, WROOTWIN_ROOT(rootwin),
                                     0, 0, 1, 1, 0,
                                     CopyFromParent, InputOnly,
                                     CopyFromParent, 0, NULL);

    p[0] = "WRootWin";
    xwindow_set_text_property(rootwin->dummy_win, XA_WM_NAME, p, 1);

    XSelectInput(ioncore_g.dpy, rootwin->dummy_win, PropertyChangeMask);
}


static void preinit_gr(WRootWin *rootwin)
{
    XGCValues gcv;
    ulong gcvmask;

    /* Create XOR gc (for resize) */
    gcv.line_style=LineSolid;
    gcv.join_style=JoinBevel;
    gcv.cap_style=CapButt;
    gcv.fill_style=FillSolid;
    gcv.line_width=2;
    gcv.subwindow_mode=IncludeInferiors;
    gcv.function=GXxor;
    gcv.foreground=~0L;

    gcvmask=(GCLineStyle|GCLineWidth|GCFillStyle|
             GCJoinStyle|GCCapStyle|GCFunction|
             GCSubwindowMode|GCForeground);

    rootwin->xor_gc=XCreateGC(ioncore_g.dpy, WROOTWIN_ROOT(rootwin),
                              gcvmask, &gcv);
}


static bool rootwin_init(WRootWin *rootwin, int xscr)
{
    Display *dpy=ioncore_g.dpy;
    WFitParams fp;
    Window root;
    WScreen *scr;

    /* Try to select input on the root window */
    root=RootWindow(dpy, xscr);

    redirect_error=FALSE;

    XSetErrorHandler(my_redirect_error_handler);
    XSelectInput(dpy, root, IONCORE_EVENTMASK_ROOT);
    XSync(dpy, 0);
    XSetErrorHandler(my_error_handler);

    if(redirect_error){
        warn(TR("Unable to redirect root window events for screen %d."),
             xscr);
        return FALSE;
    }

    rootwin->xscr=xscr;
    rootwin->default_cmap=DefaultColormap(dpy, xscr);
    rootwin->tmpwins=NULL;
    rootwin->tmpnwins=0;
    rootwin->dummy_win=None;
    rootwin->xor_gc=None;

    fp.mode=REGION_FIT_EXACT;
    fp.g.x=0; fp.g.y=0;
    fp.g.w=DisplayWidth(dpy, xscr);
    fp.g.h=DisplayHeight(dpy, xscr);

    if(!window_do_init((WWindow*)rootwin, NULL, &fp, root, "WRootWin")){
        return FALSE;
    }

    ((WWindow*)rootwin)->event_mask=IONCORE_EVENTMASK_ROOT;
    ((WRegion*)rootwin)->flags|=REGION_BINDINGS_ARE_GRABBED|REGION_PLEASE_WARP;
    ((WRegion*)rootwin)->rootwin=rootwin;

    REGION_MARK_MAPPED(rootwin);

    scan_initial_windows(rootwin);

    create_wm_windows(rootwin);
    preinit_gr(rootwin);
    netwm_init_rootwin(rootwin);

    region_add_bindmap((WRegion*)rootwin, ioncore_screen_bindmap);

    scr=create_screen(rootwin, &fp, xscr);
    if(scr==NULL){
        return FALSE;
    }
    region_set_manager((WRegion*)scr, (WRegion*)rootwin);
    region_map((WRegion*)scr);

    LINK_ITEM(*(WRegion**)&ioncore_g.rootwins, (WRegion*)rootwin, p_next, p_prev);

    ioncore_screens_updated(rootwin);

    xwindow_set_cursor(root, IONCORE_CURSOR_DEFAULT);

    return TRUE;
}


WRootWin *create_rootwin(int xscr)
{
    CREATEOBJ_IMPL(WRootWin, rootwin, (p, xscr));
}


void rootwin_deinit(WRootWin *rw)
{
    WScreen *scr, *next;

    FOR_ALL_SCREENS_W_NEXT(scr, next){
        if(REGION_MANAGER(scr)==(WRegion*)rw)
            destroy_obj((Obj*)scr);
    }

    UNLINK_ITEM(*(WRegion**)&ioncore_g.rootwins, (WRegion*)rw, p_next, p_prev);

    XSelectInput(ioncore_g.dpy, WROOTWIN_ROOT(rw), 0);

    XFreeGC(ioncore_g.dpy, rw->xor_gc);

    window_deinit((WWindow*)rw);
}


/*}}}*/


/*{{{ region dynfun implementations */


static void rootwin_do_set_focus(WRootWin *rootwin, bool warp)
{
    WRegion *sub;

    sub=REGION_ACTIVE_SUB(rootwin);

    if(sub==NULL || !REGION_IS_MAPPED(sub)){
        WScreen *scr;
        FOR_ALL_SCREENS(scr){
            if(REGION_IS_MAPPED(scr)){
                sub=(WRegion*)scr;
                break;
            }
        }
    }

    if(sub!=NULL)
        region_do_set_focus(sub, warp);
    else
        window_do_set_focus((WWindow*)rootwin, warp);
}


static bool rootwin_fitrep(WRootWin *UNUSED(rootwin), WWindow *UNUSED(par),
                           const WFitParams *UNUSED(fp))
{
    D(warn("Don't know how to reparent or fit root windows."));
    return FALSE;
}


static void rootwin_map(WRootWin *UNUSED(rootwin))
{
    D(warn("Attempt to map a root window."));
}


static void rootwin_unmap(WRootWin *UNUSED(rootwin))
{
    D(warn("Attempt to unmap a root window -- impossible."));
}


static void rootwin_managed_remove(WRootWin *rootwin, WRegion *reg)
{
    region_unset_manager(reg, (WRegion*)rootwin);
}

static WRegion *rootwin_managed_disposeroot(WRootWin *UNUSED(rootwin), WRegion *reg)
{
    WScreen *scr=OBJ_CAST(reg, WScreen);
    if(scr!=NULL && scr==scr->prev_scr){
        warn(TR("Only screen may not be destroyed/detached."));
        return NULL;
    }

    return reg;
}



static Window rootwin_x_window(WRootWin *rootwin)
{
    return WROOTWIN_ROOT(rootwin);
}


/*}}}*/


/*{{{ Misc */


static bool scr_ok(WRegion *r)
{
    return (OBJ_IS(r, WScreen) && REGION_IS_MAPPED(r));
}


/*EXTL_DOC
 * Returns previously active screen on root window \var{rootwin}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WScreen *rootwin_current(WRootWin *rootwin)
{
    WRegion *r=REGION_ACTIVE_SUB(rootwin);
    WScreen *scr;

    /* There should be no non-WScreen as children or managed by us, but... */

    if(r!=NULL && scr_ok(r))
        return (WScreen*)r;

    FOR_ALL_SCREENS(scr){
        if(REGION_MANAGER(scr)==(WRegion*)rootwin
           && REGION_IS_MAPPED(scr)){
            break;
        }
    }

    return scr;
}

/*EXTL_DOC
 * Warp the cursor pointer to this location
 *
 * I'm not *entirely* sure what 'safe' means, but this doesn't change internal
 * notion state, so I guess it's 'safe'...
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
void rootwin_warp_pointer(WRootWin *root, int x, int y)
{
    XWarpPointer(ioncore_g.dpy, None, WROOTWIN_ROOT(root), 0, 0, 0, 0, x, y);
}


/*EXTL_DOC
 * Returns the first WRootWin
 */
EXTL_SAFE
EXTL_EXPORT
WRootWin *ioncore_rootwin()
{
    return ioncore_g.rootwins;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab rootwin_dynfuntab[]={
    {region_map, rootwin_map},
    {region_unmap, rootwin_unmap},
    {region_do_set_focus, rootwin_do_set_focus},
    {(DynFun*)region_xwindow, (DynFun*)rootwin_x_window},
    {(DynFun*)region_fitrep, (DynFun*)rootwin_fitrep},
    {region_managed_remove, rootwin_managed_remove},
    {(DynFun*)region_managed_disposeroot,
     (DynFun*)rootwin_managed_disposeroot},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WRootWin, WWindow, rootwin_deinit, rootwin_dynfuntab);


/*}}}*/
