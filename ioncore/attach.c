/*
 * ion/ioncore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <libtu/objp.h>
#include "clientwin.h"
#include "saveload.h"
#include "manage.h"
#include "extlconv.h"
#include "names.h"
#include "region-iter.h"



/*{{{ New */


static WRegion *add_fn_new(WWindow *par, const WRectangle *geom,
                           WRegionSimpleCreateFn *fn)
{
    return fn(par, geom);
}


WRegion *region__attach_new(WRegion *mgr, WRegionSimpleCreateFn *cfn,
                            WRegionDoAttachFn *fn, void *param)
{
    return fn(mgr, (WRegionAttachHandler*)add_fn_new, (void*)cfn, param);
}


/*}}}*/


/*{{{ Load */


static WRegion *add_fn_load(WWindow *par, const WRectangle *geom, 
                            ExtlTab *tab)
{
    return create_region_load(par, geom, *tab);
}


WRegion *region__attach_load(WRegion *mgr, ExtlTab tab,
                             WRegionDoAttachFn *fn, void *param)
{
    return fn(mgr, (WRegionAttachHandler*)add_fn_load, (void*)&tab, param);
}


/*}}}*/


/*{{{ Reparent */


static WRegion *add_fn_reparent(WWindow *par, const WRectangle *geom, 
                                WRegion *reg)
{
    if(!region_reparent(reg, par, geom)){
        warn("Unable to reparent");
        return NULL;
    }
    region_detach_manager(reg);
    return reg;
}


WRegion *region__attach_reparent(WRegion *mgr, WRegion *reg, 
                                 WRegionDoAttachFn *fn, void *param)
{
    WRegion *reg2;
    
    if(REGION_MANAGER(reg)==mgr)
        return reg;
    
    /* Check that reg is not a parent or manager of mgr */
    reg2=mgr;
    for(reg2=mgr; reg2!=NULL; reg2=REGION_MANAGER(reg2)){
        if(reg2==reg){
            warn("Trying to make a %s manage a %s above it in management "
                 "hierarchy", OBJ_TYPESTR(mgr), OBJ_TYPESTR(reg));
            return NULL;
        }
    }
    
    for(reg2=region_parent(mgr); reg2!=NULL; reg2=region_parent(reg2)){
        if(reg2==reg){
            warn("Trying to make a %s manage its ancestor (a %s)",
                 OBJ_TYPESTR(mgr), OBJ_TYPESTR(reg));
            return NULL;
        }
    }
    
    return fn(mgr, (WRegionAttachHandler*)add_fn_reparent,
              (void*)reg, param);
}


/*}}}*/


