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

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/manage.h>
#include <ioncore/screen.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/resize.h>
#include <ioncore/extl.h>
#include <ioncore/region-iter.h>
#include <ioncore/regbind.h>
#include <ioncore/extlconv.h>
#include <ioncore/defer.h>
#include "placement.h"
#include "ionws.h"
#include "ionframe.h"
#include "split.h"
#include "main.h"


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


static void ionws_managed_add(WIonWS *ws, WRegion *reg)
{
    region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
    
    region_add_bindmap_owned(reg, mod_ionws_ionws_bindmap, (WRegion*)ws);
    
    if(REGION_IS_MAPPED(ws))
        region_map(reg);
}


static WIonFrame *create_initial_frame(WIonWS *ws, WWindow *parent,
                                       const WFitParams *fp)
{
    WIonFrame *frame;
    
    frame=create_ionframe(parent, fp);

    if(frame==NULL)
        return NULL;
    
    ws->split_tree=(Obj*)frame;
    ionws_managed_add(ws, (WRegion*)frame);

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


void ionws_managed_remove(WIonWS *ws, WRegion *reg)
{
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    WRegion *other;
    
    region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));
    region_remove_bindmap_owned(reg, mod_ionws_ionws_bindmap, (WRegion*)ws);

    other=split_tree_remove(&(ws->split_tree), reg, !ds);
    
    if(!ds){
        if(other){
            if(region_may_control_focus((WRegion*)ws))
                region_set_focus(other);
        }else{
            ioncore_defer_destroy((Obj*)ws);
        }
    }
}


bool ionws_manage_rescue(WIonWS *ws, WClientWin *cwin, WRegion *from)
{
    WWsSplit *split;
    WMPlex *nmgr;
    
    if(REGION_MANAGER(from)!=(WRegion*)ws)
        return FALSE;

    nmgr=split_tree_find_mplex(from);

    if(nmgr!=NULL)
        return (NULL!=mplex_attach_simple(nmgr, (WRegion*)cwin, 0));
    
    return FALSE;
}


/*}}}*/


/*{{{ Resize */


void ionws_managed_rqgeom(WIonWS *ws, WRegion *mgd, 
                          int flags, const WRectangle *geom,
                          WRectangle *geomret)
{
    if(REGION_MANAGER(mgd)!=(WRegion*)ws)
        return;
    
    split_tree_rqgeom(ws->split_tree, (Obj*)mgd, flags, geom, geomret);
}


/*EXTL_DOC
 * Attempt to resize and/or move the split tree starting at \var{node}
 * (\type{WWsSplit} or \type{WRegion}). Behaviour and the \var{g} 
 * parameter are as for \fnref{WRegion.rqgeom} operating on
 * \var{node} (if it were a \type{WRegion}).
 */
EXTL_EXPORT_MEMBER
ExtlTab ionws_resize_tree(WIonWS *ws, Obj *node, ExtlTab g)
{
    WRectangle geom, ogeom;
    int flags=REGION_RQGEOM_WEAK_ALL;
    
    if(node!=NULL && OBJ_IS(node, WRegion)){
        geom=REGION_GEOM((WRegion*)node);
    }else if(node!=NULL && OBJ_IS(node, WWsSplit)){
        geom=((WWsSplit*)node)->geom;
    }else{
        warn("Invalid node.");
        return extl_table_none();
    }
    
    ogeom=geom;

    if(extl_table_gets_i(g, "x", &(geom.x)))
        flags&=~REGION_RQGEOM_WEAK_X;
    if(extl_table_gets_i(g, "y", &(geom.y)))
        flags&=~REGION_RQGEOM_WEAK_Y;
    if(extl_table_gets_i(g, "w", &(geom.w)))
        flags&=~REGION_RQGEOM_WEAK_W;
    if(extl_table_gets_i(g, "h", &(geom.h)))
        flags&=~REGION_RQGEOM_WEAK_H;
    
    geom.w=maxof(1, geom.w);
    geom.h=maxof(1, geom.h);

    split_tree_rqgeom(ws->split_tree, node, flags, &geom, &ogeom);
    
    return extl_table_from_rectangle(&ogeom);
}


/*}}}*/


/*{{{ Split/unsplit */


