/*
 * ion/ioncore/region.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libextl/extl.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "focus.h"
#include "regbind.h"
#include "names.h"
#include "resize.h"
#include "manage.h"
#include "extlconv.h"
#include "activity.h"
#include "region-iter.h"
#include "return.h"


#define D2(X)


WHook *region_notify_hook=NULL;


static void region_notify_change_(WRegion *reg, WRegionNotify how);


/*{{{ Init & deinit */


void region_init(WRegion *reg, WWindow *par, const WFitParams *fp)
{
    if(fp->g.w<0 || fp->g.h<0)
        warn(TR("Creating region with negative width or height!"));
    
    reg->geom=fp->g;
    reg->flags=0;
    reg->bindings=NULL;
    reg->rootwin=NULL;
    
    reg->children=NULL;
    reg->parent=NULL;
    reg->p_next=NULL;
    reg->p_prev=NULL;
    
    reg->active_sub=NULL;
    reg->active_prev=NULL;
    reg->active_next=NULL;
    
    reg->ni.name=NULL;
    reg->ni.inst_off=0;
    reg->ni.node=NULL;
    
    reg->manager=NULL;
    
    reg->submapstat=NULL;
    
    reg->mgd_activity=FALSE;

    if(par!=NULL){
        reg->rootwin=((WRegion*)par)->rootwin;
        region_set_parent(reg, par);
    }else{
        assert(OBJ_IS(reg, WRootWin));
    }
}


static void destroy_children(WRegion *reg)
{
    WRegion *sub, *prev=NULL;
    bool complained=FALSE;
    
    /* destroy children */
    while(1){
        sub=reg->children;
        if(sub==NULL)
            break;
        assert(!OBJ_IS_BEING_DESTROYED(sub));
        assert(sub!=prev);
        if(ioncore_g.opmode!=IONCORE_OPMODE_DEINIT && !complained && OBJ_IS(reg, WClientWin)){
            warn(TR("Destroying object \"%s\" with client windows as "
                    "children."), region_name(reg));
            complained=TRUE;
        }
        prev=sub;
        destroy_obj((Obj*)sub);
    }
}


void region_deinit(WRegion *reg)
{
    region_notify_change(reg, ioncore_g.notifies.deinit);
    
    destroy_children(reg);

    if(ioncore_g.focus_next==reg){
        D(warn("Region to be focused next destroyed[1]."));
        ioncore_g.focus_next=NULL;
    }

    region_detach_manager(reg);
    region_unset_return(reg);
    region_unset_parent(reg);
    region_remove_bindings(reg);
    
    region_unregister(reg);

    region_focuslist_deinit(reg);

    if(ioncore_g.focus_next==reg){
        D(warn("Region to be focused next destroyed[2]."));
        ioncore_g.focus_next=NULL;
    }
}


/*}}}*/


/*{{{ Dynfuns */


bool region_fitrep(WRegion *reg, WWindow *par, const WFitParams *fp)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_fitrep, reg, (reg, par, fp));
    return ret;
}


void region_updategr(WRegion *reg)
{
    CALL_DYN(region_updategr, reg, (reg));
}


void region_map(WRegion *reg)
{
    CALL_DYN(region_map, reg, (reg));
}


void region_unmap(WRegion *reg)
{
    CALL_DYN(region_unmap, reg, (reg));
}


void region_notify_rootpos(WRegion *reg, int x, int y)
{
    CALL_DYN(region_notify_rootpos, reg, (reg, x, y));
}


Window region_xwindow(const WRegion *reg)
{
    Window ret=None;
    CALL_DYN_RET(ret, Window, region_xwindow, reg, (reg));
    return ret;
}


void region_activated(WRegion *reg)
{
    CALL_DYN(region_activated, reg, (reg));
}


void region_inactivated(WRegion *reg)
{
    CALL_DYN(region_inactivated, reg, (reg));
}


void region_do_set_focus(WRegion *reg, bool warp)
{
    CALL_DYN(region_do_set_focus, reg, (reg, warp));
}


/*{{{ Manager region dynfuns */


static bool region_managed_prepare_focus_default(WRegion *mgr, WRegion *reg, 
                                                 int flags, 
                                                 WPrepareFocusResult *res)
{
    if(!region_prepare_focus(mgr, flags, res))
        return FALSE;
    
    res->reg=reg;
    res->flags=flags;
    return TRUE;
}

    
bool region_managed_prepare_focus(WRegion *mgr, WRegion *reg, 
                                  int flags, 
                                  WPrepareFocusResult *res)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, region_managed_prepare_focus, mgr, 
                 (mgr, reg, flags, res));
    return ret;
}


