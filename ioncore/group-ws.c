/*
 * ion/ioncore/group-ws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/minmax.h>
#include <libtu/objp.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "focus.h"
#include "group.h"
#include "regbind.h"
#include "bindmaps.h"
#include "xwindow.h"
#include "group-ws.h"
#include "grouprescueph.h"
#include "float-placement.h"
#include "resize.h"


/*{{{ Settings */


static bool default_ws_params_set=FALSE;
static ExtlTab default_ws_params;


/*EXTL_DOC
 * Set module basic settings. Currently only the \code{placement_method} 
 * parameter is supported.
 *
 * The method can be one  of ''udlr'', ''lrud'' (default) and ''random''. 
 * The ''udlr'' method looks for free space starting from top the top left
 * corner of the workspace moving first down keeping the x coordinate fixed.
 * If it find no free space, it start looking similarly at next x coordinate
 * unoccupied by other objects and so on. ''lrud' is the same but with the 
 * role of coordinates changed and both fall back to ''random'' placement 
 * if no free area was found.
 */

void ioncore_groupws_set(ExtlTab tab)
{
    char *method=NULL;
    ExtlTab t;
    
    if(extl_table_gets_s(tab, "float_placement_method", &method)){
        if(strcmp(method, "udlr")==0)
            ioncore_placement_method=PLACEMENT_UDLR;
        else if(strcmp(method, "lrud")==0)
            ioncore_placement_method=PLACEMENT_LRUD;
        else if(strcmp(method, "random")==0)
            ioncore_placement_method=PLACEMENT_RANDOM;
        else
            warn(TR("Unknown placement method \"%s\"."), method);
        free(method);
    }
    
    if(extl_table_gets_t(tab, "default_ws_params", &t)){
        if(default_ws_params_set)
            extl_unref_table(default_ws_params);
        default_ws_params=t;
        default_ws_params_set=TRUE;
    }
}


void ioncore_groupws_get(ExtlTab t)
{
    extl_table_sets_s(t, "float_placement_method", 
                      (ioncore_placement_method==PLACEMENT_UDLR
                       ? "udlr" 
                       : (ioncore_placement_method==PLACEMENT_LRUD
                          ? "lrud" 
                          : "random")));
    
    if(default_ws_params_set)
        extl_table_sets_t(t, "default_ws_params", default_ws_params);
}


/*}}}*/


/*{{{ Attach stuff */


static bool groupws_attach_framed(WGroupWS *ws, WGroupAttachParams *ap,
                                  WRegion *reg)
{
    WRegionAttachData data;
    
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return (group_do_attach_framed(&ws->grp, ap, &data)!=NULL);
}


bool groupws_handle_drop(WGroupWS *ws, int x, int y,
                         WRegion *dropped)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    
    ap.switchto_set=TRUE;
    ap.switchto=TRUE;
    ap.geom_set=TRUE;
    ap.geom.x=x;
    ap.geom.y=y;
    ap.geom.w=REGION_GEOM(dropped).w;
    ap.geom.h=REGION_GEOM(dropped).h;
    ap.geom_weak=0;
    ap.framed_inner_geom=TRUE;
    ap.framed_gravity=NorthWestGravity;
    
    return groupws_attach_framed(ws, &ap, dropped);
}


/*EXTL_DOC
 * Attach region \var{reg} on \var{ws}.
 * At least the following fields in \var{t} are supported:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{geom} & Geometry; \var{x} and \var{y}, if set, indicates top-left of 
 *   the frame to be created while \var{width} and \var{height}, if set, indicate
 *   the size of the client window within that frame. Optional.
 * \end{tabularx}
 */
EXTL_EXPORT_AS(WGroupWS, attach_framed)
bool groupws_attach_framed_extl(WGroupWS *ws, WRegion *reg, ExtlTab t)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    ExtlTab gt;
    
    if(reg==NULL)
        return FALSE;
    
    ap.framed_inner_geom=TRUE;
    ap.framed_gravity=ForgetGravity;
    
    if(extl_table_is_bool_set(t, "switchto")){
        ap.switchto_set=TRUE;
        ap.switchto=TRUE;
    }
    
    if(extl_table_gets_t(t, "geom", &gt)){
        int pos=0, size=0;
        
        ap.geom.x=0;
        ap.geom.y=0;

        if(extl_table_gets_i(gt, "x", &(ap.geom.x)))
            pos++;
        if(extl_table_gets_i(gt, "y", &(ap.geom.y)))
            pos++;
    
        if(extl_table_gets_i(gt, "w", &(ap.geom.w)))
            size++;
        if(extl_table_gets_i(gt, "h", &(ap.geom.h)))
            size++;
        
        ap.geom.w=maxof(ap.geom.w, 1);
        ap.geom.h=maxof(ap.geom.h, 1);
        
        ap.geom_set=(size==2);
        ap.geom_weak=(pos!=2
                      ? REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y
                      : 0);
        
        extl_unref_table(gt);
    }
    
    return groupws_attach_framed(ws, &ap, reg);
}


/*}}}*/


