/*
 * ion/ioncore/rootwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
/*#include <X11/Xmu/Error.h>*/
#ifdef CF_XINERAMA
#include <X11/extensions/Xinerama.h>
#elif defined(CF_SUN_XINERAMA)
#include <X11/extensions/xinerama.h>
#endif

#include <libtu/objp.h>
#include "common.h"
#include "rootwin.h"
#include "cursor.h"
#include "global.h"
#include "event.h"
#include "gr.h"
#include "clientwin.h"
#include "property.h"
#include "focus.h"
#include "regbind.h"
#include "screen.h"
#include "screen.h"
#include "bindmaps.h"
#include <libextl/readconfig.h>
#include "resize.h"
#include "saveload.h"
#include "netwm.h"
#include "xwindow.h"


/*{{{ Error handling */


static bool redirect_error=FALSE;
static bool ignore_badwindow=TRUE;


static int my_redirect_error_handler(Display *dpy, XErrorEvent *ev)
{
    redirect_error=TRUE;
    return 0;
}


static int my_error_handler(Display *dpy, XErrorEvent *ev)
{
    static char msg[128], request[64], num[32];
    
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
    rootwin->dummy_win=XCreateWindow(ioncore_g.dpy, WROOTWIN_ROOT(rootwin),
                                     0, 0, 1, 1, 0,
                                     CopyFromParent, InputOnly,
                                     CopyFromParent, 0, NULL);

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


static WRootWin *preinit_rootwin(int xscr)
{
    Display *dpy=ioncore_g.dpy;
    WRootWin *rootwin;
    WFitParams fp;
    Window root;
    int i;
    
    /* Try to select input on the root window */
    root=RootWindow(dpy, xscr);
    
    redirect_error=FALSE;

    XSetErrorHandler(my_redirect_error_handler);
    XSelectInput(dpy, root, IONCORE_EVENTMASK_ROOT|IONCORE_EVENTMASK_SCREEN);
    XSync(dpy, 0);
    XSetErrorHandler(my_error_handler);

    if(redirect_error){
        warn(TR("Unable to redirect root window events for screen %d."),
             xscr);
        return NULL;
    }
    
    rootwin=ALLOC(WRootWin);
    
    if(rootwin==NULL)
        return NULL;
    
    /* Init the struct */
    OBJ_INIT(rootwin, WRootWin);

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
    
    if(!screen_init((WScreen*)rootwin, NULL, &fp, xscr, root)){
        free(rootwin);
        return NULL;
    }

    ((WWindow*)rootwin)->event_mask=IONCORE_EVENTMASK_ROOT|IONCORE_EVENTMASK_SCREEN;
    ((WRegion*)rootwin)->flags|=REGION_BINDINGS_ARE_GRABBED|REGION_PLEASE_WARP;
    ((WRegion*)rootwin)->rootwin=rootwin;
    
    REGION_MARK_MAPPED(rootwin);
    
    scan_initial_windows(rootwin);

    create_wm_windows(rootwin);
    preinit_gr(rootwin);
    netwm_init_rootwin(rootwin);
    
    /*region_add_bindmap((WRegion*)rootwin, ioncore_screen_bindmap);*/
    
    return rootwin;
}


static Atom net_virtual_roots=None;

#undef CF_XINERAMA
#undef CF_SUN_XINERAMA

#if defined(CF_XINERAMA) | defined(CF_SUN_XINERAMA)
static WScreen *add_screen(WRootWin *rw, int id, const WRectangle *geom, 
                           bool useroot)
{
    WScreen *scr;
    CARD32 p[1];
    WFitParams fp;
    
    fp.g=*geom;
    fp.mode=REGION_FIT_EXACT;
    
#ifdef CF_ALWAYS_VIRTUAL_ROOT
    useroot=FALSE;
#endif

    scr=create_screen(rw, id, &fp, useroot);
    
    if(scr==NULL)
        return NULL;
    
    region_set_manager((WRegion*)scr, (WRegion*)rw);
    
    region_map((WRegion*)scr);

    if(!useroot){
        p[0]=region_xwindow((WRegion*)scr);
        XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw), net_virtual_roots,
                        XA_WINDOW, 32, PropModeAppend, (uchar*)&(p[0]), 1);
    }

    return scr;
}
#endif


