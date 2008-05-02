/*
 * ion/ioncore/grouppholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2008. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libtu/pointer.h>
#include <libmainloop/defer.h>

#include <ioncore/common.h>
#include "group.h"
#include "group-cw.h"
#include "grouppholder.h"


static void group_watch_handler(Watch *watch, Obj *ws);


/*{{{ Primitives */


void grouppholder_do_link(WGroupPHolder *ph, WGroup *group, WRegion *stack_above)
{
    ph->group=group;
    
    if(group!=NULL){
        LINK_ITEM_FIRST(group->phs, ph, next, prev);
    }else{
        /* This seems very crucial for detached pholders... */
        ph->next=NULL;
        ph->prev=ph;
    }
    
    /* We must move stack_above pointer into a Watch. */
    if(stack_above!=NULL)
        watch_setup(&(ph->stack_above_watch), (Obj*)stack_above, NULL);
}


static WGroupPHolder *get_head(WGroupPHolder *ph)
{
    while(1){
        /* ph->prev==NULL should not happen.. */
        if(ph->prev==NULL || ph->prev->next==NULL)
            break;
        ph=ph->prev;
    }
    
    return ph;
}


void grouppholder_do_unlink(WGroupPHolder *ph)
{
    WGroup *group=ph->group;
    
    watch_reset(&(ph->stack_above_watch));
    
    if(ph->recreate_pholder!=NULL){
        if(ph->next!=NULL){
            ph->next->recreate_pholder=ph->recreate_pholder;
        }else{
            /* It might be in use in attach chain! So defer. */
            mainloop_defer_destroy((Obj*)ph->recreate_pholder);
        }
        ph->recreate_pholder=NULL;
    }
    
    if(group!=NULL){
        UNLINK_ITEM(group->phs, ph, next, prev);
    }else if(ph->prev!=NULL){
        WGroupPHolder *next=ph->next;
        
        ph->prev->next=next;

        if(next==NULL){
            next=get_head(ph);
            assert(next->prev==ph);
        }
        next->prev=ph->prev;
    }else{
        /* ph should not be on a list, if prev pointer is NULL (whereas
         * next alone can be NULL in our semi-doubly-linked lists).
         */
        assert(ph->next==NULL);
    }
    
    ph->group=NULL;
    ph->next=NULL;
    ph->prev=NULL;
}


/*}}}*/


/*{{{ Init/deinit */

static WGroupAttachParams dummy_param=GROUPATTACHPARAMS_INIT;


bool grouppholder_init(WGroupPHolder *ph, WGroup *ws,
                       const WStacking *st,
                       const WGroupAttachParams *param)
{
    WRegion *stack_above=NULL;
    
    pholder_init(&(ph->ph));

    watch_init(&(ph->stack_above_watch));
    ph->next=NULL;
    ph->prev=NULL;
    ph->group=NULL;
    ph->recreate_pholder=NULL;
    ph->param=(param==NULL ? dummy_param : *param);

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
        
        ph->param.bottom=(st==ws->bottom);
    }
    
    ph->param.switchto_set=FALSE;
    
    stack_above=ph->param.stack_above;
    ph->param.stack_above=NULL;
    
    grouppholder_do_link(ph, ws, stack_above);
    
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
    grouppholder_do_unlink(ph);
    
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Dynfuns */


static WPHolder *get_recreate_ph(WGroupPHolder *ph)
{
    return get_head(ph)->recreate_pholder;
}


typedef struct{
    WGroupPHolder *ph, *ph_head;
    WRegionAttachData *data;
    WRegion *reg_ret;
} RP;


static WRegion *recreate_handler(WWindow *par, 
                                 const WFitParams *fp, 
                                 void *rp_)
{
    WGroupPHolder *phtmp;
    RP *rp=(RP*)rp_;
    WGroup *grp;
    
    grp=(WGroup*)create_groupcw(par, fp);
    
    if(grp==NULL)
        return NULL;
        
    rp->ph->param.whatever=(fp->mode&REGION_FIT_WHATEVER ? 1 : 0);
    
    rp->reg_ret=group_do_attach(grp, &rp->ph->param, rp->data);
    
    rp->ph->param.whatever=0;
    
    if(rp->reg_ret==NULL){
        destroy_obj((Obj*)grp);
        return NULL;
    }else{
        grp->phs=rp->ph_head;
        
        for(phtmp=grp->phs; phtmp!=NULL; phtmp=phtmp->next)
            phtmp->group=grp;
    }
    
    if(fp->mode&REGION_FIT_WHATEVER)
        REGION_GEOM(grp)=REGION_GEOM(rp->reg_ret);
    
    return (WRegion*)grp;
}



static WRegion *grouppholder_attach_recreate(WGroupPHolder *ph, int flags,
                                             WRegionAttachData *data)
{
    WRegionAttachData data2;
    WPHolder *root, *rph;
    WGroup *grp;
    RP rp;
    
    rp.ph_head=get_head(ph);
    
    assert(rp.ph_head!=NULL);
    
    rph=rp.ph_head->recreate_pholder;
    
    if(rph==NULL)
        return NULL;
    
    rp.ph=ph;
    rp.data=data;
    rp.reg_ret=NULL;
    
    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=recreate_handler;
    data2.u.n.param=&rp;
    
    grp=(WGroup*)pholder_do_attach(rph, flags, &data2);
    
    if(grp!=NULL){
        assert(OBJ_IS(grp, WGroup));
        rp.ph_head->recreate_pholder=NULL;
        /* It might be in use in attach chain! So defer. */
        mainloop_defer_destroy((Obj*)rph);
    }

    return rp.reg_ret;
}


WRegion *grouppholder_do_attach(WGroupPHolder *ph, int flags,
                                WRegionAttachData *data)
{
    WGroup *ws=ph->group;
    WRegion *reg;

    if(ws==NULL)
        return grouppholder_attach_recreate(ph, flags, data);
    
    ph->param.switchto_set=1;
    ph->param.switchto=(flags&PHOLDER_ATTACH_SWITCHTO ? 1 : 0);
    
    /* Get stack_above from Watch. */
    ph->param.stack_above=(WRegion*)ph->stack_above_watch.obj;
    
    reg=group_do_attach(ws, &ph->param, data);
    
    ph->param.stack_above=NULL;

    return reg;
}


bool grouppholder_do_goto(WGroupPHolder *ph)
{
    return (ph->group!=NULL
            ? region_goto((WRegion*)ph->group)
            : (ph->recreate_pholder!=NULL
               ? pholder_do_goto(ph->recreate_pholder)
               : FALSE));
}


WRegion *grouppholder_do_target(WGroupPHolder *ph)
{
    return (ph->group!=NULL
            ? (WRegion*)ph->group
            : (ph->recreate_pholder!=NULL
               ? pholder_do_target(ph->recreate_pholder)
               : NULL));
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

