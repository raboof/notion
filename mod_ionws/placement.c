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
#include "ionws.h"


WHook *ionws_placement_alt=NULL;


static WRegion *find_suitable_target(WIonWS *ws)
{
    WRegion *r=ionws_current(ws);
    
    if(r!=NULL)
        return r;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r)
        return r;
    
    return NULL;
}


static bool placement_mrsh(bool (*fn)(WClientWin *cwin, WIonWS *ws,
                                      WManageParams *pm),
                           void **p)
{
    return fn((WClientWin*)p[0], 
              (WIonWS*)p[1],
              (WManageParams*)p[2]);
}



static bool placement_mrsh_extl(ExtlFn fn, void **p)
{
    WClientWin *cwin=(WClientWin*)p[0];
    WIonWS *ws=(WIonWS*)p[1];
    WManageParams *mp=(WManageParams*)p[2];
    ExtlTab t=manageparams_to_table(mp);
    bool ret=FALSE;
    
    extl_call(fn, "oot", "b", cwin, ws, t, &ret);
    
    extl_unref_table(t);
    
    return (ret && REGION_MANAGER(cwin)!=NULL);
}


bool ionws_manage_clientwin(WIonWS *ws, WClientWin *cwin,
                            const WManageParams *param,
                            int redir)
{
    WRegion *target=NULL;
    
    if(redir==MANAGE_REDIR_STRICT_NO)
        return FALSE;

    {
        void *mrshpm[3];
        bool managed;
        
        mrshpm[0]=cwin;
        mrshpm[1]=ws;
        mrshpm[2]=&param;
        
        managed=hook_call_alt(ionws_placement_alt, &mrshpm, 
                              (WHookMarshall*)placement_mrsh,
                              (WHookMarshallExtl*)placement_mrsh_extl);
        
        if(managed && REGION_MANAGER(cwin)!=NULL)
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