static bool get_split_dir_primn(const char *str, int *dir, int *primn)
{
    if(str==NULL)
        return FALSE;
    
    if(!strcmp(str, "left")){
        *primn=TOP_OR_LEFT;
        *dir=HORIZONTAL;
    }else if(!strcmp(str, "right")){
        *primn=BOTTOM_OR_RIGHT;
        *dir=HORIZONTAL;
    }else if(!strcmp(str, "top") || 
             !strcmp(str, "above") || 
             !strcmp(str, "up")){
        *primn=TOP_OR_LEFT;
        *dir=VERTICAL;
    }else if(!strcmp(str, "bottom") || 
             !strcmp(str, "below") ||
             !strcmp(str, "down")){
        *primn=BOTTOM_OR_RIGHT;
        *dir=VERTICAL;
    }else{
        return FALSE;
    }
    
    return TRUE;
}


/*EXTL_DOC
 * Create new WIonFrame on \var{ws} above/below/left of/right of
 * all other objects depending on \var{dirstr}
 * (one of ''left'', ''right'', ''top'' or ''bottom'').
 */
EXTL_EXPORT_MEMBER
WIonFrame *ionws_split_top(WIonWS *ws, const char *dirstr)
{
    WRegion *reg=NULL;
    int dir, primn, mins;
    
    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    mins=16; /* totally arbitrary */
    
    if(ws->split_tree!=NULL){
        reg=split_tree_split(&(ws->split_tree), ws->split_tree, 
                             dir, primn, mins, ANY,
                             (WRegionSimpleCreateFn*)create_ionframe,
                             REGION_PARENT_CHK(ws, WWindow));
    }
    
    if(reg!=NULL){
        ionws_managed_add(ws, reg);
        region_warp(reg);
    }
    
    return (WIonFrame*)reg;
}


/*EXTL_DOC
 * Split \var{frame} creating a new WIonFrame to direction \var{dir}
 * (one of ''left'', ''right'', ''top'' or ''bottom'') of \var{frame}.
 * If \var{attach_current} is set, the region currently displayed in
 * \var{frame}, if any, is moved to thenew frame.
 */
EXTL_EXPORT_MEMBER
WIonFrame *ionws_split_at(WIonWS *ws, WIonFrame *frame, const char *dirstr, 
                          bool attach_current)
{
    WRegion *reg, *curr;
    int dir, primn, mins;
    
    if(frame==NULL){
        warn_obj("ionws_split_at", "nil frame");
        return NULL;
    }
    
    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        warn_obj("ionws_split_at", "Frame not managed by the workspace.");
        return NULL;
    }
    
    if(!get_split_dir_primn(dirstr, &dir, &primn)){
        warn_obj("ionws_split_at", "Unknown direction parameter to split_at");
        return NULL;
    }
    
    mins=(dir==VERTICAL
          ? region_min_h((WRegion*)frame)
          : region_min_w((WRegion*)frame));
    
    reg=split_tree_split(&(ws->split_tree), (Obj*)frame, 
                         dir, primn, mins, primn,
                         (WRegionSimpleCreateFn*)create_ionframe,
                         REGION_PARENT_CHK(ws, WWindow));
    
    if(reg==NULL){
        warn_obj("ionws_split_at", "Unable to split");
        return NULL;
    }

    assert(OBJ_IS(reg, WIonFrame));

    ionws_managed_add(ws, reg);
    
    curr=mplex_l1_current(&(frame->frame.mplex));
    
    if(attach_current && curr!=NULL)
        mplex_attach_simple((WMPlex*)reg, curr, MPLEX_ATTACH_SWITCHTO);
    
    if(region_may_control_focus((WRegion*)frame))
        region_goto(reg);

    return (WIonFrame*)reg;
}


/*EXTL_DOC
 * Try to relocate regions managed by \var{frame} to another frame
 * and, if possible, destroy the frame.
 */
EXTL_EXPORT_MEMBER
void ionws_unsplit_at(WIonWS *ws, WIonFrame *frame)
{
    if(frame==NULL){
        warn_obj("ionws_unsplit_at", "nil frame");
        return;
    }
    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        warn_obj("ionws_unsplit_at", "The frame is not managed by the workspace.");
        return;
    }
    
    if(!region_may_destroy((WRegion*)frame)){
        warn_obj("ionws_unsplit_at", "Frame may not be destroyed");
        return;
    }

    if(!region_rescue_clientwins((WRegion*)frame)){
        warn_obj("ionws_unsplit_at", "Failed to rescue managed objects.");
        return;
    }

    ioncore_defer_destroy((Obj*)frame);
}


/*}}}*/


/*{{{ Navigation etc. exports */


