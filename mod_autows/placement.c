/*
 * ion/mod_autows/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <ioncore/extl.h>
#include <ioncore/framep.h>
#include <ioncore/names.h>
#include <ioncore/region-iter.h>
#include "placement.h"
#include "autoframe.h"
#include "autows.h"


static WRegion *find_suitable_target(WIonWS *ws)
{
    WRegion *r=ionws_current(ws);
    
    if(r!=NULL)
        return r;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r)
        return r;
    
    return NULL;
}


static WRegion *create_initial_frame(WIonWS *ws, WWindow *parent,
                                     const WFitParams *fp)
{
    WRegion *reg=ws->create_frame_fn(parent, fp);

    if(reg==NULL)
        return NULL;
    
    ws->split_tree=create_split_regnode(reg, &(fp->g));
    if(ws->split_tree==NULL){
        warn_err();
        destroy_obj((Obj*)reg);
        return NULL;
    }
    
    ionws_managed_add(ws, reg);

    return reg;
}



bool autows_manage_clientwin(WAutoWS *ws, WClientWin *cwin,
                            const WManageParams *param,
                            int redir)
{
    WRegion *target=NULL;
    
    if(ws->ionws.split_tree==NULL){
        WFitParams param;
        param.g=REGION_GEOM(ws);
        param.mode=REGION_FIT_BOUNDS;
        target=create_initial_frame(&(ws->ionws),  /* \/ !!! */
                                    (WWindow*)REGION_PARENT(ws), &param);
    }
    
    if(target!=NULL && REGION_MANAGER(target)==(WRegion*)ws){
        if(region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES))
            return TRUE;
    }

    target=find_suitable_target(&(ws->ionws));
    
    if(target==NULL){
        warn("Ooops... could not find a region to attach client window to "
             "on workspace %s.", region_name((WRegion*)ws));
        return FALSE;
    }
    
    return region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES);
}

