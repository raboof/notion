/*
 * ion/ioncore/group-cw.c
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
#include <libmainloop/defer.h>

#include "common.h"
#include "group-cw.h"
#include "clientwin.h"
#include "regbind.h"
#include "bindmaps.h"
#include "frame.h"
#include "resize.h"
#include "pholder.h"
#include "names.h"
#include "framedpholder.h"
#include "grouppholder.h"


#define DFLT_SZPLCY SIZEPOLICY_FREE_GLUE__SOUTH


/*{{{ Add/remove managed */


static WPHolder *groupcw_transient_pholder(WGroupCW *cwg, 
                                           const WClientWin *cwin,
                                           const WManageParams *mp)
{
    WGroupAttachParams param=GROUPATTACHPARAMS_INIT;
    WFramedParam fp=FRAMEDPARAM_INIT;
    WPHolder *ph;
    
    param.level_set=1;
    param.level=STACKING_LEVEL_MODAL1;
    
    param.szplcy_set=1;
    param.szplcy=cwg->transient_szplcy;
    
    param.switchto_set=1;
    param.switchto=1;

    param.geom_weak_set=1;
    param.geom_weak=REGION_RQGEOM_WEAK_ALL;
    
    if(!ioncore_g.framed_transients){
        param.geom_set=TRUE;
        param.geom=mp->geom;
        
        return (WPHolder*)create_grouppholder(&cwg->grp, NULL, &param);
    }else{
        fp.inner_geom_gravity_set=1;
        fp.inner_geom=mp->geom;
        fp.gravity=ForgetGravity;
        fp.mode=FRAME_MODE_TRANSIENT;
        
        ph=(WPHolder*)create_grouppholder(&cwg->grp, NULL, &param);
        
        return pholder_either((WPHolder*)create_framedpholder(ph, &fp), ph);
    }
}


WPHolder *groupcw_prepare_manage(WGroupCW *cwg, const WClientWin *cwin,
                                 const WManageParams *param, int redir)
{
    if(redir==MANAGE_REDIR_STRICT_YES)
        return NULL;
    
    /* Only catch windows with transient mode set to current here. */
    if(clientwin_get_transient_mode(cwin)!=TRANSIENT_MODE_CURRENT)
        return NULL;
    
    return groupcw_transient_pholder(cwg, cwin, param);
}


static bool groupcw_should_manage_transient(WGroupCW *cwg, 
                                            WClientWin *tfor)
{
    WRegion *mgr;
    
    if(group_find_stacking(&cwg->grp, (WRegion*)tfor))
        return TRUE;
 
    mgr=REGION_MANAGER(tfor);
    
    if(mgr!=NULL && ioncore_g.framed_transients && OBJ_IS(mgr, WFrame))
        return (group_find_stacking(&cwg->grp, mgr)!=NULL);
    
    return FALSE;
}


WPHolder *groupcw_prepare_manage_transient(WGroupCW *cwg, 
                                           const WClientWin *transient,
                                           const WManageParams *param,
                                           int unused)
{
    WPHolder *ph=region_prepare_manage_transient_default((WRegion*)cwg,
                                                         transient,
                                                         param,
                                                         unused);
    
    if(ph==NULL && groupcw_should_manage_transient(cwg, param->tfor))
        ph=groupcw_transient_pholder(cwg, transient, param);

    return ph;
}


/*}}}*/


/*{{{ Misc. */


/*_EXTL_DOC
 * Toggle transients managed by \var{cwin} between top/bottom
 * of the window.
 */