/*EXTL_DOC
 * Returns most recently active region on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_current(WIonWS *ws)
{
    return split_tree_current_tl(ws->split_tree, -1);
}


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT_MEMBER
ExtlTab ionws_managed_list(WIonWS *ws)
{
    return managed_list_to_table(ws->managed_list, NULL);
}


static WRegion *do_get_next_to(WIonWS *ws, WRegion *reg, int dir, int primn)
{
    if(reg==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return NULL;
    
    if(primn==TOP_OR_LEFT)
        return split_tree_to_tl(reg, dir);
    else
        return split_tree_to_br(reg, dir);
}


/*EXTL_DOC
 * Return the most previously active region next to \var{reg} in
 * direction \var{dirstr} (left/right/up/down). The region \var{reg}
 * must be managed by \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_next_to(WIonWS *ws, WRegion *reg, const char *dirstr)
{
    int dir=0, primn=0;
    
    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    return do_get_next_to(ws, reg, dir, primn);
}


static WRegion *do_get_farthest(WIonWS *ws, int dir, int primn)
{
    if(primn==TOP_OR_LEFT)
        return split_tree_current_tl(ws->split_tree, dir);
    else
        return split_tree_current_br(ws->split_tree, dir);
}


/*EXTL_DOC
 * Return the most previously active region on \var{ws} with no
 * other regions next to it in  direction \var{dirstr} 
 * (left/right/up/down). 
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_farthest(WIonWS *ws, const char *dirstr)
{
    int dir=0, primn=0;

    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    return do_get_farthest(ws, dir, primn);
}


static WRegion *do_goto_dir(WIonWS *ws, int dir, int primn)
{
    int primn2=(primn==TOP_OR_LEFT ? BOTTOM_OR_RIGHT : TOP_OR_LEFT);
    WRegion *reg=NULL, *curr=ionws_current(ws);
    if(curr!=NULL)
        reg=do_get_next_to(ws, curr, dir, primn);
    if(reg==NULL)
        reg=do_get_farthest(ws, dir, primn2);
    if(reg!=NULL)
        region_goto(reg);
    return reg;
}


/*EXTL_DOC
 * Go to the most previously active region on \var{ws} next to \var{reg} in
 * direction \var{dirstr} (up/down/left/right), wrapping around to a most 
 * recently active farthest region in the opposite direction if \var{reg} 
 * is already the further region in the given direction.
 * 
 * Note that this function is asynchronous; the region will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_dir(WIonWS *ws, const char *dirstr)
{
    int dir=0, primn=0;

    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    return do_goto_dir(ws, dir, primn);
}


static WRegion *do_goto_dir_nowrap(WIonWS *ws, int dir, int primn)
{
    int primn2=(primn==TOP_OR_LEFT ? BOTTOM_OR_RIGHT : TOP_OR_LEFT);
    WRegion *reg=NULL, *curr=ionws_current(ws);
    if(curr!=NULL)
        reg=do_get_next_to(ws, curr, dir, primn);
    if(reg!=NULL)
        region_goto(reg);
    return reg;
}


/*EXTL_DOC
 * Go to the most previously active region on \var{ws} next to \var{reg} in
 * direction \var{dirstr} (up/down/left/right) without wrapping around.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_dir_nowrap(WIonWS *ws, const char *dirstr)
{
    int dir=0, primn=0;

    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    return do_goto_dir_nowrap(ws, dir, primn);
}


/*EXTL_DOC
 * Find region on \var{ws} overlapping coordinates $(x, y)$.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_region_at(WIonWS *ws, int x, int y)
{
    return split_tree_region_at(ws->split_tree, x, y);
}


/*EXTL_DOC
 * For region \var{reg} managed by \var{ws} return the \type{WWsSplit}
 * a leaf of which \var{reg} is.
 */
EXTL_EXPORT_MEMBER
WWsSplit *ionws_split_of(WIonWS *ws, WRegion *reg)
{
    if(reg==NULL){
        warn_obj("ionws_split_of", "nil parameter");
        return NULL;
    }
    
    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        warn_obj("ionws_split_of", "Manager doesn't match");
        return NULL;
    }
    
    return split_tree_split_of((Obj*)reg);
}


/*}}}*/


/*{{{ Misc. */


void ionws_managed_activated(WIonWS *ws, WRegion *reg)
{
    split_tree_mark_current(reg);
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
            ionws_managed_add(ws, reg);
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

