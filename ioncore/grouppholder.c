/*
 * ion/ioncore/grouppholder.c
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
#include "group.h"
#include "grouppholder.h"


static void group_watch_handler(Watch *watch, Obj *ws);


/*{{{ Init/deinit */


static void group_watch_handler(Watch *watch, Obj *ws)
{
    WGroupPHolder *ph=FIELD_TO_STRUCT(WGroupPHolder, 
                                        group_watch, watch);
    pholder_redirect(&(ph->ph), (WRegion*)ws);
}


bool grouppholder_init(WGroupPHolder *ph, WGroup *ws,
                         const WStacking *st)
{
    pholder_init(&(ph->ph));

    watch_init(&(ph->group_watch));
    
    if(ws==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->group_watch), (Obj*)ws, 
                    group_watch_handler)){
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
 

WGroupPHolder *create_grouppholder(WGroup *ws,
                                       const WStacking *st)
{
    CREATEOBJ_IMPL(WGroupPHolder, grouppholder, (p, ws, st));
}


void grouppholder_deinit(WGroupPHolder *ph)
{
    watch_reset(&(ph->group_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


bool grouppholder_do_attach(WGroupPHolder *ph, 
                              WRegionAttachHandler *hnd, void *hnd_param,
                              int flags)
{
    WGroup *ws=(WGroup*)ph->group_watch.obj;
    WRegion *reg;
    WGroupAttachParams param;

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
    param.sticky=0;
    param.bottom=0;
    
    reg=group_do_attach(ws, hnd, hnd_param, &param);

    return (reg!=NULL);
}


bool grouppholder_do_goto(WGroupPHolder *ph)
{
    WGroup *ws=(WGroup*)ph->group_watch.obj;
    
    if(ws!=NULL)
        return region_goto((WRegion*)ws);
    
    return FALSE;
}


WRegion *grouppholder_do_target(WGroupPHolder *ph)
{
    return (WRegion*)ph->group_watch.obj;
}


/*}}}*/


/*{{{ WGroup stuff */


WGroupPHolder *group_managed_get_pholder(WGroup *ws, WRegion *mgd)
{
    WStacking *st=group_find_stacking(ws, mgd);
    
    if(mgd==NULL)
        return NULL;
    else
        return create_grouppholder(ws, st);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab grouppholder_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)grouppholder_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)grouppholder_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)grouppholder_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WGroupPHolder, WPHolder, grouppholder_deinit, 
          grouppholder_dynfuntab);


/*}}}*/