#ifdef CF_XINERAMA
static bool xinerama_sanity_check(XineramaScreenInfo *xi, int nxi)
{
    int i, j;

    for(i=0; i<nxi; i++){
        for(j=0; j<nxi; j++){
            if(i!=j &&
               (xi[j].x_org>=xi[i].x_org && xi[j].x_org<xi[i].x_org+xi[i].width) &&
               (xi[j].y_org>=xi[i].y_org && xi[j].y_org<xi[i].y_org+xi[i].height)){
                warn(TR("Xinerama sanity check failed; overlapping "
                        "screens detected."));
                return FALSE;
            }
        }
        
        if(xi[i].width<=0 || xi[i].height<=0){
            warn(TR("Xinerama sanity check failed; zero size detected."));
            return FALSE;
        }
    }
    return TRUE;
}
#elif defined(CF_SUN_XINERAMA)
static bool xinerama_sanity_check(XRectangle *monitors, int nxi)
{
    int i, j;

    for(i=0; i<nxi; i++){
        for(j=0; j<nxi; j++){
            if(i!=j &&
               (monitors[j].x>=monitors[i].x &&
                monitors[j].x<monitors[i].x+monitors[i].width) &&
               (monitors[j].y>=monitors[i].y &&
                monitors[j].y<monitors[i].y+monitors[i].height)){
                warn(TR("Xinerama sanity check failed; overlapping "
                        "screens detected."));
                return FALSE;
            }
        }
        
        if(monitors[i].width<=0 || monitors[i].height<=0){
            warn(TR("Xinerama sanity check failed; zero size detected."));
            return FALSE;
        }
    }
    return TRUE;
}
#endif


WRootWin *ioncore_manage_rootwin(int xscr, bool noxinerama)
{
    WRootWin *rootwin;
    int nxi=0, fail=0;
#ifdef CF_XINERAMA
    XineramaScreenInfo *xi=NULL;
    int i;
    int event_base, error_base;
#elif defined(CF_SUN_XINERAMA)
    XRectangle monitors[MAXFRAMEBUFFERS];
    int i;
#endif

    if(!noxinerama){
#ifdef CF_XINERAMA
        if(XineramaQueryExtension(ioncore_g.dpy, &event_base, &error_base)){
            xi=XineramaQueryScreens(ioncore_g.dpy, &nxi);

            if(xi!=NULL && ioncore_g.rootwins!=NULL){
                warn(TR("Don't know how to get Xinerama information for "
                        "multiple X root windows."));
                XFree(xi);
                xi=NULL;
                nxi=0;
            }
        }
#elif defined(CF_SUN_XINERAMA)
        if(XineramaGetState(ioncore_g.dpy, xscr)){
            unsigned char hints[16];
            int num;

            if(XineramaGetInfo(ioncore_g.dpy, xscr, monitors, hints,
                               &nxi)==0){
                warn(TR("Error retrieving Xinerama information."));
                nxi=0;
            }else{
                if(ioncore_g.rootwins!=NULL){
                    warn(TR("Don't know how to get Xinerama information for "
                            "multiple X root windows."));
                    nxi=0;
                }
            }
        }
#endif
    }

    rootwin=preinit_rootwin(xscr);

    if(rootwin==NULL){
#ifdef CF_XINERAMA
        if(xi!=NULL)
            XFree(xi);
#endif
        return NULL;
    }

    net_virtual_roots=XInternAtom(ioncore_g.dpy, "_NET_VIRTUAL_ROOTS", False);
    XDeleteProperty(ioncore_g.dpy, WROOTWIN_ROOT(rootwin), net_virtual_roots);

#if defined(CF_XINERAMA) || defined(CF_SUN_XINERAMA)
#ifdef CF_XINERAMA
    if(xi!=NULL && nxi!=0 && xinerama_sanity_check(xi, nxi)){
        bool useroot=FALSE;
        WRectangle geom;

        for(i=0; i<nxi; i++){
            geom.x=xi[i].x_org;
            geom.y=xi[i].y_org;
            geom.w=xi[i].width;
            geom.h=xi[i].height;
            /*if(nxi==1)
                useroot=(geom.x==0 && geom.y==0);*/
            if(!add_screen(rootwin, i, &geom, useroot)){
                warn(TR("Unable to setup Xinerama screen %d."), i);
                fail++;
            }
        }
        XFree(xi);
    }else
#else
    if(nxi!=0 && xinerama_sanity_check(monitors, nxi)){
        bool useroot=FALSE;
        WRectangle geom;

        for(i=0; i<nxi; i++){
            geom.x=monitors[i].x;
            geom.y=monitors[i].y;
            geom.w=monitors[i].width;
            geom.h=monitors[i].height;
            /*if(nxi==1)
                useroot=(geom.x==0 && geom.y==0);*/
            if(!add_screen(rootwin, i, &geom, useroot)){
                warn(TR("Unable to setup Xinerama screen %d."), i);
                fail++;
            }
        }
    }
#endif

    if(fail==nxi){
        warn(TR("Unable to setup X screen %d."), xscr);
        destroy_obj((Obj*)rootwin);
        return NULL;
    }
#endif
    
    
    /* */ {
        /* TODO: typed LINK_ITEM */
        WRegion *tmp=(WRegion*)ioncore_g.rootwins;
        LINK_ITEM(tmp, (WRegion*)rootwin, p_next, p_prev);
        ioncore_g.rootwins=(WRootWin*)tmp;
    }

    xwindow_set_cursor(WROOTWIN_ROOT(rootwin), IONCORE_CURSOR_DEFAULT);
    
    return rootwin;
}


