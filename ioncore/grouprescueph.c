/*
 * ion/ioncore/grouprescueph.c
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
#include "grouprescueph.h"


/*{{{ Init/deinit */


bool grouprescueph_init(WGroupRescuePH *ph, WGroup *grp,
                        const WStacking *either_st,
                        const WGroupAttachParams *or_param)
{
    if(!grouppholder_init(&ph->gph, grp, either_st, or_param))
        return FALSE;
    
    watch_init(&(ph->frame_watch));
    
    return TRUE;
}
 

WGroupRescuePH *create_grouprescueph(WGroup *grp,
                                     const WStacking *either_st,
                                     const WGroupAttachParams *or_param)
{
    CREATEOBJ_IMPL(WGroupRescuePH, grouprescueph, 
                   (p, grp, either_st, or_param));
}


void grouprescueph_deinit(WGroupRescuePH *ph)
{
    watch_reset(&(ph->frame_watch));
    grouppholder_deinit(&(ph->gph));
}


/*}}}*/


/*{{{ Dynfuns */


bool grouprescueph_do_attach(WGroupRescuePH *ph, int flags,
                             WRegionAttachData *data)
{
    WGroup *ws=(WGroup*)ph->gph.group_watch.obj;
    WRegion *reg;
    
    if(ws==NULL)
        return FALSE;

    /* Get stack_above from Watch. */
    ph->gph.param.stack_above=(WRegion*)ph->gph.stack_above_watch.obj;

    reg=group_do_attach_framed(ws, &(ph->gph.param), data);
    
    ph->gph.param.stack_above=NULL;
    
    if(reg!=NULL){
        WFrame *frame=REGION_MANAGER_CHK(reg, WFrame);
        if(frame!=NULL && !watch_ok(&(ph->frame_watch)))
            assert(watch_setup(&(ph->frame_watch), (Obj*)frame, NULL));
    }
    
    return (reg!=NULL);
}


bool grouprescueph_do_goto(WGroupRescuePH *ph)
{
    WGroup *ws=(WGroup*)ph->gph.group_watch.obj;
    WFrame *frame=(WFrame*)ph->frame_watch.obj;
    
    if(frame!=NULL)
        return region_goto((WRegion*)frame);
    else if(ws!=NULL)
        return region_goto((WRegion*)ws);
    
    return FALSE;
}


WRegion *grouprescueph_do_target(WGroupRescuePH *ph)
{
    WRegion *ws=(WRegion*)ph->gph.group_watch.obj;
    WRegion *frame=(WRegion*)ph->frame_watch.obj;
    
    return (frame!=NULL ? frame : ws);
}


/*}}}*/


/*{{{ WGroupWS stuff */


WGroupRescuePH *groupws_get_rescue_pholder_for(WGroupWS *ws, 
                                               WRegion *forwhat)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    
    ap.geom_set=TRUE;
    ap.geom=REGION_GEOM(forwhat);

    ap.pos_not_ok=(REGION_PARENT(forwhat)!=REGION_PARENT(ws));
    ap.framed_inner_geom=FALSE;
    ap.framed_gravity=StaticGravity;

    return create_grouprescueph(&ws->grp, NULL, &ap);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab grouprescueph_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)grouprescueph_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)grouprescueph_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)grouprescueph_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WGroupRescuePH, WPHolder, grouprescueph_deinit, 
          grouprescueph_dynfuntab);


/*}}}*/

