/*
 * ion/ioncore/group-ws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
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
#include "group-cw.h"
#include "grouppholder.h"
#include "framedpholder.h"
#include "float-placement.h"
#include "resize.h"
#include "conf.h"
#include "saveload.h"
#include "screen-notify.h"


/*{{{ Settings */


void ioncore_groupws_set(ExtlTab tab)
{
    char *method=NULL;
    int fpp;
    
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
    if(extl_table_gets_i(tab, "float_placement_padding", &fpp))
        ioncore_placement_padding=maxof(0, fpp);
}


void ioncore_groupws_get(ExtlTab t)
{
    extl_table_sets_s(t, "float_placement_method", 
                      (ioncore_placement_method==PLACEMENT_UDLR
                       ? "udlr" 
                       : (ioncore_placement_method==PLACEMENT_LRUD
                          ? "lrud" 
                          : "random")));
    extl_table_sets_i(t, "float_placement_padding", ioncore_placement_padding);
}


/*}}}*/


/*{{{ Attach stuff */


static bool groupws_attach_framed(WGroupWS *ws, 
                                  WGroupAttachParams *ap,
                                  WFramedParam *fp,
                                  WRegion *reg)
{
    WRegionAttachData data;
    
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return (region_attach_framed((WRegion*)ws, fp,
                                 (WRegionAttachFn*)group_do_attach,
                                 ap, &data)!=NULL);
}


bool groupws_handle_drop(WGroupWS *ws, int x, int y,
                         WRegion *dropped)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WFramedParam fp=FRAMEDPARAM_INIT;
    
    ap.switchto_set=TRUE;
    ap.switchto=TRUE;
    
    fp.inner_geom_gravity_set=TRUE;
    fp.inner_geom.x=x;
    fp.inner_geom.y=y;
    fp.inner_geom.w=REGION_GEOM(dropped).w;
    fp.inner_geom.h=REGION_GEOM(dropped).h;
    fp.gravity=NorthWestGravity;
    
    return groupws_attach_framed(ws, &ap, &fp, dropped);
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
    WFramedParam frp=FRAMEDPARAM_INIT;
    
    if(reg==NULL)
        return FALSE;
    
    groupattachparams_get(&ap, t, NULL);
    
    /* Sensible size is given in framedparams */
    if(ap.geom_set){
        ap.geom_set=0;
        ap.geom_weak_set=1;
        ap.geom_weak=0;
        
        frp.inner_geom_gravity_set=1;
        frp.inner_geom=ap.geom;
        frp.gravity=NorthWestGravity;
        extl_table_gets_i(t, "gravity", &frp.gravity);
    }
    
    return groupws_attach_framed(ws, &ap, &frp, reg);
}


/*}}}*/


/*{{{ groupws_prepare_manage */


static WPHolder *groupws_do_prepare_manage(WGroupWS *ws, 
                                           const WClientWin *cwin,
                                           const WManageParams *param)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WFramedParam fp=FRAMEDPARAM_INIT;
    WPHolder *ph;
    

    fp.inner_geom_gravity_set=TRUE;
    fp.inner_geom=param->geom;
    fp.gravity=param->gravity;

    groupattachparams_get(&ap, cwin->proptab, "attach_params");
    
    if(param->userpos ||
       !param->maprq || ioncore_g.opmode==IONCORE_OPMODE_INIT){
        ap.geom_weak_set=1;
        ap.geom_weak=0;
    }else if(!ap.geom_weak_set){
        ap.geom_weak_set=1;
        ap.geom_weak=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    }

    ph=(WPHolder*)create_grouppholder(&ws->grp, NULL, &ap);
    
    if(ph!=NULL)
        ph=pholder_either((WPHolder*)create_framedpholder(ph, &fp), ph);
    
    if(ph!=NULL){
        WGroupPHolder *gph;
        WGroupAttachParams gp=GROUPATTACHPARAMS_INIT;
        
        gp.switchto_set=1;
        gp.switchto=1;
        gp.bottom=1;
        
        gph=create_grouppholder(NULL, NULL, &gp);
        
        if(gph!=NULL){
            gph->recreate_pholder=ph;
            return (WPHolder*)gph;
        }
    }
    
    return ph;
}


