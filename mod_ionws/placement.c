/*
 * ion/mod_ionws/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/clientwin.h>
#include <ioncore/attach.h>
#include <ioncore/manage.h>
#include <libextl/extl.h>
#include <ioncore/framep.h>
#include <ioncore/names.h>
#include "placement.h"
#include "ionws.h"


WHook *ionws_placement_alt=NULL;


static WRegion *find_suitable_target(WIonWS *ws)
{
    WRegion *r=ionws_current(ws);
    
    if(r==NULL){
        FOR_ALL_MANAGED_BY_IONWS_UNSAFE(r, ws)
            break;
    }
    
    return r;
}


static bool placement_mrsh_extl(ExtlFn fn, WIonWSPlacementParams *param)
{
    ExtlTab t, mp;
    bool ret=FALSE;
    
    t=extl_create_table();
    
    mp=manageparams_to_table(param->mp);
    
    extl_table_sets_o(t, "ws", (Obj*)param->ws);
    extl_table_sets_o(t, "reg", (Obj*)param->reg);
    extl_table_sets_t(t, "mp", mp);
    
    extl_unref_table(mp);
    
    extl_protect(NULL);
    ret=extl_call(fn, "t", "b", t, &ret);
    extl_unprotect(NULL);
    
    if(ret){
        Obj *tmp;
        
        extl_table_gets_o(t, "res_frame", &tmp);
        
        param->res_frame=OBJ_CAST(tmp, WFrame);
        ret=(param->res_frame!=NULL);
    }
            
    extl_unref_table(t);
    
    return ret;
}


WPHolder *ionws_prepare_manage(WIonWS *ws, const WClientWin *cwin,
                               const WManageParams *mp, int redir)
{
    WRegion *target=NULL;
    WIonWSPlacementParams param;
    WPHolder *ph;
    bool ret;

    if(redir==MANAGE_REDIR_STRICT_NO)
        return NULL;

    param.ws=ws;
    param.reg=(WRegion*)cwin;
    param.mp=mp;
    param.res_frame=NULL;
    
    ret=hook_call_alt_p(ionws_placement_alt, &param, 
                        (WHookMarshallExtl*)placement_mrsh_extl);
        
    if(ret && param.res_frame!=NULL &&
       REGION_MANAGER(param.res_frame)==(WRegion*)ws){
        
        target=(WRegion*)param.res_frame;
        
        ph=region_prepare_manage(target, cwin, mp, redir);
        if(ph!=NULL)
            return ph;
    }

    target=find_suitable_target(ws);
    
    if(target==NULL){
        warn(TR("Ooops... could not find a region to attach client window "
                "to on workspace %s."), region_name((WRegion*)ws));
        return NULL;
    }
    
    return region_prepare_manage(target, cwin, mp, redir);
}

