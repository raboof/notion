/*
 * ion/ioncore/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "focus.h"
#include "global.h"
#include "window.h"
#include "region.h"
#include "hooks.h"
#include "colormap.h"
#include "activity.h"
#include "region-iter.h"


/*{{{ Previous active region tracking */


static Watch prev_watch=WWATCH_INIT;


static void prev_watch_handler(Watch *watch, WRegion *prev)
{
    WRegion *r;
    while(1){
        r=region_manager_or_parent(prev);
        if(r==NULL)
            break;
        
        if(watch_setup(&prev_watch, (Obj*)r, 
                       (WatchHandler*)prev_watch_handler))
            break;
        prev=r;
    }
}


void ioncore_set_previous_of(WRegion *reg)
{
    WRegion *r2=NULL, *r3=NULL;
    
    if(reg==NULL || ioncore_g.previous_protect!=0)
        return;

    if(REGION_IS_ACTIVE(reg))
        return;
    
    r3=(WRegion*)ioncore_g.active_screen;
    
    while(r3!=NULL){
        r2=r3;
        r3=r2->active_sub;
        if(r3==NULL)
            r3=region_current(r2);
    }

    if(r2!=NULL)
        watch_setup(&prev_watch, (Obj*)r2, (WatchHandler*)prev_watch_handler);
}


void ioncore_protect_previous()
{
    ioncore_g.previous_protect++;
}


void ioncore_unprotect_previous()
{
    assert(ioncore_g.previous_protect>0);
    ioncore_g.previous_protect--;
}


/*EXTL_DOC
 * Go to a region that had its activity state previously saved.
 */
EXTL_EXPORT
void ioncore_goto_previous()
{
    WRegion *r=(WRegion*)prev_watch.obj;
    
    if(r!=NULL){
        watch_reset(&prev_watch);
        region_goto(r);
    }
}


/*}}}*/


/*{{{ Await focus */


static Watch await_watch=WWATCH_INIT;


static void await_watch_handler(Watch *watch, WRegion *prev)
{
    WRegion *r;
    while(1){
        r=region_parent(prev);
        if(r==NULL)
            break;
        
        if(watch_setup(&await_watch, (Obj*)r, 
                       (WatchHandler*)await_watch_handler))
            break;
        prev=r;
    }
}


void region_set_await_focus(WRegion *reg)
{
    if(reg==NULL){
        watch_reset(&await_watch);
    }else{
        watch_setup(&await_watch, (Obj*)reg,
                    (WatchHandler*)await_watch_handler);
    }
}


static bool region_is_await(WRegion *reg)
{
    WRegion *aw=(WRegion*)await_watch.obj;
    
    while(aw!=NULL){
        if(aw==reg)
            return TRUE;
        aw=region_parent(aw);
    }
    
    return FALSE;
}


/* Only keep await status if focus event is to an ancestor of the await 
 * region.
 */
static void check_clear_await(WRegion *reg)
{
    if(region_is_await(reg) && reg!=(WRegion*)await_watch.obj)
        return;
    
    watch_reset(&await_watch);
}


/*}}}*/


/*{{{ Events */


void region_got_focus(WRegion *reg)
{
    WRegion *r;
    
    check_clear_await(reg);
    
    region_clear_activity(reg);
    
    if(!REGION_IS_ACTIVE(reg)){
        D(fprintf(stderr, "got focus (inact) %s [%p]\n", OBJ_TYPESTR(reg), reg);)
        reg->flags|=REGION_ACTIVE;
        
        r=region_parent(reg);
        if(r!=NULL)
            r->active_sub=reg;
        
        region_activated(reg);
        
        r=REGION_MANAGER(reg);
        if(r!=NULL)
            region_managed_activated(r, reg);
    }else{
        D(fprintf(stderr, "got focus (act) %s [%p]\n", OBJ_TYPESTR(reg), reg);)
    }

    /* Install default colour map only if there is no active subregion;
     * their maps should come first. WClientWins will install their maps
     * in region_activated. Other regions are supposed to use the same
     * default map.
     */
    if(reg->active_sub==NULL && !OBJ_IS(reg, WClientWin))
        rootwin_install_colormap(region_rootwin_of(reg), None); 
}


