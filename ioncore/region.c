/*
 * ion/ioncore/region.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include <libtu/objp.h>
#include "region.h"
#include "focus.h"
#include "regbind.h"
#include "names.h"
#include "stacking.h"
#include "resize.h"
#include "manage.h"
#include "extl.h"
#include "extlconv.h"
#include "activity.h"
#include "region-iter.h"


#define D2(X)


/*{{{ Init & deinit */


void region_init(WRegion *reg, WWindow *par, const WFitParams *fp)
{
    if(fp->g.w<0 || fp->g.h<0)
        warn("Creating region with negative width or height!");
    
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
    reg->ni.namespaceinfo=NULL;
    
    reg->manager=NULL;
    reg->mgr_next=NULL;
    reg->mgr_prev=NULL;
    
    reg->stacking.below_list=NULL;
    reg->stacking.next=NULL;
    reg->stacking.prev=NULL;
    reg->stacking.above=NULL;
    
    reg->mgd_activity=FALSE;

    region_init_name(reg);
    
    if(par!=NULL){
        reg->rootwin=((WRegion*)par)->rootwin;
        region_set_parent(reg, (WRegion*)par);
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
            warn("Destroying object \"%s\" with client windows as children.", 
                 region_name(reg));
            complained=TRUE;
        }
        prev=sub;
        destroy_obj((Obj*)sub);
    }
}


void region_deinit(WRegion *reg)
{
    /*rescue_child_clientwins(reg);*/
    
    destroy_children(reg);

    if(ioncore_g.focus_next==reg){
        D(warn("Region to be focused next destroyed[1]."));
        ioncore_g.focus_next=NULL;
    }
    
    region_detach(reg);
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
bool region_rqclose(WRegion *reg)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_rqclose, reg, (reg));
    return ret;
}


bool region_managed_may_destroy(WRegion *mgr, WRegion *reg)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, region_managed_may_destroy, mgr, (mgr, reg));
    return ret;
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


bool region_managed_display(WRegion *mgr, WRegion *reg)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, region_managed_display, mgr, (mgr, reg));
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


WRegion *region_managed_control_focus(WRegion *mgr, WRegion *reg)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_managed_control_focus, mgr, (mgr, reg));
    return ret;
}


/*EXTL_DOC
 * Return the object, if any, that is considered ''currently active''
 * within the objects managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
WRegion *region_current(WRegion *mgr)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_current, mgr, (mgr));
    return ret;
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


/*{{{ Detach */


void region_detach_parent(WRegion *reg)
{
    WRegion *p=reg->parent;

    region_reset_stacking(reg);

    if(p==NULL || p==reg)
        return;
    
    UNLINK_ITEM(p->children, reg, p_next, p_prev);
    reg->parent=NULL;

    if(p->active_sub==reg){
        p->active_sub=NULL;
        
        /* Removed: seems to confuse floatws:s when frames are
         * destroyd.
         */
        /*if(REGION_IS_ACTIVE(reg) && ioncore_g.focus_next==NULL)
            set_focus(p);*/
    }
}


