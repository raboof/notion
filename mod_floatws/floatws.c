/*
 * ion/mod_floatws/floatws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/minmax.h>
#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/manage.h>
#include <ioncore/screen.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/regbind.h>
#include <ioncore/frame-pointer.h>
#include <ioncore/stacking.h>
#include <ioncore/extlconv.h>
#include <ioncore/defer.h>
#include <ioncore/region-iter.h>
#include <ioncore/xwindow.h>

#include "floatws.h"
#include "floatframe.h"
#include "placement.h"
#include "main.h"


ObjList *floatws_sticky_list=NULL;


/*{{{ region dynfun implementations */


static void floatws_fit(WFloatWS *ws, const WRectangle *geom)
{
    REGION_GEOM(ws)=*geom;
}


bool floatws_fitrep(WFloatWS *ws, WWindow *par, const WFitParams *fp)
{
    WRegion *sub, *next;
    bool rs;
    int xdiff, ydiff;
    
    if(par==NULL){
        REGION_GEOM(ws)=fp->g;
        return TRUE;
    }

    if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
        return FALSE;

    genws_do_reparent(&(ws->genws), par, fp);
    
    xdiff=fp->g.x-REGION_GEOM(ws).x;
    ydiff=fp->g.y-REGION_GEOM(ws).y;
    
    FOR_ALL_MANAGED_ON_LIST_W_NEXT(ws->managed_list, sub, next){
        WRectangle g=REGION_GEOM(sub);
        g.x+=xdiff;
        g.y+=ydiff;
        if(!region_reparent(sub, par, &g, REGION_FIT_EXACT)){
            warn("Problem: can't reparent a %s managed by a WFloatWS"
                 "being reparented. Detaching from this object.",
                 OBJ_TYPESTR(sub));
            region_detach_manager(sub);
        }
    }
    
    return TRUE;
}


static void move_sticky(WFloatWS *ws)
{
    WRegion *reg, *par=REGION_PARENT(ws);
    
    if(par==NULL || !OBJ_IS(par, WMPlex))
        return;
    
    FOR_ALL_ON_OBJLIST(WRegion*, reg, floatws_sticky_list){
        WFloatWS *rmgr;
        if(REGION_PARENT(reg)!=par)
            continue;
        rmgr=REGION_MANAGER_CHK(reg, WFloatWS);
        if(rmgr==NULL)
            continue;
        if(rmgr->current_managed==reg)
            rmgr->current_managed=NULL;
        region_unset_manager(reg, (WRegion*)rmgr, &(rmgr->managed_list));
        region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
        if(REGION_IS_ACTIVE(reg) && ws->current_managed==NULL)
            ws->current_managed=reg;
    }
}


static void floatws_map(WFloatWS *ws)
{
    WRegion *reg;

    genws_do_map(&(ws->genws));
    
    move_sticky(ws);

    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        region_map(reg);
    }
}


static void floatws_unmap(WFloatWS *ws)
{
    WRegion *reg;

    genws_do_unmap(&(ws->genws));

    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        region_unmap(reg);
    }
}


static WRegion *floatws_get_focus_to(WFloatWS *ws)
{
    WRegion *next=NULL;
    WWindow *par=REGION_PARENT_CHK(ws, WWindow);
    
    if(par!=NULL){
        Window root=None, parent=None, *children=NULL;
        uint nchildren=0;
        WRegion *r;
        XQueryTree(ioncore_g.dpy, region_xwindow((WRegion*)par),
                   &root, &parent, &children, &nchildren);
        while(nchildren>0){
            nchildren--;
            r=xwindow_region_of(children[nchildren]);
            if(r!=NULL && REGION_MANAGER(r)==(WRegion*)ws){
                next=r;
                break;
            }
        }
        XFree(children);
    }
    
    return (next ? next : ws->managed_list);
}


static void floatws_do_set_focus(WFloatWS *ws, bool warp)
{
    WRegion *r=ws->current_managed;
        
    if(r==NULL)
        r=floatws_get_focus_to(ws);

    if(r==NULL){
        genws_fallback_focus(&(ws->genws), warp);
        return;
    }
    
    region_do_set_focus(r, warp);
}


