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


static WRegion *iter_child_cwins(void *st)
{
    WRegion **nextp=(WRegion**)st;
    WRegion *next;
    
    do{
        next=*nextp;
        *nextp=(next==NULL ? NULL : next->p_next);
    }while(next!=NULL && !OBJ_IS(next, WClientWin));
        
    return next;   
}


bool region_rescue_child_clientwins(WRegion *reg)
{
    WRegion *next=reg->children;
    bool res=region_rescue_some_clientwins(reg, iter_child_cwins, &next);
    assert(!res || reg->children==NULL);
    return res;
}


bool region_rescue_some_clientwins(WRegion *reg, 
                                   WRegionIterator *iter, void *st)
{
    WPHolder *ph;
    bool res=FALSE;
    int fails=0;

    reg->flags|=REGION_CWINS_BEING_RESCUED;
    
    while(TRUE){
        WRegion *tosave=iter(st);
        if(tosave==NULL)
            break;
        
        ph=region_get_rescue_pholder(reg);
        
        if(ph==NULL)
            return FALSE;

        if(!pholder_attach(ph, tosave))
            fails++;
    }
    
    reg->flags&=~REGION_CWINS_BEING_RESCUED;

    return (fails==0);
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