void region_lost_focus(WRegion *reg)
{
    WRegion *r;
    
    if(!REGION_IS_ACTIVE(reg)){
        D(fprintf(stderr, "lost focus (inact) %s [%p:]\n", OBJ_TYPESTR(reg), reg);)
        return;
    }
    
    D(fprintf(stderr, "lost focus (act) %s [%p:]\n", OBJ_TYPESTR(reg), reg);)
    
    reg->flags&=~REGION_ACTIVE;
    region_inactivated(reg);
    r=REGION_MANAGER(reg);
    if(r!=NULL)
        region_managed_inactivated(r, reg);
}


/*}}}*/


/*{{{ Focus status requests */

/*EXTL_DOC
 * Is \var{reg} active/does it or one of it's children of focus?
 */
EXTL_EXPORT_MEMBER
bool region_is_active(WRegion *reg)
{
    return REGION_IS_ACTIVE(reg);
}


bool region_may_control_focus(WRegion *reg)
{
    WRegion *par, *r2;
    
    if(OBJ_IS_BEING_DESTROYED(reg))
        return FALSE;

    if(REGION_IS_ACTIVE(reg))
        return TRUE;
    
    if(region_is_await(reg))
        return TRUE;
    
    par=region_parent(reg);
    
    if(par==NULL || !REGION_IS_ACTIVE(par))
        return FALSE;
    
    r2=par->active_sub;
    while(r2!=NULL && r2!=par){
        if(r2==reg)
            return TRUE;
        r2=REGION_MANAGER(r2);
    }

    return FALSE;
}


/*}}}*/


/*{{{ set_focus, warp */


WHooklist *region_do_warp_alt=NULL;


bool region_do_warp_default(WRegion *reg)
{
    Window root, win=None, realroot=None;
    int x, y, w, h;
    int wx=0, wy=0, px=0, py=0;
    uint mask=0;
    
    D(fprintf(stderr, "region_do_warp %p %s\n", reg, OBJ_TYPESTR(reg)));
    
    root=region_root_of(reg);
    region_rootpos(reg, &x, &y);

    if(XQueryPointer(ioncore_g.dpy, root, &realroot, &win,
                     &px, &py, &wx, &wy, &mask)){
        w=REGION_GEOM(reg).w;
        h=REGION_GEOM(reg).h;

        if(px>=x && py>=y && px<x+w && py<y+h)
            return TRUE;
    }
    
    XWarpPointer(ioncore_g.dpy, None, root, 0, 0, 0, 0,
                 x+5, y+5);
    
    return TRUE;
}


void region_do_warp(WRegion *reg)
{
    CALL_ALT_B_NORET(region_do_warp_alt, (reg));
}


void region_set_focus(WRegion *reg)
{
    D(fprintf(stderr, "set_focus %p %s\n", reg, OBJ_TYPESTR(reg)));
    ioncore_g.focus_next=reg;
    ioncore_g.warp_next=FALSE;
}


void region_warp(WRegion *reg)
{
    D(fprintf(stderr, "warp %p %s\n", reg, OBJ_TYPESTR(reg)));
    ioncore_g.focus_next=reg;
    ioncore_g.warp_next=ioncore_g.warp_enabled;
}


WRegion *region_set_focus_mgrctl(WRegion *freg, bool dowarp)
{
    WRegion *mgr=freg;
    WRegion *reg;
    
    while(1){
        reg=mgr;
        mgr=REGION_MANAGER(reg);
        if(mgr==NULL)
            break;
        reg=region_control_managed_focus(mgr, reg);
        if(reg!=NULL)
            freg=reg;
    }
    
    if(!REGION_IS_ACTIVE(freg)){
        ioncore_g.focus_next=freg;
        ioncore_g.warp_next=(ioncore_g.warp_enabled && dowarp);
    }

    return freg;
}


void xwindow_do_set_focus(Window win)
{
    region_set_await_focus(xwindow_region_of(win));
    XSetInputFocus(ioncore_g.dpy, win, RevertToParent, CurrentTime);
}


/*}}}*/