static bool floatws_managed_display(WFloatWS *ws, WRegion *reg)
{
    if(!region_is_fully_mapped((WRegion*)ws))
       return FALSE;
    region_map(reg);
    return TRUE;
}


static void floatws_managed_remove(WFloatWS *ws, WRegion *reg)
{
    WRegion *next=NULL;
    bool mcf=region_may_control_focus((WRegion*)ws);
    
    region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));
    region_remove_bindmap_owned(reg, mod_floatws_floatws_bindmap,
                                (WRegion*)ws);
    
    if(ws->current_managed!=reg)
        return;
    
    ws->current_managed=NULL;
    
    if(mcf){
        if(reg->stacking.above!=NULL)
            next=reg->stacking.above;
        else
            next=floatws_get_focus_to(ws);
        
        region_do_set_focus(next!=NULL ? next : (WRegion*)ws, FALSE);
    }
}


static void floatws_managed_activated(WFloatWS *ws, WRegion *reg)
{
    ws->current_managed=reg;
}


/*}}}*/


/*{{{ Create/destroy */


static bool floatws_init(WFloatWS *ws, WWindow *parent, const WFitParams *fp)
{
    ws->managed_list=NULL;
    ws->current_managed=NULL;

    if(!genws_init(&(ws->genws), parent, fp))
        return FALSE;

    region_add_bindmap((WRegion*)ws, mod_floatws_floatws_bindmap);
    
    return TRUE;
}


WFloatWS *create_floatws(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WFloatWS, floatws, (p, parent, fp));
}


void floatws_deinit(WFloatWS *ws)
{
    while(ws->managed_list!=NULL)
        destroy_obj((Obj*)(ws->managed_list));

    genws_deinit(&(ws->genws));
}


bool floatws_rescue_clientwins(WFloatWS *ws)
{
    return region_rescue_managed_clientwins((WRegion*)ws, ws->managed_list);
}


/*EXTL_DOC
 * Destroys \var{ws} unless this would put the WM in a possibly unusable
 * state.
 */
EXTL_EXPORT_MEMBER
bool floatws_rqclose_relocate(WFloatWS *ws)
{
    if(!region_may_destroy((WRegion*)ws)){
        warn("Workspace may not be destroyed.");
        return FALSE;
    }
    
    /* TODO: move frames to other workspaces */
    
    if(!region_rescue_clientwins((WRegion*)ws)){
        warn("Failed to rescue some client windows!");
        return FALSE;
    }
    
    ioncore_defer_destroy((Obj*)ws);
    return TRUE;
}


bool floatws_rqclose(WFloatWS *ws)
{
    if(ws->managed_list!=NULL){
        warn("Workspace %s is still managing other objects "
             " -- refusing to close.", region_name((WRegion*)ws));
        return FALSE;
    }
    
    return floatws_rqclose_relocate(ws);
}


/*}}}*/


/*{{{ manage_clientwin/transient */


static void floatws_add_managed(WFloatWS *ws, WRegion *reg)
{
    region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
    region_add_bindmap_owned(reg, mod_floatws_floatws_bindmap, (WRegion*)ws);

    if(region_is_fully_mapped((WRegion*)ws))
        region_map(reg);
}


#define REG_OK(R) OBJ_IS(R, WMPlex)


static WMPlex *find_existing(WFloatWS *ws)
{
    WRegion *r=ws->current_managed;
    
    if(r!=NULL && REG_OK(r))
        return (WMPlex*)r;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r){
        if(REG_OK(r))
            return (WMPlex*)r;
    }
    
    return NULL;
}


