/*
 * ion/mod_floatws/floatwspholder.c
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
#include "floatwspholder.h"


static void floatws_watch_handler(Watch *watch, Obj *floatws);


/*{{{ Init/deinit */


static void floatws_watch_handler(Watch *watch, Obj *floatws)
{
    WFloatWSPHolder *ph=FIELD_TO_STRUCT(WFloatWSPHolder, 
                                        floatws_watch, watch);
    pholder_redirect(&(ph->ph), (WRegion*)floatws);
}


bool floatwspholder_init(WFloatWSPHolder *ph, WFloatWS *floatws,
                         const WRectangle *geom)
{
    pholder_init(&(ph->ph));

    watch_init(&(ph->floatws_watch));
    
    if(floatws==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->floatws_watch), (Obj*)floatws, 
                    floatws_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }
    
    ph->geom=*geom;
    
    return TRUE;
}
 

WFloatWSPHolder *create_floatwspholder(WFloatWS *floatws,
                                       const WRectangle *geom)
{
    CREATEOBJ_IMPL(WFloatWSPHolder, floatwspholder, 
                   (p, floatws, geom));
}


void floatwspholder_deinit(WFloatWSPHolder *ph)
{
    watch_reset(&(ph->floatws_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


bool floatwspholder_do_attach(WFloatWSPHolder *ph, 
                              WRegionAttachHandler *hnd, void *hnd_param,
                              int flags)
{
    WFloatWS *ws=(WFloatWS*)ph->floatws_watch.obj;
    WFitParams fp;
    WRegion *reg;
    WWindow *par;

    if(ws==NULL)
        return FALSE;
    
    par=REGION_PARENT(ws);
    
    if(par==NULL)
        return FALSE;
    
    fp.g=ph->geom;
    fp.mode=REGION_FIT_EXACT;

    reg=hnd(par, &fp, hnd_param);
    
    if(reg==NULL)
        return FALSE;
    
    /* TODO: flags/switchto */
    
    floatws_add_managed(ws, reg);

    return TRUE;
}


bool floatwspholder_do_goto(WFloatWSPHolder *ph)
{
    WFloatWS *ws=(WFloatWS*)ph->floatws_watch.obj;
    
    if(ws!=NULL)
        return region_goto((WRegion*)ws);
    
    return FALSE;
}


WRegion *floatwspholder_do_target(WFloatWSPHolder *ph)
{
    return (WRegion*)ph->floatws_watch.obj;
}


/*}}}*/


/*{{{ WFloatWS stuff */


WFloatWSPHolder *floatws_managed_get_pholder(WFloatWS *floatws, WRegion *mgd)
{
    return create_floatwspholder(floatws, &REGION_GEOM(mgd));
}


/*}}}*/


/*{{{ Class information */


static DynFunTab floatwspholder_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)floatwspholder_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)floatwspholder_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)floatwspholder_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WFloatWSPHolder, WPHolder, floatwspholder_deinit, 
          floatwspholder_dynfuntab);


/*}}}*/

