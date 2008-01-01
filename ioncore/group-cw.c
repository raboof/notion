/*
 * ion/ioncore/group-cw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
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
#include "return.h"
#include "saveload.h"


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
                                 const WManageParams *param, int priority)
{
    if(!MANAGE_PRIORITY_OK(priority, MANAGE_PRIORITY_GROUP))
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


static WRegion *groupcw_managed_disposeroot(WGroupCW *ws, WRegion *reg)
{
    WGroupIterTmp tmp;
    WStacking *st;
    WRegion *tmpr;
    
    FOR_ALL_NODES_IN_GROUP(&ws->grp, st, tmp){
        if(st!=ws->grp.managed_stdisp && st->reg!=reg){
            return reg;
        }
    }
    
    tmpr=region_disposeroot((WRegion*)ws);
    return (tmpr!=NULL ? tmpr : reg);
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
            
            fp.mode=0;
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
    
    group_managed_notify(&cwg->grp, reg, how);
}


void groupcw_bottom_set(WGroupCW *cwg)
{
    region_notify_change((WRegion*)cwg, ioncore_g.notifies.name);
}


/*}}}*/


/*{{{ Rescue */


static void group_migrate_phs_to_ph(WGroup *group, WPHolder *rph)
{
    WGroupPHolder *phs, *ph;
    
    phs=group->phs;
    group->phs=NULL;
    
    phs->recreate_pholder=rph;
    
    for(ph=phs; ph!=NULL; ph=ph->next)
        ph->group=NULL;
}


bool groupcw_rescue_clientwins(WGroupCW *cwg, WRescueInfo *info)
{
    bool ret=group_rescue_clientwins(&cwg->grp, info);
    
    /* If this group can be recreated, arrange remaining placeholders
     * to do so. This takes care of e.g. recreating client window groups
     * when recreating layout delayedly under a session manager.
     */
    if(cwg->grp.phs!=NULL){
        WPHolder *rph=region_make_return_pholder((WRegion*)cwg);
        
        if(rph!=NULL)
            group_migrate_phs_to_ph(&cwg->grp, rph);
    }
    
    return ret;
}


/*}}}*/



/*{{{ WGroupCW class */


bool groupcw_init(WGroupCW *cwg, WWindow *parent, const WFitParams *fp)
{
    cwg->transient_szplcy=DFLT_SZPLCY;
    
    if(!group_init(&(cwg->grp), parent, fp))
        return FALSE;
    
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
    WGroupCW *cwg;
    ExtlTab substab, subtab;
    int i, n;
    
    cwg=create_groupcw(par, fp);
    
    if(cwg==NULL)
        return NULL;

    group_do_load(&cwg->grp, tab);
    
    if(cwg->grp.managed_list==NULL){
        if(cwg->grp.phs!=NULL){
            /* Session management hack */
            WPHolder *ph=ioncore_get_load_pholder();
            if(ph!=NULL)
                group_migrate_phs_to_ph(&cwg->grp, ph);
        }
        destroy_obj((Obj*)cwg);
        return NULL;
    }

    return (WRegion*)cwg;
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
     
    {(DynFun*)region_managed_disposeroot,
     (DynFun*)groupcw_managed_disposeroot},
    
    {(DynFun*)region_displayname,
     (DynFun*)groupcw_displayname},
    
    {region_managed_notify,
     groupcw_managed_notify},
     
    {group_bottom_set,
     groupcw_bottom_set},
     
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)groupcw_rescue_clientwins},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WGroupCW, WGroup, groupcw_deinit, groupcw_dynfuntab);


/*}}}*/

