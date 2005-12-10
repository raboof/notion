/*
 * ion/mod_floatws/floatwsrescueph.c
 *
 * Copyright (c) Tuomo Valkonen 2005. 
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
#include "floatws.h"
#include "floatwsrescueph.h"


static void floatws_watch_handler(Watch *watch, Obj *floatws);


/*{{{ Init/deinit */


static void floatws_watch_handler(Watch *watch, Obj *floatws)
{
    WFloatWSRescuePH *ph=FIELD_TO_STRUCT(WFloatWSRescuePH, 
                                         floatws_watch, watch);
    pholder_redirect(&(ph->ph), (WRegion*)floatws);
}


bool floatwsrescueph_init(WFloatWSRescuePH *ph, WFloatWS *ws,
                          const WRectangle *geom, 
                          bool pos_ok, bool inner_geom, int gravity)
{
    pholder_init(&(ph->ph));

    ph->geom=*geom;
    ph->pos_ok=pos_ok;
    ph->inner_geom=inner_geom;
    ph->gravity=gravity;
    
    watch_init(&(ph->floatws_watch));
    watch_init(&(ph->frame_watch));
    watch_init(&(ph->stack_above_watch));
    
    if(ws==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->floatws_watch), (Obj*)ws, floatws_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }
    
    return TRUE;
}
 

WFloatWSRescuePH *create_floatwsrescueph(WFloatWS *floatws,
                                         const WRectangle *geom, 
                                         bool pos_ok, bool inner_geom, 
                                         int gravity)
{
    CREATEOBJ_IMPL(WFloatWSRescuePH, floatwsrescueph, 
                   (p, floatws, geom, pos_ok, inner_geom, gravity));
}


void floatwsrescueph_deinit(WFloatWSRescuePH *ph)
{
    watch_reset(&(ph->floatws_watch));
    watch_reset(&(ph->frame_watch));
    watch_reset(&(ph->stack_above_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


bool floatwsrescueph_do_attach(WFloatWSRescuePH *ph, 
                               WRegionAttachHandler *hnd, void *hnd_param,
                               int flags)
{
    WFloatWS *ws=(WFloatWS*)ph->floatws_watch.obj;
    WFloatWSPHAttachParams p;
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
        
    ok=floatws_phattach(ws, hnd, hnd_param, &p);
    
    if(p.frame!=NULL && !watch_ok(&(ph->frame_watch)))
        assert(watch_setup(&(ph->frame_watch), (Obj*)p.frame, NULL));
    
    return ok;
}


bool floatwsrescueph_do_goto(WFloatWSRescuePH *ph)
{
    WFloatWS *ws=(WFloatWS*)ph->floatws_watch.obj;
    WFrame *frame=(WFrame*)ph->frame_watch.obj;
    
    if(frame!=NULL)
        return region_goto((WRegion*)frame);
    else if(ws!=NULL)
        return region_goto((WRegion*)ws);
    
    return FALSE;
}


WRegion *floatwsrescueph_do_target(WFloatWSRescuePH *ph)
{
    WRegion *ws=(WRegion*)ph->floatws_watch.obj;
    WRegion *frame=(WRegion*)ph->frame_watch.obj;
    
    return (frame!=NULL ? frame : ws);
}


/*}}}*/


/*{{{ WFloatWS stuff */


WFloatWSRescuePH *floatws_get_rescue_pholder_for(WFloatWS *floatws, 
                                                 WRegion *forwhat)
{    
    bool pos_ok=FALSE;
    bool inner_geom=FALSE;
    WRectangle geom=REGION_GEOM(forwhat);

    if(REGION_PARENT(forwhat)==REGION_PARENT(floatws))
        pos_ok=TRUE;

    return create_floatwsrescueph(floatws, &geom, pos_ok, inner_geom,
                                  StaticGravity);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab floatwsrescueph_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)floatwsrescueph_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)floatwsrescueph_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)floatwsrescueph_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WFloatWSRescuePH, WPHolder, floatwsrescueph_deinit, 
          floatwsrescueph_dynfuntab);


/*}}}*/

