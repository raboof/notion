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
                          WRegion *contents, WRegion *or_this)
{
    assert(contents!=NULL || or_this!=NULL);
    
    pholder_init(&(ph->ph));

    ph->pos_ok=FALSE;
    ph->inner_geom=FALSE;
    
    watch_init(&(ph->floatws_watch));
    watch_init(&(ph->frame_watch));
    
    if(ws==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->floatws_watch), (Obj*)ws, floatws_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }
    
    if(contents!=NULL){
        ph->geom=REGION_GEOM(contents);
        if(REGION_PARENT(contents)==REGION_PARENT(ws))
            ph->pos_ok=TRUE;
    }else{ /* or_this!=NULL */
        WRegion *mgr=REGION_MANAGER(or_this);
        if(REGION_PARENT(contents)==REGION_PARENT(ws))
            ph->pos_ok=TRUE;

        ph->geom=REGION_GEOM(or_this);
        ph->inner_geom=TRUE;
    }
    
    return TRUE;
}
 

WFloatWSRescuePH *create_floatwsrescueph(WFloatWS *floatws,
                                         WRegion *contents, WRegion *or_this)
{
    CREATEOBJ_IMPL(WFloatWSRescuePH, floatwsrescueph, 
                   (p, floatws, contents, or_this));
}


void floatwsrescueph_deinit(WFloatWSRescuePH *ph)
{
    watch_reset(&(ph->floatws_watch));
    watch_reset(&(ph->frame_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


bool floatwsrescueph_stale(WFloatWSRescuePH *ph)
{
    return (ph->floatws_watch.obj==NULL);
}


bool floatwsrescueph_do_attach(WFloatWSRescuePH *ph, WRegionAttachHandler *hnd,
                               void *hnd_param)
{
    WFloatWS *ws=(WFloatWS*)ph->floatws_watch.obj;
    WFrame *frame=(WFrame*)ph->frame_watch.obj;

    if(ws==NULL)
        return FALSE;
    
    if(frame==NULL){
        frame=(WFrame*)floatws_create_frame(ws, &(ph->geom), StaticGravity,
                                            ph->inner_geom, ph->pos_ok);
        
        if(frame==NULL)
            return FALSE;
        
        assert(watch_setup(&(ph->frame_watch), (Obj*)frame, NULL));
    }
    
    return (mplex_attach_hnd((WMPlex*)frame, hnd, hnd_param, 0)!=NULL);
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


/*}}}*/


/*{{{ WFloatWS stuff */


WFloatWSRescuePH *floatws_get_rescue_pholder_for(WFloatWS *floatws, 
                                                 WRegion *forwhat)
{
    return create_floatwsrescueph(floatws, forwhat, NULL);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab floatwsrescueph_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)floatwsrescueph_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)floatwsrescueph_do_goto},
    
    {(DynFun*)pholder_stale, 
     (DynFun*)floatwsrescueph_stale},
    
    END_DYNFUNTAB
};

IMPLCLASS(WFloatWSRescuePH, WPHolder, floatwsrescueph_deinit, 
          floatwsrescueph_dynfuntab);


/*}}}*/