/*{{{ groupws_prepare_manage */


#define REG_OK(R) OBJ_IS(R, WMPlex)


static WMPlex *find_existing(WGroupWS *ws)
{
    WGroupIterTmp tmp;
    WRegion *r=(ws->grp.current_managed!=NULL 
                ? ws->grp.current_managed->reg 
                : NULL);
    
    if(r!=NULL && REG_OK(r))
        return (WMPlex*)r;
    
    FOR_ALL_MANAGED_BY_GROUP(&ws->grp, r, tmp){
        if(REG_OK(r))
            return (WMPlex*)r;
    }
    
    return NULL;
}


static WGroupRescuePH *groupws_prepare_manage_in_frame(WGroupWS *ws, 
                                                       const WClientWin *cwin,
                                                       const WManageParams *param, 
                                                       int geom_weak,
                                                       WRegion *stack_above)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;

    ap.geom_set=TRUE;
    ap.geom=param->geom;
    
    ap.framed_inner_geom=TRUE;
    ap.framed_gravity=param->gravity;
    ap.geom_weak=geom_weak;
    ap.stack_above=stack_above;

    return create_grouprescueph(&ws->grp, NULL, &ap);
}


static WPHolder *groupws_do_prepare_manage(WGroupWS *ws, 
                                           const WClientWin *cwin,
                                           const WManageParams *param, 
                                           int redir, int geom_weak)
{
    WPHolder *ph;
    
    if(redir==MANAGE_REDIR_PREFER_YES){
        WMPlex *m=find_existing(ws);
        if(m!=NULL){
            ph=region_prepare_manage((WRegion*)m, cwin, param,
                                     MANAGE_REDIR_STRICT_YES);
            if(ph!=NULL)
                return ph;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return NULL;

    return (WPHolder*)groupws_prepare_manage_in_frame(ws, cwin, param,
                                                      geom_weak, NULL);
}


WPHolder *groupws_prepare_manage(WGroupWS *ws, const WClientWin *cwin,
                                 const WManageParams *param,
                                 int redir)
{
    WRegion *b=(ws->grp.bottom!=NULL ? ws->grp.bottom->reg : NULL);
    int weak=0;
    
    if(param->maprq && ioncore_g.opmode!=IONCORE_OPMODE_INIT
       && !param->userpos){
        /* When the window is mapped by application request, position
         * request is only honoured if the position was given by the user
         * and in case of a transient (the app may know better where to 
         * place them) or if we're initialising.
         */
        weak=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    }

    if(b!=NULL && HAS_DYN(b, region_prepare_manage)){
        WPHolder *ph=region_prepare_manage(b, cwin, param, redir);
        if(ph!=NULL)
            return ph;
    }
    
    return groupws_do_prepare_manage(ws, cwin, param, redir, weak);
}


WPHolder *groupws_prepare_manage_transient(WGroupWS *ws, const WClientWin *cwin,
                                           const WManageParams *param,
                                           int unused)
{
    WRegion *stack_above;
    
    stack_above=OBJ_CAST(REGION_PARENT(param->tfor), WRegion);
    if(stack_above==NULL)
        return NULL;
    ws=REGION_MANAGER_CHK(stack_above, WGroupWS);
    if(ws==NULL)
        return NULL;
    
    return (WPHolder*)groupws_prepare_manage_in_frame(ws, cwin, param, 
                                                      0, stack_above);
}


/*}}}*/


/*{{{ WGroupWS class */


bool groupws_init(WGroupWS *ws, WWindow *parent, const WFitParams *fp)
{
    if(!group_init(&(ws->grp), parent, fp))
        return FALSE;

    ((WRegion*)ws)->flags|=REGION_GRAB_ON_PARENT;
    
    region_add_bindmap((WRegion*)ws, ioncore_groupws_bindmap);
    
    return TRUE;
}


WGroupWS *create_groupws(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WGroupWS, groupws, (p, parent, fp));
}


void groupws_deinit(WGroupWS *ws)
{    
    group_deinit(&(ws->grp));
}


WRegion *groupws_load(WWindow *par, const WFitParams *fp, 
                       ExtlTab tab)
{
    WGroupWS *ws;
    
    ws=create_groupws(par, fp);
    
    if(ws==NULL)
        return NULL;

    group_do_load(&ws->grp, tab);
    
    return (WRegion*)ws;
}


WRegion *groupws_load_default(WWindow *par, const WFitParams *fp)
{
    return groupws_load(par, fp, (default_ws_params_set
                                  ? default_ws_params
                                  : extl_table_none()));
}


static DynFunTab groupws_dynfuntab[]={
    {(DynFun*)region_prepare_manage, 
     (DynFun*)groupws_prepare_manage},
    
    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)groupws_prepare_manage_transient},
    
    {(DynFun*)region_handle_drop,
     (DynFun*)groupws_handle_drop},
    
    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)groupws_get_rescue_pholder_for},
    
    {region_manage_stdisp,
     group_manage_stdisp},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroupWS, WGroup, groupws_deinit, groupws_dynfuntab);


/*}}}*/