void region_detach_manager(WRegion *reg)
{
    WRegion *mgr;
    
    mgr=REGION_MANAGER(reg);
    
    if(mgr==NULL)
        return;
    
    D2(fprintf(stderr, "detach %s (mgr:%s)\n", OBJ_TYPESTR(reg),
               OBJ_TYPESTR(mgr)));
    
    /* Restore activity state to non-parent manager */
    if(region_may_control_focus(reg)){
        WRegion *par=region_parent(reg);
        if(par!=NULL && mgr!=par && region_parent(mgr)==par){
            /* REGION_ACTIVE shouldn't be set for windowless regions
             * but make the parent's active_sub point to it
             * nevertheless so that region_may_control_focus can
             * be made to work.
             */
            D2(fprintf(stderr, "detach mgr %s, %s->active_sub=%s\n",
                       OBJ_TYPESTR(reg), OBJ_TYPESTR(par),
                       OBJ_TYPESTR(mgr)));
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


void region_detach(WRegion *reg)
{
    region_detach_manager(reg);
    region_detach_parent(reg);
}


/*}}}*/


/*{{{ Goto */


/*EXTL_DOC
 * Attempt to display \var{reg}.
 */
EXTL_EXPORT_MEMBER
bool region_display(WRegion *reg)
{
    WRegion *mgr, *preg;

    if(region_is_fully_mapped(reg))
        return TRUE;
    
    mgr=REGION_MANAGER(reg);
    
    if(mgr!=NULL){
        if(!region_display(mgr))
            return FALSE;
        return region_managed_display(mgr, reg);
    }
    
    preg=region_parent(reg);

    if(preg!=NULL && !region_display(preg))
        return FALSE;

    region_map(reg);
    return TRUE;
}


/*EXTL_DOC
 * Attempt to display \var{reg} and save the current region
 * activity status for use by \fnref{goto_previous}.
 */
EXTL_EXPORT_MEMBER
bool region_display_sp(WRegion *reg)
{
    bool ret;
    
    ioncore_set_previous_of(reg);
    ioncore_protect_previous();
    ret=region_display(reg);
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
    bool ret=FALSE;

    ioncore_set_previous_of(reg);
    ioncore_protect_previous();
    if(region_display(reg))
        ret=(reg==region_set_focus_mgrctl(reg, TRUE));
    ioncore_unprotect_previous();
    
    return ret;
}


/*}}}*/


/*{{{ Helpers/misc */


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
    if(maybe_sub==NULL)
        maybe_sub=region_current(reg);
    if(maybe_sub!=NULL)
        return region_rqclose_propagate(maybe_sub, NULL);
    return (region_rqclose(reg) ? reg : NULL);
}


/*EXTL_DOC
 * Is \var{reg} visible/is it and all it's ancestors mapped?
 */
EXTL_EXPORT_AS(WRegion, is_mapped)
bool region_is_fully_mapped(WRegion *reg)
{
    for(; reg!=NULL; reg=region_parent(reg)){
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
        reg=region_parent(reg);
        if(reg==NULL)
            break;
        *xret+=REGION_GEOM(reg).x;
        *yret+=REGION_GEOM(reg).y;
    }
}


void region_rootpos(WRegion *reg, int *xret, int *yret)
{
    WRegion *par;

    par=region_parent(reg);
    
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


void region_set_manager(WRegion *reg, WRegion *mgr, WRegion **listptr)
{
    assert(reg->manager==NULL);
    
    reg->manager=mgr;
    if(listptr!=NULL){
        LINK_ITEM(*listptr, reg, mgr_next, mgr_prev);
    }
}


void region_unset_manager(WRegion *reg, WRegion *mgr, WRegion **listptr)
{
    if(reg->manager!=mgr)
        return;
    reg->manager=NULL;
    if(listptr!=NULL){
        UNLINK_ITEM(*listptr, reg, mgr_next, mgr_prev);
    }
}


void region_set_parent(WRegion *reg, WRegion *parent)
{
    assert(reg->parent==NULL && parent!=NULL);
    LINK_ITEM(parent->children, reg, p_next, p_prev);
    reg->parent=parent;
}


void region_attach_parent(WRegion *reg, WRegion *parent)
{
    region_set_parent(reg, parent);
    region_raise(reg);
}


/*EXTL_DOC
 * Returns the region that manages \var{reg}.
 */
EXTL_EXPORT_MEMBER
WRegion *region_manager(WRegion *reg)
{
    return reg->manager;
}


/*EXTL_DOC
 * Returns the parent region of \var{reg}.
 */
EXTL_EXPORT_MEMBER
WRegion *region_parent(WRegion *reg)
{
    return reg->parent;
}


WRegion *region_manager_or_parent(WRegion *reg)
{
    if(reg->manager!=NULL)
        return reg->manager;
    else
        return reg->parent;
}


/*EXTL_DOC
 * Returns the geometry of \var{reg} within its parent; a table with fields
 * \var{x}, \var{y}, \var{w} and \var{h}.
 */
EXTL_EXPORT_MEMBER
ExtlTab region_geom(WRegion *reg)
{
    return extl_table_from_rectangle(&REGION_GEOM(reg));
}


bool region_may_destroy(WRegion *reg)
{
    WRegion *mgr=REGION_MANAGER(reg);
    if(mgr==NULL)
        return TRUE;
    else
        return region_managed_may_destroy(mgr, reg);
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


/*{{{ Misc. */


/*EXTL_DOC
 * Returns the root window \var{reg} is on.
 */
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
EXTL_EXPORT_MEMBER
WScreen *region_screen_of(WRegion *reg)
{
    while(reg!=NULL){
        if(OBJ_IS(reg, WScreen))
            return (WScreen*)reg;
        reg=region_parent(reg);
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

    {(DynFun*)region_manage_rescue,
     (DynFun*)region_manage_rescue_default},
    
    END_DYNFUNTAB
};


IMPLCLASS(WRegion, Obj, region_deinit, region_dynfuntab);

    
/*}}}*/

