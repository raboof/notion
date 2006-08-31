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


static WGroupAttachParams dummy_param=GROUPATTACHPARAMS_INIT;


bool grouppholder_init(WGroupPHolder *ph, WGroup *ws,
                       const WStacking *st,
                       const WGroupAttachParams *param)
{
    pholder_init(&(ph->ph));

    watch_init(&(ph->group_watch));
    watch_init(&(ph->stack_above_watch));
    
    if(ws!=NULL){
        if(!watch_setup(&(ph->group_watch), (Obj*)ws, 
                        group_watch_handler)){
            pholder_deinit(&(ph->ph));
            return FALSE;
        }
    }
    
    if(param==NULL)
        param=&dummy_param;
    
    if(st!=NULL){
        /* TODO? Just link to the stacking structure to remember 
         * stacking order? 
         */
        
        ph->param.szplcy_set=TRUE;
        ph->param.szplcy=st->szplcy;
        ph->param.level_set=TRUE;
        ph->param.level=st->level;
        
        if(st->reg!=NULL){
            ph->param.geom_set=TRUE;
            ph->param.geom=REGION_GEOM(st->reg);
        }
        
        if(st->above!=NULL && st->above->reg!=NULL)
            ph->param.stack_above=st->above->reg;
        
        ph->param.modal=FALSE;
        ph->param.bottom=(st==ws->bottom);
        /*ph->passive=FALSE;*/
    }else{
        ph->param=*param;
    }

    ph->param.switchto_set=FALSE;

    if(ph->param.stack_above!=NULL){
        /* We must move stack_above pointer into a Watch. */
        watch_setup(&(ph->stack_above_watch), 
                    (Obj*)ph->param.stack_above, NULL);
        ph->param.stack_above=NULL;
    }
    
    return TRUE;
}
 

WGroupPHolder *create_grouppholder(WGroup *ws,
                                   const WStacking *st,
                                   const WGroupAttachParams *param)
{
    CREATEOBJ_IMPL(WGroupPHolder, grouppholder, (p, ws, st, param));
}


void grouppholder_deinit(WGroupPHolder *ph)
{
    watch_reset(&(ph->group_watch));
    watch_reset(&(ph->stack_above_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


bool grouppholder_do_attach(WGroupPHolder *ph, int flags,
                            WRegionAttachData *data)
{
    WGroup *ws=(WGroup*)ph->group_watch.obj;
    WRegion *reg;

    if(ws==NULL)
        return FALSE;

    ph->param.switchto_set=1;
    ph->param.switchto=(flags&PHOLDER_ATTACH_SWITCHTO ? 1 : 0);
    
    /* Get stack_above from Watch. */
    ph->param.stack_above=(WRegion*)ph->stack_above_watch.obj;
    
    reg=group_do_attach(ws, &ph->param, data);
    
    ph->param.stack_above=NULL;

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
        return create_grouppholder(ws, st, NULL);
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

