/*
 * ion/ioncore/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
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
#include "regbind.h"


/*{{{ Hooks. */


WHook *region_do_warp_alt=NULL;


/*}}}*/


/*{{{ Focus list */


void region_focuslist_remove_with_mgrs(WRegion *reg)
{
    WRegion *mgrp=region_manager_or_parent(reg);
    
    UNLINK_ITEM(ioncore_g.focus_current, reg, active_next, active_prev);
    
    if(mgrp!=NULL)
        region_focuslist_remove_with_mgrs(mgrp);
}


void region_focuslist_push(WRegion *reg)
{
    region_focuslist_remove_with_mgrs(reg);
    LINK_ITEM_FIRST(ioncore_g.focus_current, reg, active_next, active_prev);
}


void region_focuslist_move_after(WRegion *reg, WRegion *after)
{
    region_focuslist_remove_with_mgrs(reg);
    LINK_ITEM_AFTER(ioncore_g.focus_current, after, reg, 
                    active_next, active_prev);
}


void region_focuslist_deinit(WRegion *reg)
{
    WRegion *replace=region_manager_or_parent(reg);
    
    if(replace!=NULL)
        region_focuslist_move_after(replace, reg);
        
    UNLINK_ITEM(ioncore_g.focus_current, reg, active_next, active_prev);
}


/*EXTL_DOC
 * Go to and return to a previously active region (if any).
 * 
 * Note that this function is asynchronous; the region will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT
WRegion *ioncore_goto_previous()
{
    WRegion *next;
    
    if(ioncore_g.focus_current==NULL)
        return NULL;
    
    /* Find the first region on focus history list that isn't currently
     * active.
     */
    for(next=ioncore_g.focus_current->active_next;
        next!=NULL; 
        next=next->active_next){
        
        if(!REGION_IS_ACTIVE(next))
            break;
    }
    
    if(next!=NULL)
        region_goto(next);
    
    return next;
}


/*EXTL_DOC
 * Iterate over focus history until \var{iterfn} returns \code{false}.
 * The function is called in protected mode.
 * This routine returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_EXPORT
bool ioncore_focushistory_i(ExtlFn iterfn)
{
    WRegion *next;
    
    if(ioncore_g.focus_current==NULL)
        return FALSE;
    
    /* Find the first region on focus history list that isn't currently
     * active.
     */
    for(next=ioncore_g.focus_current->active_next;
        next!=NULL; 
        next=next->active_next){
        
        if(!extl_iter_obj(iterfn, (Obj*)next))
            return FALSE;
    }
    
    return TRUE;
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


static bool region_is_parent(WRegion *reg, WRegion *aw)
{
    while(aw!=NULL){
        if(aw==reg)
            return TRUE;
        aw=REGION_PARENT_REG(aw);
    }
    
    return FALSE;
}


static bool region_is_await(WRegion *reg)
{
    return region_is_parent(reg, (WRegion*)await_watch.obj);
}


