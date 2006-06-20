/*
 * ion/mod_floatws/floatwspholder.c
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
#include "floatws.h"
#include "floatwspholder.h"


static void floatws_watch_handler(Watch *watch, Obj *ws);


/*{{{ Init/deinit */


static void floatws_watch_handler(Watch *watch, Obj *ws)
{
    WFloatWSPHolder *ph=FIELD_TO_STRUCT(WFloatWSPHolder, 
                                        floatws_watch, watch);
    pholder_redirect(&(ph->ph), (WRegion*)ws);
}


bool floatwspholder_init(WFloatWSPHolder *ph, WFloatWS *ws,
                         const WStacking *st)
{
    pholder_init(&(ph->ph));

    watch_init(&(ph->floatws_watch));
    
    if(ws==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->floatws_watch), (Obj*)ws, 
                    floatws_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }
    
    /* TODO? Just link to the stacking structure to remember 
     * stacking order? 
     */
    
    ph->szplcy=st->szplcy;
    ph->level=st->level;
    
    if(st->reg!=NULL)
        ph->geom=REGION_GEOM(st->reg);
    else
        ph->geom=REGION_GEOM(ws);
    
    return TRUE;
}
 

WFloatWSPHolder *create_floatwspholder(WFloatWS *ws,
                                       const WStacking *st)
{
    CREATEOBJ_IMPL(WFloatWSPHolder, floatwspholder, (p, ws, st));
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
    WRegion *reg;
    WFloatWSAttachParams param;

    if(ws==NULL)
        return FALSE;

    param.level_set=1;
    param.level=ph->level;
    
    param.szplcy_set=1;
    param.szplcy=ph->szplcy;
    
    param.geom_set=1;
    param.geom=ph->geom;
    
    param.switchto_set=1;
    param.switchto=(flags&PHOLDER_ATTACH_SWITCHTO ? 1 : 0);
    
    /* Should these be remembered? */
    param.modal=0;
    param.sticky=0;
    param.bottom=0;
    
    reg=floatws_do_attach(ws, hnd, hnd_param, &param);

    return (reg!=NULL);
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


WFloatWSPHolder *floatws_managed_get_pholder(WFloatWS *ws, WRegion *mgd)
{
    WStacking *st=floatws_find_stacking(ws, mgd);
    
    if(mgd==NULL)
        return NULL;
    else
        return create_floatwspholder(ws, st);
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

