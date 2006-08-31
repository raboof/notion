/*
 * ion/ioncore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "attach.h"
#include "clientwin.h"
#include "saveload.h"
#include "manage.h"
#include "extlconv.h"
#include "names.h"


/*{{{ Helper */


static WRegion *doit_new(WRegion *mgr,
                         WWindow *par, const WFitParams *fp,
                         WRegionDoAttachFn *cont, void *cont_param,
                         WRegionCreateFn *fn, void *fn_param)
{
    WRegion *reg=fn(par, fp, fn_param);
    
    if(reg==NULL)
        return NULL;
    
    if(!cont(mgr, reg, cont_param)){
        destroy_obj((Obj*)reg);
        return NULL;
    }
    
    return reg;
}


static WRegion *doit_reparent(WRegion *mgr,
                              WWindow *par, const WFitParams *fp,
                              WRegionDoAttachFn *cont, void *cont_param,
                              WRegion *reg)
{
    WFitParams fp2;

    if(!region_attach_reparent_check(mgr, reg))
        return NULL;
    
    if(fp->mode&REGION_FIT_WHATEVER){
        /* fp->g is not final; substitute size with current to avoid
         * useless resizing. 
         */
        fp2.mode=fp->mode;
        fp2.g.x=fp->g.x;
        fp2.g.y=fp->g.y;
        fp2.g.w=REGION_GEOM(reg).w;
        fp2.g.h=REGION_GEOM(reg).h;
        fp=&fp2;
    }
    
    if(!region_fitrep(reg, par, fp)){
        warn(TR("Unable to reparent."));
        return NULL;
    }
    
    region_detach_manager(reg);
    
    if(!cont(mgr, reg, cont_param)){
        #warning "TODO: What?"
        return NULL;
    }
       
    return reg;
}


static WRegion *wrap_load(WWindow *par, const WFitParams *fp, 
                          ExtlTab *tab)
{
    return create_region_load(par, fp, *tab);
}


static WRegion *doit_load(WRegion *mgr,
                          WWindow *par, const WFitParams *fp,
                          WRegionDoAttachFn *cont, void *cont_param,
                          ExtlTab tab)
{
    WRegion *reg;
    
    if(extl_table_gets_o(tab, "reg", (Obj**)&reg)){
        if(!OBJ_IS(reg, WRegion))
            return FALSE;
        return doit_reparent(mgr, par, fp, cont, cont_param, reg);
    }
    
    return doit_new(mgr, par, fp, cont, cont_param,
                    (WRegionCreateFn*)wrap_load, &tab);
}

WRegion *region_attach_helper(WRegion *mgr,
                              WWindow *par, WFitParams *fp,
                              WRegionDoAttachFn *fn, void *fn_param,
                              const WRegionAttachData *data)
{
    if(data->type==REGION_ATTACH_NEW){
        return doit_new(mgr, par, fp, fn, fn_param, 
                        data->u.n.fn, data->u.n.param);
    }else if(data->type==REGION_ATTACH_LOAD){
        return doit_load(mgr, par, fp, fn, fn_param, data->u.tab);
    }else if(data->type==REGION_ATTACH_REPARENT){
        return doit_reparent(mgr, par, fp, fn, fn_param, data->u.reg);
    }else{
        return NULL;
    }
}


/*}}}*/


/*{{{ Reparent check */


bool region_attach_reparent_check(WRegion *mgr, WRegion *reg)
{
    WRegion *reg2;
    
    /*if(REGION_MANAGER(reg)==mgr){
        warn(TR("Same manager."));
        return FALSE;
    }*/
    
    /* Check that reg is not a parent or manager of mgr */
    for(reg2=mgr; reg2!=NULL; reg2=REGION_MANAGER(reg2)){
        if(reg2==reg)
            goto err;
    }
    
    for(reg2=REGION_PARENT_REG(mgr); reg2!=NULL; reg2=REGION_PARENT_REG(reg2)){
        if(reg2==reg)
            goto err;
    }

    return TRUE;
    
err:
    warn(TR("Attempt to make region %s manage its ancestor %s."),
         region_name(mgr), region_name(reg));
    return FALSE;
}


/*}}}*/


