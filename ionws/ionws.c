/*
 * ion/ionws/ionws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <libtu/objp.h>
#include <ioncore/region.h>
#include <ioncore/manage.h>
#include <ioncore/screen.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/resize.h>
#include <ioncore/extl.h>
#include <ioncore/region-iter.h>
#include <libtu/minmax.h>
#include "placement.h"
#include "ionws.h"
#include "ionframe.h"
#include "split.h"


/*{{{ region dynfun implementations */


static bool ionws_fitrep(WIonWS *ws, WWindow *par, const WFitParams *fp)
{
    WRegion *sub, *next;
    bool rs;
    
    if(par!=NULL){
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
    
        region_detach_parent((WRegion*)ws);
        region_attach_parent((WRegion*)ws, (WRegion*)par);
    
        FOR_ALL_MANAGED_ON_LIST_W_NEXT(ws->managed_list, sub, next){
            WFitParams subfp;
            subfp.g=REGION_GEOM(sub);
            subfp.mode=REGION_FIT_EXACT;
            if(!region_fitrep(sub, par, &subfp)){
                warn("Problem: can't reparent a %s managed by a WIonWS"
                     "being reparented. Detaching from this object.",
                     OBJ_TYPESTR(sub));
                region_detach_manager(sub);
            }
        }
    }
    
    REGION_GEOM(ws)=fp->g;
    
    if(ws->split_tree==NULL)
        return TRUE;
    
    split_tree_resize((Obj*)ws->split_tree, HORIZONTAL, ANY, fp->g.x, fp->g.w);
    split_tree_resize((Obj*)ws->split_tree, VERTICAL, ANY, fp->g.y,  fp->g.h);
    
    return TRUE;
}


static void ionws_map(WIonWS *ws)
{
    WRegion *reg;

    REGION_MARK_MAPPED(ws);
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        region_map(reg);
    }
}


static void ionws_unmap(WIonWS *ws)
{
    WRegion *reg;
    
    REGION_MARK_UNMAPPED(ws);
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        region_unmap(reg);
    }
}


static void ionws_do_set_focus(WIonWS *ws, bool warp)
{
    WRegion *sub=ionws_current(ws);
    
    if(sub==NULL){
        warn("Trying to focus an empty ionws.");
        return;
    }

    region_do_set_focus(sub, warp);
}


static bool ionws_managed_display(WIonWS *ws, WRegion *reg)
{
    return TRUE;
}


/*}}}*/


/*{{{ Create/destroy */


static WIonFrame *create_initial_frame(WIonWS *ws, WWindow *parent,
                                       const WFitParams *fp)
{
    WIonFrame *frame;
    
    frame=create_ionframe(parent, fp);

    if(frame==NULL)
        return NULL;
    
    ws->split_tree=(Obj*)frame;
    ionws_add_managed(ws, (WRegion*)frame);

    return frame;
}


static bool ionws_init(WIonWS *ws, WWindow *parent, const WFitParams *fp,
                       bool ci)
{
    ws->managed_splits=extl_create_table();
    
    if(ws->managed_splits==extl_table_none())
        return FALSE;
    
    ws->split_tree=NULL;

    genws_init(&(ws->genws), parent, fp);
    
    if(ci){
        if(create_initial_frame(ws, parent, fp)==NULL){
            genws_deinit(&(ws->genws));
            extl_unref_table(ws->managed_splits);
            return FALSE;
        }
    }
    
    return TRUE;
}


WIonWS *create_ionws(WWindow *parent, const WFitParams *fp, bool ci)
{
    CREATEOBJ_IMPL(WIonWS, ionws, (p, parent, fp, ci));
}


WIonWS *create_ionws_simple(WWindow *parent, const WFitParams *fp)
{
    return create_ionws(parent, fp, TRUE);
}


void ionws_deinit(WIonWS *ws)
{
    WRegion *reg;
    
    while(ws->managed_list!=NULL)
        ionws_managed_remove(ws, ws->managed_list);

    genws_deinit(&(ws->genws));
    
    extl_unref_table(ws->managed_splits);
}


static bool ionws_managed_may_destroy(WIonWS *ws, WRegion *reg)
{
    if(ws->split_tree==(Obj*)reg)
        return region_may_destroy((WRegion*)ws);
    else
        return TRUE;
}


bool ionws_rescue_clientwins(WIonWS *ws)
{
    return region_rescue_managed_clientwins((WRegion*)ws, ws->managed_list);
}


/*}}}*/


/*{{{ Save */


static ExtlTab get_obj_config(Obj *obj)
{
    WWsSplit *split;
    int tls, brs;
    ExtlTab tab, stab;
    
    if(obj==NULL)
        return extl_table_none();
    
    if(OBJ_IS(obj, WRegion)){
        if(region_supports_save((WRegion*)obj))
            return region_get_configuration((WRegion*)obj);
        return extl_table_none();
    }
    
    if(!OBJ_IS(obj, WWsSplit))
        return extl_table_none();
    
    split=(WWsSplit*)obj;
    
    tab=extl_create_table();
    
    tls=split_tree_size(split->tl, split->dir);
    brs=split_tree_size(split->br, split->dir);
    
    extl_table_sets_s(tab, "split_dir",(split->dir==VERTICAL
                                        ? "vertical" : "horizontal"));
    
    extl_table_sets_i(tab, "split_tls", tls);
    extl_table_sets_i(tab, "split_brs", brs);
    
    stab=get_obj_config(split->tl);
    if(stab==extl_table_none()){
        warn("Could not get configuration for split TL (a %s).", 
             OBJ_TYPESTR(obj));
    }else{
        extl_table_sets_t(tab, "tl", stab);
        extl_unref_table(stab);
    }
    
    stab=get_obj_config(split->br);
    if(stab==extl_table_none()){
        warn("Could not get configuration for split BR (a %s).", 
             OBJ_TYPESTR(obj));
    }else{
        extl_table_sets_t(tab, "br", stab);
        extl_unref_table(stab);
    }

    return tab;
}