void region_managed_notify(WRegion *mgr, WRegion *reg, WRegionNotify how)
{
    CALL_DYN(region_managed_notify, mgr, (mgr, reg, how));
}


void region_managed_remove(WRegion *mgr, WRegion *reg)
{
    CALL_DYN(region_managed_remove, mgr, (mgr, reg));
}


/*EXTL_DOC
 * Return the object, if any, that is considered ''currently active''
 * within the objects managed by \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *region_current(WRegion *mgr)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_current, mgr, (mgr));
    return ret;
}


void region_child_removed(WRegion *reg, WRegion *sub)
{
    CALL_DYN(region_child_removed, reg, (reg, sub));
}


/*}}}*/


/*{{{ Dynfun defaults */


void region_updategr_default(WRegion *reg)
{
    WRegion *sub=NULL;
    
    FOR_ALL_CHILDREN(reg, sub){
        region_updategr(sub);
    }
}


/*}}}*/


/*}}}*/


/*{{{ Goto */


bool region_prepare_focus(WRegion *reg, int flags, 
                          WPrepareFocusResult *res)
{
    WRegion *mgr=REGION_MANAGER(reg);
    WRegion *par=REGION_PARENT_REG(reg);

    if(REGION_IS_MAPPED(reg) && region_may_control_focus(reg)){
        res->reg=reg;
        res->flags=0;
        return TRUE;
    }else{
        if(mgr!=NULL){
            return region_managed_prepare_focus(mgr, reg, flags, res);
        }else if(par!=NULL){
            if(!region_prepare_focus(par, flags, res))
                return FALSE;
            /* Just focus reg, if it has no manager, and parent can be 
             * focused. 
             */
        }
        res->reg=reg;
        res->flags=flags;
        return TRUE;
    }
}


bool region_goto_flags(WRegion *reg, int flags)
{
    WPrepareFocusResult res;
    bool ret;
    
    ret=region_prepare_focus(reg, flags, &res);
    
    if(res.reg!=NULL){
        if(res.flags&REGION_GOTO_FOCUS)
            region_maybewarp(res.reg, !(res.flags&REGION_GOTO_NOWARP));
    }
    
    return ret;
}


/*EXTL_DOC
 * Attempt to display \var{reg}, save region activity status and then
 * warp to (or simply set focus to if warping is disabled) \var{reg}.
 * 
 * Note that this function is asynchronous; the region will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT_MEMBER
bool region_goto(WRegion *reg)
{
    return region_goto_flags(reg, REGION_GOTO_FOCUS);
}


/*}}}*/


/*{{{ Fit/reparent */


void region_fit(WRegion *reg, const WRectangle *geom, WRegionFitMode mode)
{
    WFitParams fp;
    fp.g=*geom;
    fp.mode=mode&~REGION_FIT_GRAVITY;
    fp.gravity=ForgetGravity;
    region_fitrep(reg, NULL, &fp);
}


bool region_reparent(WRegion *reg, WWindow *par,
                     const WRectangle *geom, WRegionFitMode mode)
{
    WFitParams fp;
    fp.g=*geom;
    fp.mode=mode;
    return region_fitrep(reg, par, &fp);
}


/*}}}*/


/*{{{ Close */


static bool region_rqclose_default(WRegion *reg, bool relocate)
{
    WPHolder *ph;
    bool refuse=TRUE, mcf;
    
    if((!relocate && !region_may_destroy(reg)) ||
       !region_manager_allows_destroying(reg)){
        return FALSE;
    }
    
    ph=region_get_rescue_pholder(reg);
    mcf=region_may_control_focus(reg);
    
    if(ph!=NULL){
        refuse=!region_rescue_clientwins(reg, ph);
        destroy_obj((Obj*)ph);
    }

    if(refuse){
        warn(TR("Failed to rescue some client windows - not closing."));
        return FALSE;
    }

    region_dispose(reg, mcf);
    
    return TRUE;
}


