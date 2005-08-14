/*
 * ion/ioncore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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


/*{{{ New */


WRegion *region__attach_new_doit(WWindow *par, const WFitParams *fp,
                                 WRegionSimpleCreateFn *fn)
{
    return fn(par, fp);
}


WRegion *region__attach_new(WRegion *mgr, WRegionSimpleCreateFn *cfn,
                            WRegionDoAttachFn *fn, void *param)
{
    return fn(mgr, (WRegionAttachHandler*)region__attach_new_doit, 
              (void*)cfn, param);
}


/*}}}*/


/*{{{ Load */


WRegion *region__attach_load_doit(WWindow *par, const WFitParams *fp, 
                                  ExtlTab *tab)
{
    return create_region_load(par, fp, *tab);
}


WRegion *region__attach_load(WRegion *mgr, ExtlTab tab,
                             WRegionDoAttachFn *fn, void *param)
{
    WRegion *reg=NULL;
    
    if(extl_table_gets_o(tab, "reg", (Obj**)&reg))
        return region__attach_reparent(mgr, reg, fn, param);

    return fn(mgr, (WRegionAttachHandler*)region__attach_load_doit, 
              (void*)&tab, param);
}


/*}}}*/


/*{{{ Reparent */


WRegion *region__attach_reparent_doit(WWindow *par, const WFitParams *fp, 
                                      WRegion *reg)
{
    WFitParams fp2;
    
    if(fp->mode&REGION_FIT_WHATEVER){
        /* fp->g is not final; substitute size with current to avoid
         * useless resizing. 
         */
        fp2.mode=fp->mode;
        fp2.g.x=fp->g.x;
        fp2.g.y=fp->g.y;
        fp2.g.w=REGION_GEOM(reg).w;
        fp2.g.h=REGION_GEOM(reg).h;
    }
    
    if(!region_fitrep(reg, par, fp)){
        warn(TR("Unable to reparent."));
        return NULL;
    }
    region_detach_manager(reg);
    return reg;
}


bool region__attach_reparent_check(WRegion *mgr, WRegion *reg)
{
    WRegion *reg2;
    
    if(REGION_MANAGER(reg)==mgr){
        warn(TR("Same manager."));
        return FALSE;
    }
    
    /* Check that reg is not a parent or manager of mgr */
    reg2=mgr;
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


WRegion *region__attach_reparent(WRegion *mgr, WRegion *reg, 
                                 WRegionDoAttachFn *fn, void *param)
{
    if(!region__attach_reparent_check(mgr, reg))
        return NULL;
    
    return fn(mgr, (WRegionAttachHandler*)region__attach_reparent_doit,
              (void*)reg, param);
}


/*}}}*/


