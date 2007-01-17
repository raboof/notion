/*
 * ion/ioncore/manage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libextl/extl.h>
#include "global.h"
#include "common.h"
#include "region.h"
#include "manage.h"
#include "names.h"
#include "fullscreen.h"
#include "pointer.h"
#include "netwm.h"
#include "extlconv.h"
#include "return.h"


/*{{{ Add */


WScreen *clientwin_find_suitable_screen(WClientWin *cwin, 
                                        const WManageParams *param)
{
    WScreen *scr=NULL, *found=NULL;
    bool respectpos=(param->tfor!=NULL || param->userpos);
    
    FOR_ALL_SCREENS(scr){
        if(!region_same_rootwin((WRegion*)scr, (WRegion*)cwin))
            continue;
            
        if(!OBJ_IS(scr, WRootWin)){
            /* The root window itself is only a fallback */
            
            if(REGION_IS_ACTIVE(scr)){
                found=scr;
                if(!respectpos)
                    break;
            }
            
            if(rectangle_contains(&REGION_GEOM(scr), 
                                  param->geom.x, param->geom.y)){
                found=scr;
                if(respectpos)
                    break;
            }
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
    WPHolder *ph=NULL;
    int fs=-1;
    int swf;
    bool ok, tmp;

    /* Check if param->tfor or any of its managers want to manage cwin. */
    if(param->tfor!=NULL){
        assert(param->tfor!=cwin);
        ph=region_prepare_manage_transient((WRegion*)param->tfor, cwin, 
                                           param, 0);
    }
    
    /* Find a suitable screen */
    scr=clientwin_find_suitable_screen(cwin, param);
    if(scr==NULL){
        warn(TR("Unable to find a screen for a new client window."));
        return FALSE;
    }
    
    if(ph==NULL){
        /* Find a placeholder for non-fullscreen state */
        ph=region_prepare_manage((WRegion*)scr, cwin, param,
                                 MANAGE_REDIR_PREFER_YES);
    }
    
    /* Check fullscreen mode */
    if(extl_table_gets_b(cwin->proptab, "fullscreen", &tmp))
        fs=tmp;

    if(fs<0)
        fs=netwm_check_initial_fullscreen(cwin, param->switchto);
    
    if(fs<0){
        fs=clientwin_check_fullscreen_request(cwin, 
                                              param->geom.w,
                                              param->geom.h,
                                              param->switchto);
    }

    if(fs>0){
        /* Full-screen mode succesfull. */
        if(pholder_target(ph)==(WRegion*)scr){
            /* Discard useless placeholder. */
            destroy_obj((Obj*)ph);
            return TRUE;
        }
        
        if(!region_do_set_return((WRegion*)cwin, ph))
            destroy_obj((Obj*)ph);
        
        return TRUE;
    }
        
    if(ph==NULL)
        return FALSE;
    
    /* Not in full-screen mode; use the placeholder to attach. */
    
    swf=(param->switchto ? PHOLDER_ATTACH_SWITCHTO : 0);
    ok=pholder_attach(ph, swf, (WRegion*)cwin);
    destroy_obj((Obj*)ph);
    
    return ok;
}


/*}}}*/


/*{{{ region_prepare_manage/region_manage_clientwin/etc. */


WPHolder *region_prepare_manage(WRegion *reg, const WClientWin *cwin,
                                const WManageParams *param, int redir)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, region_prepare_manage, reg, 
                 (reg, cwin, param, redir));
    return ret;
}


WPHolder *region_prepare_manage_default(WRegion *reg, const WClientWin *cwin,
                                        const WManageParams *param, int redir)
{
    WRegion *curr;
    
    if(redir==MANAGE_REDIR_STRICT_NO)
        return NULL;
    
    curr=region_current(reg);
    
    if(curr==NULL)
        return NULL;
        
    return region_prepare_manage(curr, cwin, param, MANAGE_REDIR_PREFER_YES);
}


WPHolder *region_prepare_manage_transient(WRegion *reg, 
                                          const WClientWin *cwin,
                                          const WManageParams *param,
                                          int unused)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, region_prepare_manage_transient, reg, 
                 (reg, cwin, param, unused));
    return ret;
}


WPHolder *region_prepare_manage_transient_default(WRegion *reg, 
                                                  const WClientWin *cwin,
                                                  const WManageParams *param,
                                                  int unused)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    if(mgr!=NULL)
        return region_prepare_manage_transient(mgr, cwin, param, unused);
    else
        return NULL;
}


bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
                             const WManageParams *par, int redir)
{
    bool ret;
    WPHolder *ph=region_prepare_manage(reg, cwin, par, redir);
    int swf=(par->switchto ? PHOLDER_ATTACH_SWITCHTO : 0);
    
    if(ph==NULL)
        return FALSE;
    
    ret=pholder_attach(ph, swf, (WRegion*)cwin);
    
    destroy_obj((Obj*)ph);
    
    return ret;
}


/*}}}*/


/*{{{ Rescue */


static WRegion *iter_children(void *st)
{
    WRegion **nextp=(WRegion**)st;
    WRegion *next=*nextp;
    *nextp=(next==NULL ? NULL : next->p_next);
    return next;   
}


bool region_rescue_child_clientwins(WRegion *reg, WPHolder *ph)
{
    WRegion *next=reg->children;
    return region_rescue_some_clientwins(reg, ph, iter_children, &next);
}


bool region_rescue_some_clientwins(WRegion *reg, WPHolder *ph,
                                   WRegionIterator *iter, void *st)
{
    bool res=FALSE;
    int fails=0;

    reg->flags|=REGION_CWINS_BEING_RESCUED;
    
    while(TRUE){
        WRegion *tosave=iter(st);
        
        if(tosave==NULL)
            break;
        
        if(!OBJ_IS(tosave, WClientWin)){
            if(!region_rescue_clientwins(tosave, ph))
                fails++;
        }else{
            if(!pholder_attach(ph, 0, tosave))
                fails++;
        }
    }
    
    reg->flags&=~REGION_CWINS_BEING_RESCUED;

    return (fails==0);
}


bool region_rescue_clientwins(WRegion *reg, WPHolder *ph)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, region_rescue_clientwins, reg, (reg, ph));
    return ret;
}


/*}}}*/


/*{{{ Misc. */


ExtlTab manageparams_to_table(const WManageParams *mp)
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
