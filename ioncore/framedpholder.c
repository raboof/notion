/*
 * ion/ioncore/framedpholder.c
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
#include <libtu/minmax.h>

#include "frame.h"
#include "framedpholder.h"
#include "sizehint.h"


/*{{{ Init/deinit */


bool framedpholder_init(WFramedPHolder *ph, WPHolder *cont,
                        const WFramedParam *param)
{
    assert(cont!=NULL);
    
    pholder_init(&(ph->ph));

    ph->cont=cont;
    ph->param=*param;
    
    return TRUE;
}
 

WFramedPHolder *create_framedpholder(WPHolder *cont,
                                     const WFramedParam *param)
{
    CREATEOBJ_IMPL(WFramedPHolder, framedpholder, (p, cont, param));
}


void framedpholder_deinit(WFramedPHolder *ph)
{
    if(ph->cont!=NULL){
        destroy_obj((Obj*)ph->cont);
        ph->cont=NULL;
    }
    
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Attach */


typedef struct{
    WRegionAttachData *data;
    WFramedParam *param;
} AP;


WRegion *framed_handler(WWindow *par, 
                        const WFitParams *fp, 
                        void *ap_)
{
    AP *ap=(AP*)ap_;
    WMPlexAttachParams mp=MPLEXATTACHPARAMS_INIT;
    WFramedParam *param=ap->param;
    WRectangle rqg, mg;
    WFrame *frame;
    WRegion *reg;
    
    /*if(param->mkframe!=NULL)
        frame=(WFrame*)(param->mkframe)(par, fp);
    else*/
    frame=create_frame(par, fp, param->mode);
    
    if(frame==NULL)
        return NULL;
    
    if(fp->mode&(REGION_FIT_BOUNDS|REGION_FIT_WHATEVER))
        mp.flags|=MPLEX_ATTACH_WHATEVER;

    reg=mplex_do_attach(&frame->mplex, &mp, ap->data);
    
    if(reg==NULL){
        destroy_obj((Obj*)frame);
        return NULL;
    }

    if(!(fp->mode&(REGION_FIT_BOUNDS|REGION_FIT_WHATEVER)))
        return (WRegion*)frame;

    mplex_managed_geom((WMPlex*)frame, &mg);

    /* Adjust geometry */
    if(!param->inner_geom_gravity_set){
        rqg.x=REGION_GEOM(frame).x;
        rqg.y=REGION_GEOM(frame).y;
        rqg.w=maxof(1, REGION_GEOM(reg).w+(REGION_GEOM(frame).w-mg.w));
        rqg.h=maxof(1, REGION_GEOM(reg).h+(REGION_GEOM(frame).h-mg.h));
    }else{
        int bl=mg.x;
        int br=REGION_GEOM(frame).w-(mg.x+mg.w);
        int bt=mg.y;
        int bb=REGION_GEOM(frame).h-(mg.y+mg.h);
        
        rqg.x=(fp->g.x+param->inner_geom.x+
               xgravity_deltax(param->gravity, bl, br));
        rqg.y=(fp->g.y+param->inner_geom.y+
               xgravity_deltay(param->gravity, bt, bb));
        rqg.w=maxof(1, param->inner_geom.w+(REGION_GEOM(frame).w-mg.w));
        rqg.h=maxof(1, param->inner_geom.h+(REGION_GEOM(frame).h-mg.h));
    }

    if(!(fp->mode&REGION_FIT_WHATEVER))
        rectangle_constrain(&rqg, &fp->g);
    
    region_fit((WRegion*)frame, &rqg, REGION_FIT_EXACT);
    
    return (WRegion*)frame;
}


WRegion *region_attach_framed(WRegion *reg, WFramedParam *param,
                              WRegionAttachFn *fn, void *fn_param,
                              WRegionAttachData *data)
{
    WRegionAttachData data2;
    AP ap;

    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=framed_handler;
    data2.u.n.param=&ap;
    
    ap.data=data;
    ap.param=param;
    
    return fn(reg, fn_param, &data2);
}


WRegion *framedpholder_do_attach(WFramedPHolder *ph, int flags,
                                 WRegionAttachData *data)
{
    WRegionAttachData data2;
    AP ap;
    
    if(ph->cont==NULL)
        return FALSE;

    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=framed_handler;
    data2.u.n.param=&ap;
    
    ap.data=data;
    ap.param=&ph->param;
        
    return pholder_attach_(ph->cont, flags, &data2);
}


/*}}}*/


/*{{{ Other dynfuns */


bool framedpholder_do_goto(WFramedPHolder *ph)
{
    if(ph->cont!=NULL)
        return pholder_goto(ph->cont);
    
    return FALSE;
}


WRegion *framedpholder_do_target(WFramedPHolder *ph)
{
    if(ph->cont!=NULL)
        return pholder_target(ph->cont);
    
    return NULL;
}


/*}}}*/


/*{{{ Class information */


static DynFunTab framedpholder_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)framedpholder_do_attach},

    {(DynFun*)pholder_do_goto, 
     (DynFun*)framedpholder_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)framedpholder_do_target},
    
    END_DYNFUNTAB
};

IMPLCLASS(WFramedPHolder, WPHolder, framedpholder_deinit, 
          framedpholder_dynfuntab);


/*}}}*/

