/*
 * ion/ioncore/manage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include "global.h"
#include "common.h"
#include "region.h"
#include "manage.h"
#include "names.h"
#include "fullscreen.h"
#include "pointer.h"
#include <libextl/extl.h>
#include "netwm.h"
#include "extlconv.h"
#include "region-iter.h"


/*{{{ Add */


WScreen *clientwin_find_suitable_screen(WClientWin *cwin, 
                                        const WManageParams *param)
{
    WScreen *scr=NULL, *found=NULL;
    bool respectpos=(param->tfor!=NULL || param->userpos);
    
    FOR_ALL_SCREENS(scr){
        if(!region_same_rootwin((WRegion*)scr, (WRegion*)cwin))
            continue;
        if(REGION_IS_ACTIVE(scr)){
            found=scr;
            if(!respectpos)
                break;
        }
        
        if(rectangle_contains(&REGION_GEOM(scr), param->geom.x, param->geom.y)){
            found=scr;
            if(respectpos)
                break;
        }
        
        if(found==NULL)
            found=scr;
    }
    
    return found;
}


bool clientwin_do_manage_default(WClientWin *cwin, 
                                 const WManageParams *param)
{
    WRegion *r=NULL, *r2;
    WScreen *scr=NULL;
    int fs;
    
    /* Transients are managed by their transient_for client window unless the
     * behaviour is overridden before this function.
     */
    if(param->tfor!=NULL){
        if(clientwin_attach_transient(param->tfor, (WRegion*)cwin))
            return TRUE;
    }
    
    /* Check fullscreen mode */
    
    fs=netwm_check_initial_fullscreen(cwin, param->switchto);
    
    if(fs<0){
        fs=clientwin_check_fullscreen_request(cwin, 
                                              param->geom.w,
                                              param->geom.h,
                                              param->switchto);
    }

    if(fs>0)
        return TRUE;

    /* Find a suitable screen */
    scr=clientwin_find_suitable_screen(cwin, param);
    if(scr==NULL){
        warn(TR("Unable to find a screen for a new client window."));
        return FALSE;
    }

    return region_manage_clientwin((WRegion*)scr, cwin, param,
                                   MANAGE_REDIR_PREFER_YES);
}


/*}}}*/


/*{{{ region_manage_clientwin */


bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
                             const WManageParams *param, int redir)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_manage_clientwin, reg, 
                 (reg, cwin, param, redir));
    return ret;
}


bool region_manage_clientwin_default(WRegion *reg, WClientWin *cwin,
                                     const WManageParams *param, int redir)
{
    WRegion *curr;
    
    if(redir==MANAGE_REDIR_STRICT_NO)
        return FALSE;
    
    curr=region_current(reg);
    
    if(curr==NULL)
        return FALSE;
        
    return region_manage_clientwin(curr, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES);
}


/*}}}*/


/*{{{ Rescue */


bool region_manage_rescue(WRegion *reg, WClientWin *cwin, WRegion *from)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_manage_rescue, reg, (reg, cwin, from));
    return ret;
}


bool region_manage_rescue_default(WRegion *reg, WClientWin *cwin,
                                  WRegion *from)
{
    WRegion *curr=region_current(reg);
    WManageParams param=MANAGEPARAMS_INIT;
    
    if(curr==from)
        return FALSE;
    
    if(OBJ_IS_BEING_DESTROYED(curr))
        return FALSE;

    region_rootpos((WRegion*)cwin, &(param.geom.x), &(param.geom.y));
    param.geom.w=REGION_GEOM(cwin).w;
    param.geom.h=REGION_GEOM(cwin).h;
    
    return region_manage_clientwin(curr, cwin, &param, 
                                   MANAGE_REDIR_STRICT_NO);
}


static bool do_rescue(WRegion *reg, WRegion *r)
{
    WRegion *mgr;
    
    if(!OBJ_IS(r, WClientWin))
        return region_rescue_clientwins(r);

    while(1){
        mgr=region_manager_or_parent(reg);
        if(mgr==NULL)
            break;
        if(!OBJ_IS_BEING_DESTROYED(mgr) &&
           !(mgr->flags&REGION_CWINS_BEING_RESCUED)){
            if(region_manage_rescue(mgr, (WClientWin*)r, reg))
                return TRUE;
        }
        reg=mgr;
    }
    
    warn(TR("Unable to rescue \"%s\"."), region_name(r));
    
    return FALSE;
}


#warning "TODO: FIX"
/*bool region_rescue_managed_clientwins(WRegion *reg, WRegion *list)
{
    WRegion *r, *next;
    bool res=TRUE;
    
    reg->flags|=REGION_CWINS_BEING_RESCUED;
    
    FOR_ALL_MANAGED_ON_LIST_W_NEXT(list, r, next){
        if(!do_rescue(reg, r))
            res=FALSE;
    }
    
    reg->flags&=~REGION_CWINS_BEING_RESCUED;

    return res;
}*/


bool region_rescue_child_clientwins(WRegion *reg)
{
    WRegion *r, *next;
    bool res=TRUE;
    
    reg->flags|=REGION_CWINS_BEING_RESCUED;

    for(r=reg->children; r!=NULL; r=next){
        next=r->p_next;
        if(!do_rescue(reg, r))
            res=FALSE;
    }
    
    reg->flags&=~REGION_CWINS_BEING_RESCUED;

    return res;
}


bool region_rescue_clientwins(WRegion *reg)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_rescue_clientwins, reg, (reg));
    return ret;
}


/*}}}*/


/*{{{ Misc. */


ExtlTab manageparams_to_table(WManageParams *mp)
{
    ExtlTab t=extl_create_table();
    extl_table_sets_b(t, "switchto", mp->switchto);
    extl_table_sets_b(t, "jumpto", mp->jumpto);
    extl_table_sets_b(t, "userpos", mp->userpos);
    extl_table_sets_b(t, "dockapp", mp->dockapp);
    extl_table_sets_b(t, "maprq", mp->maprq);
    extl_table_sets_i(t, "gravity", mp->gravity);
    extl_table_sets_rectangle(t, "geom", &(mp->geom));
    extl_table_sets_o(t, "tfor", (Obj*)(mp->tfor));
    
    return t;
}


/*}}}*/
