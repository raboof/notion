/*
 * ion/ioncore/groupedpholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <libtu/obj.h>

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


static bool grouped_do_attach_final(WGroupCW *cwg,
                                    WRegion *reg,
                                    WGroupAttachParams *param)
{
    if(!param->geom_set){
        /* Comm. hack */
        REGION_GEOM(cwg)=REGION_GEOM(reg);
    }

    param->geom_set=1;
    param->geom.x=0;
    param->geom.y=0;
    param->geom.w=REGION_GEOM(reg).w;
    param->geom.h=REGION_GEOM(reg).h;
    param->szplcy=SIZEPOLICY_FULL_EXACT;
    param->szplcy_set=TRUE;

    return group_do_attach_final(&cwg->grp, reg, param);
}


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
        /* Comm. hack */
        param.geom_set=TRUE;
    }

    reg=region_attach_helper((WRegion*)cwg, par, fp,
                             (WRegionDoAttachFn*)grouped_do_attach_final,
                             &param, data);

    if(reg==NULL){
        destroy_obj((Obj*)cwg);
        return NULL;
    }

    return (WRegion*)cwg;
}


WRegion *groupedpholder_do_attach(WGroupedPHolder *ph, int flags,
                                  WRegionAttachData *data)
{
    WRegionAttachData data2;

    if(ph->cont==NULL)
        return FALSE;

    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=grouped_handler;
    data2.u.n.param=data;

    return pholder_do_attach(ph->cont, flags, &data2);
}


/*}}}*/


/*{{{ Other dynfuns */


bool groupedpholder_do_goto(WGroupedPHolder *ph)
{
    return (ph->cont!=NULL
            ? pholder_goto(ph->cont)
            : FALSE);
}


WRegion *groupedpholder_do_target(WGroupedPHolder *ph)
{
    return (ph->cont!=NULL
            ? pholder_target(ph->cont)
            : NULL);
}


WPHolder *groupedpholder_do_root(WGroupedPHolder *ph)
{
    WPHolder *root;

    if(ph->cont==NULL)
        return NULL;

    root=pholder_root(ph->cont);

    return (root!=ph->cont
            ? root
            : &ph->ph);
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

    {(DynFun*)pholder_do_root,
     (DynFun*)groupedpholder_do_root},

    END_DYNFUNTAB
};

IMPLCLASS(WGroupedPHolder, WPHolder, groupedpholder_deinit,
          groupedpholder_dynfuntab);


/*}}}*/