/*EXTL_DOC
 * Attempt to close/destroy \var{reg}. Whether this operation works
 * depends on whether the particular type of region in question has
 * implemented the feature and, in case of client windows, whether
 * the client supports the \code{WM_DELETE} protocol (see also
 * \fnref{WClientWin.kill}). If the operation is likely to succeed,
 * \code{true} is returned, otherwise \code{false}. In most cases the
 * region will not have been actually destroyed when this function returns.
 * If \var{relocate} is not set, and \var{reg} manages other regions, it
 * will not be closed. Otherwise the managed regions will be attempted
 * to be relocated.
 */
EXTL_EXPORT_MEMBER
bool region_rqclose(WRegion *reg, bool relocate)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_rqclose, reg, (reg, relocate));
    return ret;
}


static WRegion *region_rqclose_propagate_default(WRegion *reg, 
                                                 WRegion *maybe_sub)
{
    if(maybe_sub==NULL)
        maybe_sub=region_current(reg);
    if(maybe_sub!=NULL)
        return region_rqclose_propagate(maybe_sub, NULL);
    return (region_rqclose(reg, FALSE) ? reg : NULL);
}


/*EXTL_DOC
 * Recursively attempt to close a region or one of the regions managed by 
 * it. If \var{sub} is set, it will be used as the managed region, otherwise
 * \fnref{WRegion.current}\code{(reg)}. The object to be closed is
 * returned or NULL if nothing can be closed. Also see notes for
 * \fnref{WRegion.rqclose}.
 */
EXTL_EXPORT_MEMBER
WRegion *region_rqclose_propagate(WRegion *reg, WRegion *maybe_sub)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_rqclose_propagate, reg,
                 (reg, maybe_sub));
    return ret;
}


bool region_may_destroy(WRegion *reg)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, region_may_destroy, reg, (reg));
    return ret;
}


bool region_managed_may_destroy(WRegion *mgr, WRegion *reg)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, region_managed_may_destroy, mgr, (mgr, reg));
    return ret;
}


bool region_manager_allows_destroying(WRegion *reg)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    if(mgr==NULL)
        return TRUE;
    
    return region_managed_may_destroy(mgr, reg);
}


void region_dispose(WRegion *reg, bool was_mcf)
{
    if(was_mcf){
        WPHolder *ph=region_get_return(reg);
        if(ph!=NULL)
            pholder_goto(ph);
    }

    mainloop_defer_destroy((Obj*)reg);
}


void region_dispose_(WRegion *reg)
{
    region_dispose(reg, region_may_control_focus(reg));
}


/*}}}*/


/*{{{ Manager/parent stuff */


/* Routine to call to unmanage a region */
void region_detach_manager(WRegion *reg)
{
    WRegion *mgr;
    
    mgr=REGION_MANAGER(reg);
    
    if(mgr==NULL)
        return;
    
#if 0
    /* Restore activity state to non-parent manager */
    if(region_may_control_focus(reg)){
        WRegion *par=REGION_PARENT_REG(reg);
        if(par!=NULL && mgr!=par && REGION_PARENT_REG(mgr)==par){
            /* REGION_ACTIVE shouldn't be set for windowless regions
             * but make the parent's active_sub point to it
             * nevertheless so that region_may_control_focus can
             * be made to work.
             */
            par->active_sub=mgr;
            region_maybewarp_now(mgr, FALSE);
        }
    }
#endif

    region_set_activity(reg, SETPARAM_UNSET);

    region_managed_remove(mgr, reg);

    assert(REGION_MANAGER(reg)==NULL);
}


void region_unset_manager_pseudoactivity(WRegion *reg)
{
    WRegion *mgr=reg->manager, *par=REGION_PARENT_REG(reg);
    
    if(mgr==NULL || mgr==par || !REGION_IS_PSEUDOACTIVE(mgr))
        return;
        
    mgr->flags&=~REGION_PSEUDOACTIVE;
    
    region_notify_change(mgr, ioncore_g.notifies.pseudoinactivated);
    
    region_unset_manager_pseudoactivity(mgr);
}


void region_set_manager_pseudoactivity(WRegion *reg)
{
    WRegion *mgr=reg->manager, *par=REGION_PARENT_REG(reg);
    
    if(!REGION_IS_ACTIVE(reg) && !REGION_IS_PSEUDOACTIVE(reg))
        return;
        
    if(mgr==NULL || mgr==par || REGION_IS_PSEUDOACTIVE(mgr))
        return;
    
    mgr->flags|=REGION_PSEUDOACTIVE;
    
    region_notify_change(mgr, ioncore_g.notifies.pseudoactivated);
    
    region_set_manager_pseudoactivity(mgr);
}


