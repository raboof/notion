/*
 * ion/mod_autows/autows.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/focus.h>
#include <ioncore/manage.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/extl.h>
#include <ioncore/regbind.h>
#include <ioncore/extlconv.h>
#include <mod_ionws/ionws.h>
#include <mod_ionws/split.h>
#include "autows.h"
#include "autoframe.h"
#include "placement.h"
#include "main.h"


/*{{{ Create/destroy */


void autows_managed_add(WAutoWS *ws, WRegion *reg)
{
    region_add_bindmap_owned(reg, mod_autows_autows_bindmap, (WRegion*)ws);
    ionws_managed_add_default(&(ws->ionws), reg);
}


bool autows_init(WAutoWS *ws, WWindow *parent, const WFitParams *fp)
{
    return ionws_init(&(ws->ionws), parent, fp, FALSE,
                      (WRegionSimpleCreateFn*)create_autoframe);
}


WAutoWS *create_autows(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WAutoWS, autows, (p, parent, fp));
}


void autows_deinit(WAutoWS *ws)
{
    ionws_deinit(&(ws->ionws));
}


void autows_managed_remove(WAutoWS *ws, WRegion *reg)
{
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    WRegion *other=ionws_do_managed_remove(&(ws->ionws), reg, !ds);
    
    region_remove_bindmap_owned(reg, mod_autows_autows_bindmap, (WRegion*)ws);
    
    if(other!=NULL && !ds && region_may_control_focus((WRegion*)ws))
        region_set_focus(other);
}


/*}}}*/


/*{{{ Misc. */


bool autows_managed_may_destroy(WAutoWS *ws, WRegion *reg)
{
    return TRUE;
}


/*}}}*/


/*{{{ Save */


ExtlTab autows_get_configuration(WAutoWS *ws)
{
    return ionws_get_configuration(&(ws->ionws));
}


/*}}}*/


/*{{{ Load */


WRegion *autows_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WAutoWS *ws;
    ExtlTab treetab;

    ws=create_autows(par, fp);
    
    if(ws==NULL)
        return NULL;

    if(extl_table_gets_t(tab, "split_tree", &treetab)){
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws), par, 
                                             &REGION_GEOM(ws), treetab);
        extl_unref_table(treetab);
    }
    
    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab autows_dynfuntab[]={
    {region_managed_remove, 
     autows_managed_remove},

    {(DynFun*)region_manage_clientwin, 
     (DynFun*)autows_manage_clientwin},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)autows_get_configuration},

    {(DynFun*)region_managed_may_destroy,
     (DynFun*)autows_managed_may_destroy},

    {ionws_managed_add,
     autows_managed_add},

    END_DYNFUNTAB
};


IMPLCLASS(WAutoWS, WIonWS, autows_deinit, autows_dynfuntab);

    
/*}}}*/