EXTL_EXPORT_MEMBER
void groupcw_toggle_transients_pos(WGroupCW *cwg)
{
    WStacking *st;
    WGroupIterTmp tmp;
    
    if((cwg->transient_szplcy&SIZEPOLICY_VERT_MASK)==SIZEPOLICY_VERT_TOP){
        cwg->transient_szplcy&=~SIZEPOLICY_VERT_MASK;
        cwg->transient_szplcy|=SIZEPOLICY_VERT_BOTTOM;
    }else{
        cwg->transient_szplcy&=~SIZEPOLICY_VERT_MASK;
        cwg->transient_szplcy|=SIZEPOLICY_VERT_TOP;
    }

    FOR_ALL_NODES_IN_GROUP(&cwg->grp, st, tmp){
        st->szplcy&=~SIZEPOLICY_VERT_MASK;
        st->szplcy|=(cwg->transient_szplcy&SIZEPOLICY_VERT_MASK);
        
        if(st->reg!=NULL){
            WFitParams fp;

            fp.g=REGION_GEOM(cwg);
            
            sizepolicy(&st->szplcy, st->reg, NULL, 
                       REGION_RQGEOM_WEAK_ALL, &fp);
            region_fitrep(st->reg, NULL, &fp);
        }
    }
}


const char *groupcw_displayname(WGroupCW *cwg)
{
    const char *name=NULL;
    
    if(cwg->grp.bottom!=NULL && cwg->grp.bottom->reg!=NULL)
        name=region_name(cwg->grp.bottom->reg);
    
    if(name==NULL)
        name=region_name((WRegion*)cwg);
    
    return name;
}


void groupcw_managed_notify(WGroupCW *cwg, WRegion *reg, WRegionNotify how)
{
    if(group_bottom(&cwg->grp)==reg && how==ioncore_g.notifies.name){
        /* Title has changed */
        region_notify_change((WRegion*)cwg, how);
    }
}


/*}}}*/


/*{{{ WGroupCW class */


bool groupcw_init(WGroupCW *cwg, WWindow *parent, const WFitParams *fp)
{
    cwg->transient_szplcy=DFLT_SZPLCY;
    
    if(!group_init(&(cwg->grp), parent, fp))
        return FALSE;
    
    cwg->grp.bottom_last_close=TRUE;

    region_add_bindmap((WRegion*)cwg, ioncore_groupcw_bindmap);
    
    return TRUE;
}


WGroupCW *create_groupcw(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WGroupCW, groupcw, (p, parent, fp));
}


void groupcw_deinit(WGroupCW *cwg)
{    
    group_deinit(&(cwg->grp));
}


WRegion *groupcw_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WGroupCW *ws;
    ExtlTab substab, subtab;
    int i, n;
    
    ws=create_groupcw(par, fp);
    
    if(ws==NULL)
        return NULL;
        
    if(!extl_table_gets_t(tab, "managed", &substab))
        return (WRegion*)ws;

    n=extl_table_get_n(substab);
    for(i=1; i<=n; i++){
        if(extl_table_geti_t(substab, i, &subtab)){
            group_attach_new(&ws->grp, subtab);
            extl_unref_table(subtab);
        }
    }
    
    extl_unref_table(substab);
    
    if(ws->grp.managed_list==NULL){
        destroy_obj((Obj*)ws);
        return NULL;
    }

    return (WRegion*)ws;
}


static DynFunTab groupcw_dynfuntab[]={
    {(DynFun*)region_prepare_manage, 
     (DynFun*)groupcw_prepare_manage},
    
    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)groupcw_prepare_manage_transient},
    
    /*
    {(DynFun*)region_handle_drop,
     (DynFun*)groupcw_handle_drop},
    
    {(DynFun*)group_do_add_managed,
     (DynFun*)groupcw_do_add_managed},
    */
    
    /*
    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)groupcw_get_rescue_pholder_for},
     */
    
    {(DynFun*)region_prepare_manage,
     (DynFun*)groupcw_prepare_manage},

    {(DynFun*)region_prepare_manage_transient,
     (DynFun*)groupcw_prepare_manage_transient},
    
    {(DynFun*)region_displayname,
     (DynFun*)groupcw_displayname},
    
    {region_managed_notify,
     groupcw_managed_notify},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroupCW, WGroup, groupcw_deinit, groupcw_dynfuntab);


/*}}}*/