/* This should only be called within region_managed_remove,
 * _after_ any managed lists and other essential structures
 * of mgr have been broken.
 */
void region_unset_manager(WRegion *reg, WRegion *mgr)
{
    if(reg->manager!=mgr)
        return;
        
    region_notify_change_(reg, ioncore_g.notifies.unset_manager);
    
    region_unset_manager_pseudoactivity(reg);
    
    reg->manager=NULL;
    
    if(region_is_activity_r(reg))
        region_clear_mgd_activity(mgr);
    
    region_unset_return(reg);
}


/* This should be called within region attach routines,
 * _after_ any managed lists and other essential structures
 * of mgr have been set up.
 */
void region_set_manager(WRegion *reg, WRegion *mgr)
{
    assert(reg->manager==NULL);
    
    reg->manager=mgr;
    
    region_set_manager_pseudoactivity(reg);
    
    if(region_is_activity_r(reg))
        region_mark_mgd_activity(mgr);
    
    region_notify_change_(reg, ioncore_g.notifies.set_manager);
}


void region_set_parent(WRegion *reg, WWindow *parent)
{
    assert(reg->parent==NULL && parent!=NULL);
    LINK_ITEM(((WRegion*)parent)->children, reg, p_next, p_prev);
    reg->parent=parent;
}


void region_unset_parent(WRegion *reg)
{
    WRegion *p=REGION_PARENT_REG(reg);

    if(p==NULL || p==reg)
        return;

    UNLINK_ITEM(p->children, reg, p_next, p_prev);
    reg->parent=NULL;

    if(p->active_sub==reg){
        p->active_sub=NULL;
        region_update_owned_grabs(p);
    }
    
    region_child_removed(p, reg);
}


/*EXTL_DOC
 * Returns the region that manages \var{reg}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *region_manager(WRegion *reg)
{
    return reg->manager;
}


/*EXTL_DOC
 * Returns the parent region of \var{reg}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WWindow *region_parent(WRegion *reg)
{
    return reg->parent;
}


WRegion *region_manager_or_parent(WRegion *reg)
{
    if(reg->manager!=NULL)
        return reg->manager;
    else
        return (WRegion*)(reg->parent);
}


WRegion *region_get_manager_chk(WRegion *p, const ClassDescr *descr)
{
    WRegion *mgr=NULL;
    
    if(p!=NULL){
        mgr=REGION_MANAGER(p);
        if(obj_is((Obj*)mgr, descr))
            return mgr;
    }
    
    return NULL;
}

/*}}}*/


/*{{{ Stacking and ordering */


static void region_stacking_default(WRegion *reg, 
                                    Window *bottomret, Window *topret)
{
    Window win=region_xwindow(reg);
    *bottomret=win;
    *topret=win;
}


void region_stacking(WRegion *reg, Window *bottomret, Window *topret)
{
    CALL_DYN(region_stacking, reg, (reg, bottomret, topret));
}


void region_restack(WRegion *reg, Window other, int mode)
{
    CALL_DYN(region_restack, reg, (reg, other, mode));
}



bool region_managed_rqorder(WRegion *reg, WRegion *sub, WRegionOrder order)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_managed_rqorder, reg, (reg, sub, order));
    return ret;
}


bool region_rqorder(WRegion *reg, WRegionOrder order)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    if(mgr==NULL)
        return FALSE;
    else
        return region_managed_rqorder(mgr, reg, order);
}


/*EXTL_DOC
 * Request ordering. Currently supported values for \var{ord}
 * are 'front' and 'back'.
 */
