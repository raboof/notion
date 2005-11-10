/*
 * ion/ioncore/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libmainloop/hooks.h>
#include "common.h"
#include "focus.h"
#include "global.h"
#include "window.h"
#include "region.h"
#include "colormap.h"
#include "activity.h"
#include "xwindow.h"


/*{{{ Hooks. */


WHook *region_do_warp_alt=NULL;
WHook *region_activated_hook=NULL;
WHook *region_inactivated_hook=NULL;


/*}}}*/


/*{{{ Previous active region tracking */


static Watch prev_watch=WATCH_INIT;


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
 * Go to and return the region that had its activity state previously 
 * saved.
 * 
 * Note that this function is asynchronous; the region will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT
WRegion *ioncore_goto_previous()
{
    WRegion *r=(WRegion*)prev_watch.obj;
    
    if(r!=NULL){
        watch_reset(&prev_watch);
        region_goto(r);
    }
    
    return r;
}


/*}}}*/


/*{{{ Await focus */


static Watch await_watch=WATCH_INIT;


static void await_watch_handler(Watch *watch, WRegion *prev)
{
    WRegion *r;
    while(1){
        r=REGION_PARENT_REG(prev);
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
        aw=REGION_PARENT_REG(aw);
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
    WRegion *par, *mgr, *tmp;
    
    check_clear_await(reg);
    
    region_set_activity(reg, SETPARAM_UNSET);

    if(reg->active_sub==NULL)
        ioncore_g.focus_current=reg;
    
    if(!REGION_IS_ACTIVE(reg)){
        D(fprintf(stderr, "got focus (inact) %s [%p]\n", OBJ_TYPESTR(reg), reg);)
        reg->flags|=REGION_ACTIVE;
        
        par=REGION_PARENT_REG(reg);
        if(par!=NULL)
            par->active_sub=reg;
        
        region_activated(reg);
        
        mgr=REGION_MANAGER(reg);
        tmp=reg;
        while(mgr!=NULL){
            /* We need to loop over managing non-windows (workspaces) here to
             * signal their managers.
             */
            region_managed_activated(mgr, tmp);
            if(REGION_PARENT_REG(reg)==mgr)
                break;
            tmp=mgr;
            mgr=REGION_MANAGER(mgr);
        }
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
    
    extl_protect(NULL);
    hook_call_o(region_activated_hook, (Obj*)reg);
    extl_unprotect(NULL);
}


void region_lost_focus(WRegion *reg)
{
    WRegion *r, *par;
    
    if(!REGION_IS_ACTIVE(reg)){
        D(fprintf(stderr, "lost focus (inact) %s [%p:]\n", OBJ_TYPESTR(reg), reg);)
        return;
    }
    
    par=REGION_PARENT_REG(reg);
    if(par!=NULL && par->active_sub==reg)
        par->active_sub=NULL;
    
    if(ioncore_g.focus_current==reg){
        /* Find the closest active parent, or if none is found, stop at the
         * screen and mark it "currently focused".
         */
        while(par!=NULL && !REGION_IS_ACTIVE(par) && !OBJ_IS(par, WScreen))
            par=REGION_PARENT_REG(par);
        ioncore_g.focus_current=par;
    }

    D(fprintf(stderr, "lost focus (act) %s [%p:]\n", OBJ_TYPESTR(reg), reg);)
    
    reg->flags&=~REGION_ACTIVE;
    region_inactivated(reg);
    r=REGION_MANAGER(reg);
    if(r!=NULL)
        region_managed_inactivated(r, reg);
    
    extl_protect(NULL);
    hook_call_o(region_inactivated_hook, (Obj*)reg);
    extl_unprotect(NULL);
}


/*}}}*/


/*{{{ Focus status requests */


/*EXTL_DOC
 * Is \var{reg} active/does it or one of it's children of focus?
 */
EXTL_SAFE
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
    
    par=REGION_PARENT_REG(reg);
    
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


static WRegion *find_warp_to_reg(WRegion *reg)
{
    if(reg==NULL)
        return NULL;
    if(reg->flags&REGION_PLEASE_WARP)
        return reg;
    return find_warp_to_reg(region_manager_or_parent(reg));
}


bool region_do_warp_default(WRegion *reg)
{
    int x, y, w, h, px=0, py=0;
    Window root;
    
    reg=find_warp_to_reg(reg);
    
    if(reg==NULL)
        return FALSE;
    
    D(fprintf(stderr, "region_do_warp %p %s\n", reg, OBJ_TYPESTR(reg)));
    
    root=region_root_of(reg);
    
    region_rootpos(reg, &x, &y);
    w=REGION_GEOM(reg).w;
    h=REGION_GEOM(reg).h;

    if(xwindow_pointer_pos(root, &px, &py)){
        if(px>=x && py>=y && px<x+w && py<y+h)
            return TRUE;
    }
    
    XWarpPointer(ioncore_g.dpy, None, root, 0, 0, 0, 0,
                 x+5, y+5);
    
    return TRUE;
}


void region_do_warp(WRegion *reg)
{
    extl_protect(NULL);
    hook_call_alt_o(region_do_warp_alt, (Obj*)reg);
    extl_unprotect(NULL);
}


void region_maybewarp(WRegion *reg, bool warp)
{
    ioncore_g.focus_next=reg;
    ioncore_g.warp_next=(warp && ioncore_g.warp_enabled);
}


void region_set_focus(WRegion *reg)
{
    region_maybewarp(reg, FALSE);
}


void region_warp(WRegion *reg)
{
    region_maybewarp(reg, TRUE);
}


/*}}}*/


/*{{{ Misc. */


bool region_skip_focus(WRegion *reg)
{
    while(reg!=NULL){
        if(reg->flags&REGION_SKIP_FOCUS)
            return TRUE;
        reg=REGION_PARENT_REG(reg);
    }
    return FALSE;
}

/*EXTL_DOC
 * Returns the currently focused region, if any.
 */
EXTL_EXPORT
WRegion *ioncore_current()
{
    return ioncore_g.focus_current;
}

/*}}}*/