void rootwin_deinit(WRootWin *rw)
{
    WScreen *scr, *next;

    FOR_ALL_SCREENS_W_NEXT(scr, next){
        if(REGION_MANAGER(scr)==(WRegion*)rw)
            destroy_obj((Obj*)scr);
    }
    
    /* */ {
        WRegion *tmp=(WRegion*)ioncore_g.rootwins;
        UNLINK_ITEM(tmp, (WRegion*)rw, p_next, p_prev);
        ioncore_g.rootwins=(WRootWin*)tmp;
    }
    
    XSelectInput(ioncore_g.dpy, WROOTWIN_ROOT(rw), 0);
    
    XFreeGC(ioncore_g.dpy, rw->xor_gc);
    
    rw->scr.mplex.win.win=None;

    screen_deinit(&rw->scr);
}


/*}}}*/


/*{{{ region dynfun implementations */


static bool rootwin_fitrep(WRootWin *rootwin, WWindow *par, 
                           const WFitParams *fp)
{
    D(warn("Don't know how to reparent or fit root windows."));
    return FALSE;
}


static void rootwin_map(WRootWin *rootwin)
{
    D(warn("Attempt to map a root window."));
}


static void rootwin_unmap(WRootWin *rootwin)
{
    D(warn("Attempt to unmap a root window -- impossible."));
}


/*}}}*/


/*{{{ Misc */


/*EXTL_DOC
 * Returns previously active screen on root window \var{rootwin}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WScreen *rootwin_current_scr(WRootWin *rootwin)
{
    WScreen *scr, *fb=NULL;
    
    FOR_ALL_SCREENS(scr){
        if(REGION_MANAGER(scr)==(WRegion*)rootwin && REGION_IS_MAPPED(scr)){
            fb=scr;
            if(REGION_IS_ACTIVE(scr))
                return scr;
        }
    }
    
    return (fb ? fb : &rootwin->scr);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab rootwin_dynfuntab[]={
    {region_map, rootwin_map},
    {region_unmap, rootwin_unmap},
    {(DynFun*)region_fitrep, (DynFun*)rootwin_fitrep},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WRootWin, WScreen, rootwin_deinit, rootwin_dynfuntab);

    
/*}}}*/