EXTL_EXPORT_AS(WRegion, rqorder)
bool region_rqorder_extl(WRegion *reg, const char *ord)
{
    WRegionOrder order;
    
    if(strcmp(ord, "front")==0){
        order=REGION_ORDER_FRONT;
    }else if(strcmp(ord, "back")==0){
        order=REGION_ORDER_BACK;
    }else{
        return FALSE;
    }
    
    return region_rqorder(reg, order);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns the root window \var{reg} is on.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRootWin *region_rootwin_of(const WRegion *reg)
{
    WRootWin *rw;
    assert(reg!=NULL); /* Lua interface should not pass NULL reg. */
    rw=(WRootWin*)(reg->rootwin);
    assert(rw!=NULL);
    return rw;
}


/*EXTL_DOC
 * Returns the screen \var{reg} is on.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WScreen *region_screen_of(WRegion *reg)
{
    while(reg!=NULL){
        if(OBJ_IS(reg, WScreen))
            return (WScreen*)reg;
        reg=REGION_PARENT_REG(reg);
    }
    return NULL;
}


Window region_root_of(const WRegion *reg)
{
    return WROOTWIN_ROOT(region_rootwin_of(reg));
}


bool region_same_rootwin(const WRegion *reg1, const WRegion *reg2)
{
    return (reg1->rootwin==reg2->rootwin);
}


/*EXTL_DOC
 * Is \var{reg} visible/is it and all it's ancestors mapped?
 */
EXTL_SAFE
EXTL_EXPORT_AS(WRegion, is_mapped)
bool region_is_fully_mapped(WRegion *reg)
{
    for(; reg!=NULL; reg=REGION_PARENT_REG(reg)){
        if(!REGION_IS_MAPPED(reg))
            return FALSE;
    }
    
    return TRUE;
}


void region_rootpos(WRegion *reg, int *xret, int *yret)
{
    WRegion *par;

    par=REGION_PARENT_REG(reg);
    
    if(par==NULL || par==reg){
        *xret=0;
        *yret=0;
        return;
    }
    
    region_rootpos(par, xret, yret);
    
    *xret+=REGION_GEOM(reg).x;
    *yret+=REGION_GEOM(reg).y;
}


typedef struct{
    WRegion *reg;
    WRegionNotify how;
} MRSHP;


static bool mrsh_notify_change(WHookDummy *fn, void *p_)
{
    MRSHP *p=(MRSHP*)p_;
    
    fn(p->reg, p->how);
    
    return TRUE;
}


static bool mrshe_notify_change(ExtlFn fn, void *p_)
{
    MRSHP *p=(MRSHP*)p_;
    
    extl_call(fn, "os", NULL, p->reg, stringstore_get(p->how));
    
    return TRUE;
}


static void region_notify_change_(WRegion *reg, WRegionNotify how)
{
    MRSHP p;
    
    p.reg=reg;
    p.how=how;
    
    extl_protect(NULL);
    hook_call(region_notify_hook, &p, mrsh_notify_change, mrshe_notify_change),
    extl_unprotect(NULL);
}


void region_notify_change(WRegion *reg, WRegionNotify how)
{
    WRegion *mgr=REGION_MANAGER(reg);

    if(mgr!=NULL)
        region_managed_notify(mgr, reg, how);
    
    region_notify_change_(reg, how);
}


/*EXTL_DOC
 * Returns the geometry of \var{reg} within its parent; a table with fields
 * \var{x}, \var{y}, \var{w} and \var{h}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab region_geom(WRegion *reg)
{
    return extl_table_from_rectangle(&REGION_GEOM(reg));
}


bool region_handle_drop(WRegion *reg, int x, int y, WRegion *dropped)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_handle_drop, reg, (reg, x, y, dropped));
    return ret;
}


WRegion *region_managed_within(WRegion *reg, WRegion *mgd)
{
    while(mgd!=NULL && 
          (REGION_PARENT_REG(mgd)==reg ||
           REGION_PARENT_REG(mgd)==REGION_PARENT_REG(reg))){
        
        if(REGION_MANAGER(mgd)==reg)
            return mgd;
        mgd=REGION_MANAGER(mgd);
    }
    
    return NULL;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab region_dynfuntab[]={
    {region_managed_rqgeom,
     region_managed_rqgeom_allow},
     
    {region_managed_rqgeom_absolute,
     region_managed_rqgeom_absolute_default},
    
    {region_updategr, 
     region_updategr_default},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)region_rescue_child_clientwins},

    {(DynFun*)region_prepare_manage,
     (DynFun*)region_prepare_manage_default},

    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)region_prepare_manage_transient_default},

    {(DynFun*)region_managed_prepare_focus,
     (DynFun*)region_managed_prepare_focus_default},

    {(DynFun*)region_rqclose_propagate,
     (DynFun*)region_rqclose_propagate_default},

    {(DynFun*)region_rqclose,
     (DynFun*)region_rqclose_default},
    
    {(DynFun*)region_displayname,
     (DynFun*)region_name},
    
    {region_stacking, 
     region_stacking_default},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WRegion, Obj, region_deinit, region_dynfuntab);

    
/*}}}*/

