/*
 * ion/ioncore/region.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

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


#define D2(X)


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
    
    reg->ni.name=NULL;
    reg->ni.inst_off=0;
    reg->ni.namespaceinfo=NULL;
    
    reg->manager=NULL;
    
    reg->mgd_activity=FALSE;

    region_init_name(reg);
    
    if(par!=NULL){
        reg->rootwin=((WRegion*)par)->rootwin;
        region_set_parent(reg, par);
    }else{
        assert(OBJ_IS(reg, WRootWin));/* || OBJ_IS(reg, WScreen));*/
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
    destroy_children(reg);

    if(ioncore_g.focus_next==reg){
        D(warn("Region to be focused next destroyed[1]."));
        ioncore_g.focus_next=NULL;
    }
    
    region_detach_manager(reg);
    region_unset_parent(reg);
    region_remove_bindings(reg);
    region_do_unuse_name(reg, FALSE);

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


/*{{{ Manager region dynfuns */


void region_managed_activated(WRegion *mgr, WRegion *reg)
{
    CALL_DYN(region_managed_activated, mgr, (mgr, reg));
}


void region_managed_inactivated(WRegion *mgr, WRegion *reg)
{
    CALL_DYN(region_managed_inactivated, mgr, (mgr, reg));
}


static bool region_managed_goto_default(WRegion *mgr, WRegion *reg, int flags)
{
    if(!region_is_fully_mapped(mgr))
        return FALSE;
    if(!REGION_IS_MAPPED(reg))
        region_map(reg);
    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp(reg, !(flags&REGION_GOTO_NOWARP));
    return TRUE;
}

    
bool region_managed_goto(WRegion *mgr, WRegion *reg, int flags)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, region_managed_goto, mgr, (mgr, reg, flags));
    return ret;
}


void region_managed_notify(WRegion *mgr, WRegion *reg)
{
    CALL_DYN(region_managed_notify, mgr, (mgr, reg));
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


void region_manager_changed(WRegion *reg, WRegion *mgr_or_null)
{
    CALL_DYN(region_child_removed, reg, (reg, mgr_or_null));
}


/*}}}*/


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



/*{{{ Goto */


static bool region_do_goto(WRegion *reg, int flags)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    if(mgr!=NULL){
        if(!region_do_goto(mgr, flags))
            return FALSE;
        return region_managed_goto(mgr, reg, flags);
    }else{
        WRegion *par=REGION_PARENT_REG(reg);
        if(par!=NULL){
            if(!region_do_goto(par, flags))
                return FALSE;
        }

        region_map(reg);
        if(flags&REGION_GOTO_FOCUS)
            region_maybewarp(reg, !(flags&REGION_GOTO_NOWARP));
        
        return TRUE;
    }
}


bool region_goto_flags(WRegion *reg, int flags)
{
    bool ret=FALSE;

    ioncore_set_previous_of(reg);
    ioncore_protect_previous();
    region_do_goto(reg, flags);
    ioncore_unprotect_previous();
    
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
    fp.mode=mode;
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
    bool refuse=TRUE;
    
    if((!relocate && !region_may_destroy(reg)) ||
       !region_manager_allows_destroying(reg)){
        return FALSE;
    }
    
    ph=region_get_rescue_pholder(reg);
    
    if(ph!=NULL){
        refuse=!region_rescue_clientwins(reg, ph);
        destroy_obj((Obj*)ph);
    }

    if(refuse){
        warn(TR("Failed to rescue some client windows - not closing."));
        return FALSE;
    }

    mainloop_defer_destroy((Obj*)reg);
    
    return TRUE;
}


/*EXTL_DOC
 * Attempt to close/destroy \var{reg}. Whether this operation works
 * depends on whether the particular type of region in question has
 * implemented the feature and, in case of client windows, whether
 * the client supports the \code{WM_DELETE} protocol (see also
 * \fnref{WClientWin.kill}). If the operations is likely to succeed,
 * \code{true} is returned, otherwise \code{false}. In most cases the
 * region will not have been actually destroyed when this function returns.
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
 * \fnref{WRegion.rqclose_propagate}.
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


/*}}}*/


/*{{{ Manager/parent stuff */


/* Routine to call to unmanage a region */
void region_detach_manager(WRegion *reg)
{
    WRegion *mgr;
    
    mgr=REGION_MANAGER(reg);
    
    if(mgr==NULL)
        return;
    
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
            /*if(region_xwindow(mgr)!=None){*/
                region_do_set_focus(mgr, FALSE);
            /*}*/
        }
    }

    region_clear_activity(reg, TRUE);

    region_managed_remove(mgr, reg);

    assert(REGION_MANAGER(reg)==NULL);
}


/* This should only be called within region_managed_remove */
void region_unset_manager(WRegion *reg, WRegion *mgr)
{
    if(reg->manager!=mgr)
        return;
    reg->manager=NULL;
    
    if(!OBJ_IS_BEING_DESTROYED(reg))
        region_manager_changed(reg, NULL);
}


void region_set_manager(WRegion *reg, WRegion *mgr)
{
    assert(reg->manager==NULL);
    
    reg->manager=mgr;
    
    region_manager_changed(reg, mgr);
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

    if(p->active_sub==reg)
        p->active_sub=NULL;
    
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


/*{{{ Stacking */


void region_raise(WRegion *reg)
{
    region_restack(reg, None, Above);
}


void region_lower(WRegion *reg)
{
    region_restack(reg, None, Below);
}


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


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns the root window \var{reg} is on.
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


void region_rootgeom(WRegion *reg, int *xret, int *yret)
{
    *xret=REGION_GEOM(reg).x;
    *yret=REGION_GEOM(reg).y;
    
    while(1){
        reg=REGION_PARENT_REG(reg);
        if(reg==NULL)
            break;
        *xret+=REGION_GEOM(reg).x;
        *yret+=REGION_GEOM(reg).y;
    }
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


void region_notify_change(WRegion *reg)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    if(mgr!=NULL)
        region_managed_notify(mgr, reg);
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


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab region_dynfuntab[]={
    {region_managed_rqgeom,
     region_managed_rqgeom_allow},
    
    {region_updategr, 
     region_updategr_default},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)region_rescue_child_clientwins},

    {(DynFun*)region_manage_clientwin,
     (DynFun*)region_manage_clientwin_default},

    {(DynFun*)region_managed_goto,
     (DynFun*)region_managed_goto_default},

    {(DynFun*)region_rqclose_propagate,
     (DynFun*)region_rqclose_propagate_default},

    {(DynFun*)region_rqclose,
     (DynFun*)region_rqclose_default},
    
    {region_stacking, 
     region_stacking_default},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WRegion, Obj, region_deinit, region_dynfuntab);

    
/*}}}*/