static ExtlTab ionws_get_configuration(WIonWS *ws)
{
    ExtlTab tab, split_tree;
    
    tab=region_get_base_configuration((WRegion*)ws);
    split_tree=get_obj_config(ws->split_tree);
    
    if(split_tree==extl_table_none()){
        warn("Could not get split tree for a WIonWS.");
    }else{
        extl_table_sets_t(tab, "split_tree", split_tree);
        extl_unref_table(split_tree);
    }
    
    return tab;
}


/*}}}*/


/*{{{ Load */


extern void set_split_of(Obj *obj, WWsSplit *split);
static Obj *load_obj(WIonWS *ws, WWindow *par, const WRectangle *geom, 
                      ExtlTab tab);

#define MINS 8

static Obj *load_split(WIonWS *ws, WWindow *par, const WRectangle *geom,
                        ExtlTab tab)
{
    WWsSplit *split;
    char *dir_str;
    int dir, brs, tls;
    ExtlTab subtab;
    Obj *tl=NULL, *br=NULL;
    WRectangle geom2;

    if(!extl_table_gets_i(tab, "split_tls", &tls))
        return FALSE;
    if(!extl_table_gets_i(tab, "split_brs", &brs))
        return FALSE;
    if(!extl_table_gets_s(tab, "split_dir", &dir_str))
        return FALSE;
    if(strcmp(dir_str, "vertical")==0){
        dir=VERTICAL;
    }else if(strcmp(dir_str, "horizontal")==0){
        dir=HORIZONTAL;
    }else{
        free(dir_str);
        return NULL;
    }
    free(dir_str);

    split=create_split(dir, NULL, NULL, geom);
    if(split==NULL){
        warn("Unable to create a split.\n");
        return NULL;
    }

    tls=maxof(tls, MINS);
    brs=maxof(brs, MINS);
        
    geom2=*geom;
    if(dir==HORIZONTAL){
        tls=maxof(0, geom->w)*tls/(tls+brs);
        geom2.w=tls;
    }else{
        tls=maxof(0, geom->h)*tls/(tls+brs);
        geom2.h=tls;
    }
    
    if(extl_table_gets_t(tab, "tl", &subtab)){
        tl=load_obj(ws, par, &geom2, subtab);
        extl_unref_table(subtab);
    }

    geom2=*geom;
    if(tl!=NULL){
        if(dir==HORIZONTAL){
            geom2.w-=tls;
            geom2.x+=tls;
        }else{
            geom2.h-=tls;
            geom2.y+=tls;
        }
    }
            
    if(extl_table_gets_t(tab, "br", &subtab)){
        br=load_obj(ws, par, &geom2, subtab);
        extl_unref_table(subtab);
    }
    
    if(tl==NULL || br==NULL){
        free(split);
        return (tl==NULL ? br : tl);
    }
    
    set_split_of(tl, split);
    set_split_of(br, split);

    /*split->tmpsize=tls;*/
    split->tl=tl;
    split->br=br;
    
    return (Obj*)split;
}


static Obj *load_obj(WIonWS *ws, WWindow *par, const WRectangle *geom,
                      ExtlTab tab)
{
    char *typestr;
    WRegion *reg;
    WFitParams fp;
    
    if(extl_table_gets_s(tab, "type", &typestr)){
        free(typestr);
        fp.g=*geom;
        fp.mode=REGION_FIT_EXACT;
        reg=create_region_load(par, &fp, tab);
        if(reg!=NULL)
            ionws_add_managed(ws, reg);
        return (Obj*)reg;
    }
    
    return load_split(ws, par, geom, tab);
}


WRegion *ionws_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WIonWS *ws;
    ExtlTab treetab;
    bool ci=TRUE;

    if(extl_table_gets_t(tab, "split_tree", &treetab))
        ci=FALSE;
    
    ws=create_ionws(par, fp, ci);
    
    if(ws==NULL){
        if(!ci)
            extl_unref_table(treetab);
        return NULL;
    }

    if(!ci){
        ws->split_tree=load_obj(ws, par, &REGION_GEOM(ws), treetab);
        extl_unref_table(treetab);
    }
    
    if(ws->split_tree==NULL){
        warn("Workspace empty");
        destroy_obj((Obj*)ws);
        return NULL;
    }
    
    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionws_dynfuntab[]={
    {region_map, ionws_map},
    {region_unmap, ionws_unmap},
    {region_do_set_focus, ionws_do_set_focus},
    
    {(DynFun*)region_fitrep,
     (DynFun*)ionws_fitrep},
    
    {region_managed_rqgeom, ionws_managed_rqgeom},
    {region_managed_activated, ionws_managed_activated},
    {region_managed_remove, ionws_managed_remove},
    
    {(DynFun*)region_managed_display,
     (DynFun*)ionws_managed_display},
    
    {(DynFun*)region_manage_clientwin, 
     (DynFun*)ionws_manage_clientwin},
    {(DynFun*)region_manage_rescue,
     (DynFun*)ionws_manage_rescue},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)ionws_rescue_clientwins},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)ionws_get_configuration},

    {(DynFun*)region_managed_may_destroy,
     (DynFun*)ionws_managed_may_destroy},

    {(DynFun*)region_current,
     (DynFun*)ionws_current},

    END_DYNFUNTAB
};


IMPLCLASS(WIonWS, WGenWS, ionws_deinit, ionws_dynfuntab);

    
/*}}}*/