WPHolder *groupws_prepare_manage(WGroupWS *ws, const WClientWin *cwin,
                                 const WManageParams *param,
                                 int priority)
{
    int cpriority=MANAGE_PRIORITY_SUB(priority, MANAGE_PRIORITY_GROUP);
    int bpriority=MANAGE_PRIORITY_SUBX(priority, MANAGE_PRIORITY_GROUP);
    WRegion *b=(ws->grp.bottom!=NULL ? ws->grp.bottom->reg : NULL);
    WPHolder *ph=NULL;
    bool act_b=(ws->grp.bottom==ws->grp.current_managed);
    bool use_bottom;

    if(b!=NULL && !HAS_DYN(b, region_prepare_manage))
        b=NULL;
    
    use_bottom=(act_b
                ? !extl_table_is_bool_set(cwin->proptab, "float")
                : act_b);
    
    if(b!=NULL && use_bottom)
        ph=region_prepare_manage(b, cwin, param, bpriority);
    
    if(ph==NULL){
        /* Check current */
        WRegion *r=(ws->grp.current_managed!=NULL 
                    ? ws->grp.current_managed->reg 
                    : NULL);
        
        if(r!=NULL && r!=b)
            ph=region_prepare_manage(r, cwin, param, cpriority);
    }
    
    if(ph==NULL && MANAGE_PRIORITY_OK(priority, MANAGE_PRIORITY_GROUP))
        ph=groupws_do_prepare_manage(ws, cwin, param);
    
    if(ph==NULL && b!=NULL && !use_bottom)
        ph=region_prepare_manage(b, cwin, param, cpriority);
    
    return ph;
}


WPHolder *groupws_prepare_manage_transient(WGroupWS *ws, const WClientWin *cwin,
                                           const WManageParams *param,
                                           int unused)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WFramedParam fp=FRAMEDPARAM_INIT;
    WPHolder *ph;
    bool tmp;
    
    ap.stack_above=OBJ_CAST(REGION_PARENT(param->tfor), WRegion);
    if(ap.stack_above==NULL)
        return NULL;
    
    fp.inner_geom_gravity_set=TRUE;
    fp.inner_geom=param->geom;
    fp.gravity=param->gravity;
    fp.mode=FRAME_MODE_TRANSIENT;

    groupattachparams_get(&ap, cwin->proptab, "attach_params");
    
    if(!ap.geom_weak_set || param->userpos ||
       !param->maprq || ioncore_g.opmode==IONCORE_OPMODE_INIT){
        ap.geom_weak_set=1;
        ap.geom_weak=0;
    }
    
    ph=(WPHolder*)create_grouppholder(&ws->grp, NULL, &ap);
    
    return pholder_either((WPHolder*)create_framedpholder(ph, &fp), ph);
}


static bool group_empty_for_bottom_stdisp(WGroup *ws)
{
    WGroupIterTmp tmp;
    WStacking *st;
    
    FOR_ALL_NODES_IN_GROUP(ws, st, tmp){
        if(st!=ws->bottom && st!=ws->managed_stdisp)
            return FALSE;
    }
    
    return TRUE;
}


static WRegion *groupws_managed_disposeroot(WGroupWS *ws, WRegion *reg)
{
    if(group_bottom(&ws->grp)==reg){
        if(group_empty_for_bottom_stdisp(&ws->grp))
            return region_disposeroot((WRegion*)ws);
    }
    
    return reg;
}


/*}}}*/


/*{{{ WGroupWS class */


bool groupws_init(WGroupWS *ws, WWindow *parent, const WFitParams *fp)
{
    if(!group_init(&(ws->grp), parent, fp, "Notion GroupWS"))
        return FALSE;

    ws->initial_outputs=extl_create_table();

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
    extl_unref_table(ws->initial_outputs);

    screen_unnotify_if_workspace(ws);

    group_deinit(&(ws->grp));
}


WRegion *groupws_load(WWindow *par, const WFitParams *fp, 
                      ExtlTab tab)
{
    WGroupWS *ws;
    ExtlTab io;
    
    ws=create_groupws(par, fp);
    
    if(ws==NULL)
        return NULL;

    group_do_load(&ws->grp, tab);
    
    if(extl_table_gets_t(tab, "initial_outputs", &io))
        ws->initial_outputs=io;

    return (WRegion*)ws;
}

ExtlTab groupws_get_configuration(WGroupWS *ws)
{

    ExtlTab tab;

    tab=group_get_configuration(&ws->grp);
    extl_table_sets_t(tab, "initial_outputs", ws->initial_outputs);
    return tab;
}

EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab groupws_get_initial_outputs(WGroupWS *ws)
{
    return extl_ref_table(ws->initial_outputs);
}

EXTL_SAFE
EXTL_EXPORT_MEMBER
void groupws_set_initial_outputs(WGroupWS *ws, ExtlTab tab)
{
    extl_unref_table(ws->initial_outputs);
    ws->initial_outputs=extl_ref_table(tab);
}


static DynFunTab groupws_dynfuntab[]={
    {(DynFun*)region_prepare_manage, 
     (DynFun*)groupws_prepare_manage},
    
    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)groupws_prepare_manage_transient},
     
    {(DynFun*)region_managed_disposeroot,
     (DynFun*)groupws_managed_disposeroot},
    
    {(DynFun*)region_handle_drop,
     (DynFun*)groupws_handle_drop},
    
    {region_manage_stdisp,
     group_manage_stdisp},

    {(DynFun*)region_get_configuration,
     (DynFun*)groupws_get_configuration},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroupWS, WGroup, groupws_deinit, groupws_dynfuntab);


/*}}}*/