static bool floatws_do_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                                        const WManageParams *param, 
                                        int redir, bool respectpos)
{
    WFloatFrame *frame=NULL;
    WWindow *par;
    WFitParams fp;
    int swf;
    
    if(redir==MANAGE_REDIR_PREFER_YES){
        WMPlex *m=find_existing(ws);
        if(m!=NULL){
            if(region_manage_clientwin((WRegion*)m, cwin, param,
                                       MANAGE_REDIR_STRICT_YES))
                return TRUE;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return FALSE;

    par=REGION_PARENT_CHK(ws, WWindow);
    assert(par!=NULL);
    
    /* Create frame with dummy geometry */
    fp.mode=REGION_FIT_EXACT;
    fp.g=param->geom;/*REGION_GEOM(cwin);*/
    frame=create_floatframe(par, &fp);

    if(frame==NULL){
        warn("Failed to create a new WFloatFrame for client window");
        return FALSE;
    }

    assert(region_same_rootwin((WRegion*)frame, (WRegion*)cwin));
    
    floatframe_geom_from_initial_geom(frame, ws, &fp.g, param->gravity);

    if(param->maprq && ioncore_g.opmode!=IONCORE_OPMODE_INIT){
        /* When the window is mapped by application request, position
         * request is only honoured if the position was given by the user
         * and in case of a transient (the app may know better where to 
         * place them) or if we're initialising.
         */
        respectpos=(param->tfor!=NULL || param->userpos);
    }
    
    /* However, if the requested geometry does not overlap the
     * workspaces's geometry, position request is never honoured.
     */
    if((fp.g.x+fp.g.w<=REGION_GEOM(ws).x) ||
       (fp.g.y+fp.g.h<=REGION_GEOM(ws).y) ||
       (fp.g.x>=REGION_GEOM(ws).x+REGION_GEOM(ws).w) ||
       (fp.g.y>=REGION_GEOM(ws).y+REGION_GEOM(ws).h)){
        respectpos=FALSE;
    }
    
    if(!respectpos)
        floatws_calc_placement(ws, &fp.g);
    
    /* Set proper geometry */
    region_fit((WRegion*)frame, &fp.g, REGION_FIT_EXACT);
    
    swf=(param->switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    if(!mplex_attach_simple((WMPlex*)frame, (WRegion*)cwin, swf)){
        destroy_obj((Obj*)frame);
        return FALSE;
    }

    floatws_add_managed(ws, (WRegion*)frame);
    
    /* Don't warp, it is annoying in this case */
    if(param->switchto && region_may_control_focus((WRegion*)ws)){
        ioncore_set_previous_of((WRegion*)frame);
        region_set_focus((WRegion*)frame);
    }
    
    return TRUE;
}


bool floatws_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                              const WManageParams *param,
                              int redir)
{
    return floatws_do_manage_clientwin(ws, cwin, param, redir, TRUE);
}


static bool floatws_handle_drop(WFloatWS *ws, int x, int y,
                                WRegion *dropped)
{
    WFitParams fp;
    WFloatFrame *frame;
    WWindow *par;
    
    par=REGION_PARENT_CHK(ws, WWindow);
    
    if(par==NULL)
        return FALSE;
    
    fp.mode=REGION_FIT_EXACT;
    fp.g=REGION_GEOM(dropped);
    frame=create_floatframe(par, &fp);
    
    if(frame==NULL){
        warn("Failed to create a new WFloatFrame.");
        return FALSE;
    }

    /* Resize */
    
    floatframe_geom_from_managed_geom(frame, &fp.g);
    /* The x and y arguments are in root coordinate space */
    region_rootpos((WRegion*)par, &fp.g.x, &fp.g.y);
    fp.g.x=x-fp.g.x;
    fp.g.y=y-fp.g.y;
    region_fitrep((WRegion*)frame, NULL, &fp);
    
    if(!mplex_attach_simple((WMPlex*)frame, dropped, MPLEX_ATTACH_SWITCHTO)){
        destroy_obj((Obj*)frame);
        warn("Failed to attach dropped region to created WFloatFrame");
        return FALSE;
    }
    
    floatws_add_managed(ws, (WRegion*)frame);

    return TRUE;
}


/*EXTL_DOC
 * Attach client window \var{cwin} on \var{ws}.
 * At least the following fields in \var{p} are supported:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \hline
 *  Field & Description \\
 *  \hline
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{geom} & Geometry; \var{x} and \var{y}, if set, indicates top-left of 
 *   the frame to be created while \var{width} and \var{height}, if set, indicate
 *   the size of the client window within that frame. Optional.
 * \end{tabularx}
 */
EXTL_EXPORT_MEMBER
bool floatws_attach(WFloatWS *ws, WClientWin *cwin, ExtlTab p)
{
    int posok=0;
    WManageParams param=MANAGEPARAMS_INIT;
    ExtlTab g;
    
    if(cwin==NULL)
        return FALSE;
    
    param.gravity=ForgetGravity;
    param.geom.x=0;
    param.geom.y=0;
    param.geom.w=REGION_GEOM(cwin).w;
    param.geom.h=REGION_GEOM(cwin).h;
    
    extl_table_gets_b(p, "switchto", &(param.switchto));
    
    if(extl_table_gets_t(p, "geom", &g)){
        if(extl_table_gets_i(g, "x", &(param.geom.x)))
            posok++;
        if(extl_table_gets_i(g, "y", &(param.geom.y)))
            posok++;
    
        extl_table_gets_i(p, "w", &(param.geom.w));
        extl_table_gets_i(p, "h", &(param.geom.h));
        
        extl_unref_table(g);
    }
    
    param.geom.w=maxof(0, param.geom.w);
    param.geom.h=maxof(0, param.geom.h);

    /*if(posok==2)
        param.userpos=TRUE;*/
    
    return floatws_do_manage_clientwin(ws, cwin, &param, 
                                       MANAGE_REDIR_STRICT_NO,
                                       posok==2);
}


/* A handler for clientwin_do_manage_alt hook to handle transients for windows
 * on WFloatWS:s differently from the normal behaviour.
 */
bool mod_floatws_clientwin_do_manage(WClientWin *cwin, 
                                    const WManageParams *param)
{
    WRegion *stack_above;
    WFloatWS *ws;
    
    if(param->tfor==NULL)
        return FALSE;

    stack_above=(WRegion*)REGION_PARENT_CHK(param->tfor, WFloatFrame);
    if(stack_above==NULL)
        return FALSE;
    
    ws=REGION_MANAGER_CHK(stack_above, WFloatWS);
    if(ws==NULL)
        return FALSE;
    
    if(!floatws_manage_clientwin(ws, cwin, param, MANAGE_REDIR_PREFER_NO))
        return FALSE;

    region_stack_above(REGION_MANAGER(cwin), stack_above);

    return TRUE;
}


bool floatws_manage_rescue(WFloatWS *ws, WClientWin *cwin, WRegion *from)
{
    WManageParams param=MANAGEPARAMS_INIT;
    
    region_rootpos((WRegion*)cwin, &(param.geom.x), &(param.geom.y));
    param.geom.x=0;
    param.geom.y=0;
    param.geom.w=REGION_GEOM(cwin).w;
    param.geom.h=REGION_GEOM(cwin).h;
    
    return region_manage_clientwin((WRegion*)ws, cwin, &param,
                                   MANAGE_REDIR_PREFER_NO);
}


/*}}}*/


/*{{{ Circulate */


/*EXTL_DOC
 * Activate next object on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *floatws_circulate(WFloatWS *ws)
{
    WRegion *r=REGION_NEXT_MANAGED_WRAP(ws->managed_list, ws->current_managed);
    if(r!=NULL)
        region_goto(r);
    return r;
}


/*EXTL_DOC
 * Activate and raise next object on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *floatws_circulate_and_raise(WFloatWS *ws)
{
    WRegion *r=floatws_circulate(ws);
    if(r!=NULL)
        region_raise(r);
    return r;
}


/*EXTL_DOC
 * Activate previous object on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *floatws_backcirculate(WFloatWS *ws)
{
    WRegion *r=REGION_PREV_MANAGED_WRAP(ws->managed_list, ws->current_managed);
    if(r!=NULL)
        region_goto(r);
    return r;
}


/*EXTL_DOC
 * Activate and raise previous object on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *floatws_backcirculate_and_raise(WFloatWS *ws)
{
    WRegion *r=floatws_backcirculate(ws);
    if(r!=NULL)
        region_raise(r);
    return r;
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT_MEMBER
ExtlTab floatws_managed_list(WFloatWS *ws)
{
    return managed_list_to_table(ws->managed_list, NULL);
}


/*EXTL_DOC
 * Returns the object that currently has or previously had focus on \var{ws}
 * (if no other object on the workspace currently has focus).
 */
EXTL_EXPORT_MEMBER
WRegion* floatws_current(WFloatWS *ws)
{
    return ws->current_managed;
}


/*}}}*/


/*{{{ Save/load */


static ExtlTab floatws_get_configuration(WFloatWS *ws)
{
    ExtlTab tab, mgds, st, g;
    WRegion *mgd;
    int n=0;
    
    tab=region_get_base_configuration((WRegion*)ws);
    
    mgds=extl_create_table();
    
    extl_table_sets_t(tab, "managed", mgds);
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, mgd){
        st=region_get_configuration(mgd);
        if(st==extl_table_none())
            continue;

        g=extl_table_from_rectangle(&REGION_GEOM(mgd));
        extl_table_sets_t(st, "geom", g);
        extl_unref_table(g);
        
        extl_table_seti_t(mgds, ++n, st);
        extl_unref_table(st);
    }
    
    extl_unref_table(mgds);
    
    return tab;
}


static WRegion *floatws_do_attach(WFloatWS *ws, WRegionAttachHandler *fn,
                                  void *fnparams, const WFitParams *fp)
{
    WWindow *par;
    WRegion *reg;

    par=REGION_PARENT_CHK(ws, WWindow);
    assert(par!=NULL);
    
    reg=fn(par, fp, fnparams);

    if(reg!=NULL)
        floatws_add_managed(ws, reg);
    
    return reg;
}



static WRegion *floatws_attach_load(WFloatWS *ws, ExtlTab param)
{
    WRectangle geom;
    
    if(!extl_table_gets_rectangle(param, "geom", &geom)){
        warn("No geometry specified");
        return NULL;
    }

    geom.w=maxof(geom.w, 0);
    geom.h=maxof(geom.h, 0);
    
    return region__attach_load((WRegion*)ws, param, 
                               (WRegionDoAttachFn*)floatws_do_attach,
                               &geom);
}


WRegion *floatws_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WFloatWS *ws;
    ExtlTab substab, subtab;
    int i, n;
    
    ws=create_floatws(par, fp);
    
    if(ws==NULL)
        return NULL;
        
    if(!extl_table_gets_t(tab, "managed", &substab))
        return (WRegion*)ws;

    n=extl_table_get_n(substab);
    for(i=1; i<=n; i++){
        if(extl_table_geti_t(substab, i, &subtab)){
            floatws_attach_load(ws, subtab);
            extl_unref_table(subtab);
        }
    }
    
    extl_unref_table(substab);

    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab floatws_dynfuntab[]={
    {(DynFun*)region_fitrep,
     (DynFun*)floatws_fitrep},

    {region_map, 
     floatws_map},
    {region_unmap, 
     floatws_unmap},
    {(DynFun*)region_managed_display, 
     (DynFun*)floatws_managed_display},

    {region_do_set_focus, 
     floatws_do_set_focus},
    {region_managed_activated, 
     floatws_managed_activated},
    
    {(DynFun*)region_manage_clientwin, 
     (DynFun*)floatws_manage_clientwin},
    {(DynFun*)region_handle_drop,
     (DynFun*)floatws_handle_drop},
    {region_managed_remove,
     floatws_managed_remove},
    
    {(DynFun*)region_get_configuration, 
     (DynFun*)floatws_get_configuration},

    {(DynFun*)region_rqclose,
     (DynFun*)floatws_rqclose},

    {(DynFun*)region_current,
     (DynFun*)floatws_current},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)floatws_rescue_clientwins},
    
    END_DYNFUNTAB
};


IMPLCLASS(WFloatWS, WGenWS, floatws_deinit, floatws_dynfuntab);


/*}}}*/