static bool region_is_focusnext(WRegion *reg)
{
    return region_is_parent(reg, ioncore_g.focus_next);
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


WRegion *ioncore_await_focus()
{
    return (WRegion*)(await_watch.obj);
}


/*}}}*/


/*{{{ Events */


void region_got_focus(WRegion *reg)
{
    WRegion *par;
    
    check_clear_await(reg);
    
    region_set_activity(reg, SETPARAM_UNSET);

    if(reg->active_sub==NULL){
        region_focuslist_push(reg);
        /*ioncore_g.focus_current=reg;*/
    }
    
    if(!REGION_IS_ACTIVE(reg)){
        D(fprintf(stderr, "got focus (inact) %s [%p]\n", OBJ_TYPESTR(reg), reg);)
        
        reg->flags|=REGION_ACTIVE;
        region_set_manager_pseudoactivity(reg);
        
        par=REGION_PARENT_REG(reg);
        if(par!=NULL){
            par->active_sub=reg;
            region_update_owned_grabs(par);
        }
        
        region_activated(reg);
        region_notify_change(reg, ioncore_g.notifies.activated);
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
    WRegion *par;
    
    if(!REGION_IS_ACTIVE(reg)){
        D(fprintf(stderr, "lost focus (inact) %s [%p:]\n", OBJ_TYPESTR(reg), reg);)
        return;
    }
    
    par=REGION_PARENT_REG(reg);
    if(par!=NULL && par->active_sub==reg){
        par->active_sub=NULL;
        region_update_owned_grabs(par);
    }


#if 0    
    if(ioncore_g.focus_current==reg){
        /* Find the closest active parent, or if none is found, stop at the
         * screen and mark it "currently focused".
         */
        while(par!=NULL && !REGION_IS_ACTIVE(par) && !OBJ_IS(par, WScreen))
            par=REGION_PARENT_REG(par);
        ioncore_g.focus_current=par;
    }
#endif

    D(fprintf(stderr, "lost focus (act) %s [%p:]\n", OBJ_TYPESTR(reg), reg);)
    
    reg->flags&=~REGION_ACTIVE;
    region_unset_manager_pseudoactivity(reg);
    
    region_inactivated(reg);
    region_notify_change(reg, ioncore_g.notifies.inactivated);
}


/*}}}*/


/*{{{ Focus status requests */


/*EXTL_DOC
 * Is \var{reg} active/does it or one of it's children of focus?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool region_is_active(WRegion *reg, bool pseudoact_ok)
{
    return (REGION_IS_ACTIVE(reg) || 
            (pseudoact_ok && REGION_IS_PSEUDOACTIVE(reg)));
}


bool region_manager_is_focusnext(WRegion *reg)
{
    if(reg==NULL || ioncore_g.focus_next==NULL)
        return FALSE;
        
    if(reg==ioncore_g.focus_next)
        return TRUE;
        
    return region_manager_is_focusnext(REGION_MANAGER(reg));
}


bool region_may_control_focus(WRegion *reg)
{
    if(OBJ_IS_BEING_DESTROYED(reg))
        return FALSE;

    if(REGION_IS_ACTIVE(reg) || REGION_IS_PSEUDOACTIVE(reg))
        return TRUE;
        
    if(region_is_await(reg) || region_is_focusnext(reg))
        return TRUE;
        
    if(region_manager_is_focusnext(reg))
        return TRUE;

    return FALSE;
}


/*}}}*/


/*{{{ set_focus, warp */


/*Time ioncore_focus_time=CurrentTime;*/


void region_finalise_focusing(WRegion* reg, Window win, bool warp, Time time, int set_input)
{
    if(warp)
        region_do_warp(reg);
    
    if(REGION_IS_ACTIVE(reg) && ioncore_await_focus()==NULL)
        return;
    
    region_set_await_focus(reg);
    if(set_input)
        XSetInputFocus(ioncore_g.dpy, win, RevertToParent, time);
}



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
    ioncore_g.focus_next_source=IONCORE_FOCUSNEXT_OTHER;
    ioncore_g.warp_next=(warp && ioncore_g.warp_enabled);
}


void region_maybewarp_now(WRegion *reg, bool warp)
{
    ioncore_g.focus_next=NULL;
    /* TODO: what if focus isn't set? Should focus_next be reset then? */
    region_do_set_focus(reg, warp && ioncore_g.warp_enabled);
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


/*{{{ Pointer focus hack */


/* This ugly hack tries to prevent focus change, when the pointer is
 * in a window to be unmapped (or destroyed), and that does not have
 * the focus, or should not soon have it.
 */
void region_pointer_focus_hack(WRegion *reg)
{
    WRegion *act;
    
    if(ioncore_g.opmode!=IONCORE_OPMODE_NORMAL)
        return;
        
    if(ioncore_g.focus_next!=NULL &&
       ioncore_g.focus_next_source<=IONCORE_FOCUSNEXT_POINTERHACK){
        return;
    }
    
    act=ioncore_await_focus();
    
    if((REGION_IS_ACTIVE(reg) && act==NULL) || !region_is_fully_mapped(reg))
        return;
    
    if(act==NULL)
        act=ioncore_g.focus_current;
    
    if(act==NULL || 
       OBJ_IS_BEING_DESTROYED(act) || 
       !region_is_fully_mapped(act) ||
       region_skip_focus(act)){
        return;
    }

    region_set_focus(act);
    ioncore_g.focus_next_source=IONCORE_FOCUSNEXT_POINTERHACK;
}


/*}}}*/

