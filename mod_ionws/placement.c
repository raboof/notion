/*
 * ion/mod_ionws/placement.c
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
#include "ionframe.h"
#include "ionws.h"


static WRegion *find_suitable_target(WIonWS *ws)
{
    WRegion *r=ionws_current(ws);
    
    if(r!=NULL)
        return r;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r)
        return r;
    
    return NULL;
}


bool ionws_manage_clientwin(WIonWS *ws, WClientWin *cwin,
                            const WManageParams *param,
                            int redir)
{
    WRegion *target=NULL;
    
    if(redir==MANAGE_REDIR_STRICT_NO)
        return FALSE;

    extl_call_named("ionws_placement_method", "oob", "o", 
                    ws, cwin, param->userpos, &target);
    
    if(target!=NULL && REGION_MANAGER(target)==(WRegion*)ws){
        if(region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES))
            return TRUE;
    }

    target=find_suitable_target(ws);
    
    if(target==NULL){
        warn("Ooops... could not find a region to attach client window to "
             "on workspace %s.", region_name((WRegion*)ws));
        return FALSE;
    }
    
    return region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES);
}

