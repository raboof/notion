/*
 * ion/ioncore/groupwsrescueph.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libtu/pointer.h>

#include <ioncore/common.h>
#include "group-ws.h"
#include "group-ws-rescueph.h"


static void groupws_watch_handler(Watch *watch, Obj *groupws);


/*{{{ Init/deinit */


static void groupws_watch_handler(Watch *watch, Obj *groupws)
{
    WGroupWSRescuePH *ph=FIELD_TO_STRUCT(WGroupWSRescuePH, 
                                         groupws_watch, watch);
    pholder_redirect(&(ph->ph), (WRegion*)groupws);
}


bool groupwsrescueph_init(WGroupWSRescuePH *ph, WGroupWS *ws,
                          const WRectangle *geom, 
                          bool pos_ok, bool inner_geom, int gravity)
{
    pholder_init(&(ph->ph));

    ph->geom=*geom;
    ph->pos_ok=pos_ok;
    ph->inner_geom=inner_geom;
    ph->gravity=gravity;
    
    watch_init(&(ph->groupws_watch));
    watch_init(&(ph->frame_watch));
    watch_init(&(ph->stack_above_watch));
    
    if(ws==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->groupws_watch), (Obj*)ws, groupws_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }
    
    return TRUE;
}
 

WGroupWSRescuePH *create_groupwsrescueph(WGroupWS *groupws,
                                         const WRectangle *geom, 
                                         bool pos_ok, bool inner_geom, 
                                         int gravity)
{
    CREATEOBJ_IMPL(WGroupWSRescuePH, groupwsrescueph, 
                   (p, groupws, geom, pos_ok, inner_geom, gravity));
}


void groupwsrescueph_deinit(WGroupWSRescuePH *ph)
{
    watch_reset(&(ph->groupws_watch));
    watch_reset(&(ph->frame_watch));
    watch_reset(&(ph->stack_above_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


bool groupwsrescueph_do_attach(WGroupWSRescuePH *ph, 
                               WRegionAttachHandler *hnd, void *hnd_param,
                               int flags)
{
    WGroupWS *ws=(WGroupWS*)ph->groupws_watch.obj;
    WGroupWSPHAttachParams p;
    bool ok;
    
    if(ws==NULL)
        return FALSE;

    p.frame=(WFrame*)ph->frame_watch.obj;
    p.geom=ph->geom;
    p.inner_geom=ph->inner_geom;
    p.pos_ok=ph->pos_ok;
    p.gravity=ph->gravity;
    p.aflags=flags;
    p.stack_above=(WRegion*)ph->stack_above_watch.obj;
        
    ok=groupws_phattach(ws, hnd, hnd_param, &p);
    
    if(p.frame!=NULL && !watch_ok(&(ph->frame_watch)))
        assert(watch_setup(&(ph->frame_watch), (Obj*)p.frame, NULL));
    
    return ok;
}


bool groupwsrescueph_do_goto(WGroupWSRescuePH *ph)
{
    WGroupWS *ws=(WGroupWS*)ph->groupws_watch.obj;
    WFrame *frame=(WFrame*)ph->frame_watch.obj;
    
    if(frame!=NULL)
        return region_goto((WRegion*)frame);
    else if(ws!=NULL)
        return region_goto((WRegion*)ws);
    
    return FALSE;
}


WRegion *groupwsrescueph_do_target(WGroupWSRescuePH *ph)
{
    WRegion *ws=(WRegion*)ph->groupws_watch.obj;
    WRegion *frame=(WRegion*)ph->frame_watch.obj;
    
    return (frame!=NULL ? frame : ws);
}


/*}}}*/


/*{{{ WGroupWS stuff */


WGroupWSRescuePH *groupws_get_rescue_pholder_for(WGroupWS *groupws, 
                                                 WRegion *forwhat)
{    
    bool pos_ok=FALSE;
    bool inner_geom=FALSE;
    WRectangle geom=REGION_GEOM(forwhat);

    if(REGION_PARENT(forwhat)==REGION_PARENT(groupws))
        pos_ok=TRUE;

    return create_groupwsrescueph(groupws, &geom, pos_ok, inner_geom,
                                  StaticGravity);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab groupwsrescueph_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)groupwsrescueph_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)groupwsrescueph_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)groupwsrescueph_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WGroupWSRescuePH, WPHolder, groupwsrescueph_deinit, 
          groupwsrescueph_dynfuntab);


/*}}}*/

