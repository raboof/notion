/*
 * ion/ioncore/groupedpholder.c
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

#include "group.h"
#include "group-cw.h"
#include "groupedpholder.h"


/*{{{ Init/deinit */


bool groupedpholder_init(WGroupedPHolder *ph, WPHolder *cont)
{
    assert(cont!=NULL);
    
    pholder_init(&(ph->ph));

    ph->cont=cont;
    
    return TRUE;
}
 

WGroupedPHolder *create_groupedpholder(WPHolder *cont)
{
    CREATEOBJ_IMPL(WGroupedPHolder, groupedpholder, (p, cont));
}


void groupedpholder_deinit(WGroupedPHolder *ph)
{
    if(ph->cont!=NULL){
        destroy_obj((Obj*)ph->cont);
        ph->cont=NULL;
    }
    
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Attach */


WRegion *grouped_handler(WWindow *par, 
                         const WFitParams *fp, 
                         void *frp_)
{
    WRegionAttachData *data=(WRegionAttachData*)frp_;
    WGroupAttachParams param=GROUPATTACHPARAMS_INIT;
    WGroupCW *cwg;
    WRegion *reg;
    WStacking *st;
    
    cwg=create_groupcw(par, fp);
    
    if(cwg==NULL)
        return NULL;
    
    param.level_set=1;
    param.level=STACKING_LEVEL_BOTTOM;
    param.switchto_set=1;
    param.switchto=1;
    param.bottom=1;
    
    if(!(fp->mode&REGION_FIT_WHATEVER)){
        param.geom_set=1;
        param.geom.x=0;
        param.geom.y=0;
        param.geom.w=fp->g.w;
        param.geom.h=fp->g.h;
        param.szplcy=SIZEPOLICY_FULL_EXACT;
        param.szplcy_set=TRUE;
    }else{
    }
    
    reg=group_do_attach(&cwg->grp, &param, data);
    
    if(reg==NULL){
        destroy_obj((Obj*)cwg);
        return NULL;
    }
    
    return (WRegion*)cwg;
}


bool groupedpholder_do_attach(WGroupedPHolder *ph, int flags,
                              WRegionAttachData *data)
{
    WRegionAttachData data2;
    
    if(ph->cont==NULL)
        return FALSE;

    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=grouped_handler;
    data2.u.n.param=data;
        
    return pholder_attach_(ph->cont, flags, &data2);
}


/*}}}*/


/*{{{ Other dynfuns */


bool groupedpholder_do_goto(WGroupedPHolder *ph)
{
    if(ph->cont!=NULL)
        return pholder_goto(ph->cont);
    
    return FALSE;
}


WRegion *groupedpholder_do_target(WGroupedPHolder *ph)
{
    if(ph->cont!=NULL)
        return pholder_target(ph->cont);
    
    return NULL;
}


/*}}}*/


/*{{{ Class information */


static DynFunTab groupedpholder_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)groupedpholder_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)groupedpholder_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)groupedpholder_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WGroupedPHolder, WPHolder, groupedpholder_deinit, 
          groupedpholder_dynfuntab);


/*}}}*/

