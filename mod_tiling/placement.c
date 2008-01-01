/*
 * ion/mod_tiling/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
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
#include "tiling.h"


WHook *tiling_placement_alt=NULL;


static WRegion *find_suitable_target(WTiling *ws)
{
    WRegion *r=tiling_current(ws);
    
    if(r==NULL){
        FOR_ALL_MANAGED_BY_TILING_UNSAFE(r, ws)
            break;
    }
    
    return r;
}


static bool placement_mrsh_extl(ExtlFn fn, WTilingPlacementParams *param)
{
    ExtlTab t, mp;
    bool ret=FALSE;
    
    t=extl_create_table();
    
    mp=manageparams_to_table(param->mp);
    
    extl_table_sets_o(t, "tiling", (Obj*)param->ws);
    extl_table_sets_o(t, "reg", (Obj*)param->reg);
    extl_table_sets_t(t, "mp", mp);
    
    extl_unref_table(mp);
    
    extl_protect(NULL);
    ret=extl_call(fn, "t", "b", t, &ret);
    extl_unprotect(NULL);
    
    if(ret){
        Obj *tmp=NULL;
        
        extl_table_gets_o(t, "res_frame", &tmp);
        
        param->res_frame=OBJ_CAST(tmp, WFrame);
        ret=(param->res_frame!=NULL);
    }
            
    extl_unref_table(t);
    
    return ret;
}


WPHolder *tiling_prepare_manage(WTiling *ws, const WClientWin *cwin,
                                const WManageParams *mp, int priority)
{
    int cpriority=MANAGE_PRIORITY_SUBX(priority, MANAGE_PRIORITY_NORMAL);
    WRegion *target=NULL;
    WTilingPlacementParams param;
    WPHolder *ph;
    bool ret;
    
    param.ws=ws;
    param.reg=(WRegion*)cwin;
    param.mp=mp;
    param.res_frame=NULL;
    
    ret=hook_call_alt_p(tiling_placement_alt, &param, 
                        (WHookMarshallExtl*)placement_mrsh_extl);
        
    if(ret && param.res_frame!=NULL &&
       REGION_MANAGER(param.res_frame)==(WRegion*)ws){
        
        target=(WRegion*)param.res_frame;
        
        ph=region_prepare_manage(target, cwin, mp, cpriority);
        if(ph!=NULL)
            return ph;
    }

    target=find_suitable_target(ws);
    
    if(target==NULL){
        warn(TR("Ooops... could not find a region to attach client window "
                "to on workspace %s."), region_name((WRegion*)ws));
        return NULL;
    }
    
    return region_prepare_manage(target, cwin, mp, cpriority);
}

