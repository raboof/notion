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
#include <ioncore/defer.h>
#include <ioncore/frame.h>
#include <ioncore/region-iter.h>
#include <mod_ionws/ionws.h>
#include <mod_ionws/split.h>
#include "autows.h"
#include "placement.h"
#include "main.h"


/*{{{ Create/destroy */


void autows_managed_add(WAutoWS *ws, WRegion *reg)
{
    region_add_bindmap_owned(reg, mod_autows_autows_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_add_bindmap(reg, mod_autows_frame_bindmap);
    
    ionws_managed_add_default(&(ws->ionws), reg);
}


static WRegion *create_frame_autows(WWindow *parent, const WFitParams *fp)
{
    return (WRegion*)create_frame(parent, fp, "frame-autoframe");
}


static bool autows_create_initial_unused(WAutoWS *ws)
{
    ws->ionws.split_tree=(WSplit*)create_splitunused(&REGION_GEOM(ws));
    return (ws->ionws.split_tree!=NULL);
}


bool autows_init(WAutoWS *ws, WWindow *parent, const WFitParams *fp, bool cu)
{
    if(!ionws_init(&(ws->ionws), parent, fp, 
                   create_frame_autows, FALSE))
        return FALSE;
    
    assert(ws->ionws.split_tree==NULL);
    
    if(cu){
        if(!autows_create_initial_unused(ws)){
            autows_deinit(ws);
            return FALSE;
        }
    }
    
    return TRUE;
}


WAutoWS *create_autows(WWindow *parent, const WFitParams *fp, bool cu)
{
    CREATEOBJ_IMPL(WAutoWS, autows, (p, parent, fp, cu));
}


WAutoWS *create_autows_simple(WWindow *parent, const WFitParams *fp)
{
    return create_autows(parent, fp, TRUE);
}


void autows_deinit(WAutoWS *ws)
{
    ionws_deinit(&(ws->ionws));
}


static WSplitRegion *get_node_check(WAutoWS *ws, WRegion *reg)
{
    WSplitRegion *node;

    if(reg==NULL)
        return NULL;
    
    node=splittree_node_of(reg);
    
    if(node==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return NULL;
    
    return node;
}


static void autows_do_managed_remove(WAutoWS *ws, WRegion *reg)
{
    ionws_do_managed_remove(&(ws->ionws), reg);
    region_remove_bindmap_owned(reg, mod_autows_autows_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_remove_bindmap(reg, mod_autows_frame_bindmap);
}


static bool plainregionfilter(WSplit *node)
{
    return (strcmp(OBJ_TYPESTR(node), "WSplitRegion")==0);
}



void autows_managed_remove(WAutoWS *ws, WRegion *reg)
{
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    bool act=REGION_IS_ACTIVE(reg);
    bool mcf=region_may_control_focus((WRegion*)ws);
    WSplitRegion *other=NULL, *node=get_node_check(ws, reg);
    
    autows_do_managed_remove(ws, reg);

    if(node==NULL)
        return;

    other=(WSplitRegion*)split_closest_leaf((WSplit*)node, 
                                            plainregionfilter);

    if(ws->ionws.split_tree!=NULL){
        if(node==(WSplitRegion*)(ws->ionws.stdispnode))
            ws->ionws.stdispnode=NULL;
        splittree_remove(&(ws->ionws.split_tree), (WSplit*)node, !ds);
    }
    
    if(!ds){
        if(other==NULL){
#warning "Dock reinitissä"            
#if 0
            if(ws->ionws.split_tree!=NULL){
                ioncore_defer_destroy((Obj*)(ws->ionws.split_tree));
                ws->ionws.split_tree=NULL;
            }

            autows_create_initial_unused(ws);
            
            if(ws->ionws.split_tree==NULL){
                warn("Unable to re-initialise workspace. Destroying.");
                ioncore_defer_destroy((Obj*)ws);
            }
#endif

            if(act && mcf){
                /* We don't want to give the stdisp focus, even if one exists. 
                 * Or do we?
                 */
                genws_fallback_focus((WGenWS*)ws, FALSE);
            }
        }else if(act && mcf){
            region_set_focus(other->reg);
        }
    }
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

    ws=create_autows(par, fp, FALSE);
    
    if(ws==NULL)
        return NULL;

    if(extl_table_gets_t(tab, "split_tree", &treetab)){
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws), &REGION_GEOM(ws), 
                                             treetab);
        extl_unref_table(treetab);
    }
    
    if(ws->ionws.split_tree==NULL){
        if(!autows_create_initial_unused(ws)){
            destroy_obj((Obj*)ws);
            return NULL;
        }
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

