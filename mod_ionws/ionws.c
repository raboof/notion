/*
 * ion/mod_ionws/ionws.c
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
#include <ioncore/xwindow.h>
#include "placement.h"
#include "ionws.h"
#include "split.h"
#include "split-stdisp.h"
#include "main.h"


/*{{{ Dynfun implementations */


bool ionws_fitrep(WIonWS *ws, WWindow *par, const WFitParams *fp)
{
    WRegion *sub, *next;
    bool rs;
    
    if(par!=NULL){
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
    
        genws_do_reparent(&(ws->genws), par, fp);
    
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
    
    split_resize(ws->split_tree, &(fp->g), PRIMN_ANY, PRIMN_ANY);
    
    return TRUE;
}


void ionws_map(WIonWS *ws)
{
    WRegion *reg;

    genws_do_map(&(ws->genws));
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        region_map(reg);
    }
}


void ionws_unmap(WIonWS *ws)
{
    WRegion *reg;
    
    genws_do_unmap(&(ws->genws));
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        region_unmap(reg);
    }
}


void ionws_do_set_focus(WIonWS *ws, bool warp)
{
    WRegion *sub=ionws_current(ws);
    
    if(sub==NULL){
        genws_fallback_focus(&(ws->genws), warp);
        return;
    }

    region_do_set_focus(sub, warp);
}


bool ionws_managed_display(WIonWS *ws, WRegion *reg)
{
    return TRUE;
}


void ionws_unmanage_stdisp(WIonWS *ws, bool permanent, bool nofocus)
{
    bool setfocus=FALSE;
    WSplit *tofocus=NULL;
    WRegion *od;
    
    if(ws->stdispnode==NULL)
        return;
    
    od=ws->stdispnode->u.reg;
    
    if(od!=NULL){
        if(!nofocus && REGION_IS_ACTIVE(od) && 
           region_may_control_focus((WRegion*)ws)){
            setfocus=TRUE;
            tofocus=split_closest_leaf(ws->stdispnode, NULL);
        }
        ionws_do_managed_remove(ws, od);
        ws->stdispnode->u.reg=NULL;
    }
    
    if(permanent){
        split_tree_remove(&(ws->split_tree), ws->stdispnode, TRUE, FALSE);
        ws->stdispnode=NULL;
    }
    
    if(setfocus){
        if(tofocus!=NULL)
            region_set_focus(tofocus->u.reg);
        else
            genws_fallback_focus((WGenWS*)ws, FALSE);
    }
}


static void ionws_create_stdispnode(WIonWS *ws, WRegion *stdisp, 
                                    int corner, int orientation)
{
    WSplit *stdispnode, *split;
    WRectangle *wg=&REGION_GEOM(ws);
    WRectangle dg;
    int flags=REGION_RQGEOM_NORMAL;
    
    if(orientation==REGION_ORIENTATION_HORIZONTAL){
        dg.x=wg->x;
        dg.w=wg->w;
        dg.h=0;
        dg.y=((corner==MPLEX_STDISP_BL || corner==MPLEX_STDISP_BR)
              ? wg->y+wg->h
              : 0);
    }else{
        dg.y=wg->y;
        dg.h=wg->h;
        dg.w=0;
        dg.x=((corner==MPLEX_STDISP_TR || corner==MPLEX_STDISP_BR)
              ? wg->x+wg->w
              : 0);
    }
            
    stdispnode=create_split_regnode(&dg, stdisp);
    
    if(stdispnode==NULL){
        WARN_FUNC("Unable to create a node for status display.");
        return;
    }
    
    stdispnode->type=SPLIT_STDISPNODE;
    stdispnode->u.d.corner=corner;
    stdispnode->u.d.orientation=orientation;
    
    split=create_split(wg, (orientation==REGION_ORIENTATION_HORIZONTAL 
                            ? SPLIT_VERTICAL
                            : SPLIT_HORIZONTAL));

    if(split==NULL){
        WARN_FUNC("Unable to create new split for status display.");
        stdispnode->u.reg=NULL;
        destroy_obj((Obj*)stdispnode);
        return;
    }

    /* Set up new split tree */
    stdispnode->parent=split;
    ws->split_tree->parent=split;
    
    if((orientation==REGION_ORIENTATION_HORIZONTAL && 
        (corner==MPLEX_STDISP_BL || corner==MPLEX_STDISP_BR)) ||
       (orientation==REGION_ORIENTATION_VERTICAL && 
        (corner==MPLEX_STDISP_TR || corner==MPLEX_STDISP_BR))){
        split->u.s.tl=ws->split_tree;
        split->u.s.br=stdispnode;
    }else{
        split->u.s.tl=stdispnode;
        split->u.s.br=ws->split_tree;
    }

    ws->split_tree=split;
    ws->stdispnode=stdispnode;
    
    ionws_managed_add(ws, stdisp);

    /* Adjust stdisp geometry. */
    
    split_regularise_stdisp(stdispnode);
    dg=stdispnode->geom;
    
    if(orientation==REGION_ORIENTATION_HORIZONTAL){
        dg.h=maxof(CF_STDISP_MIN_SZ, region_min_h(stdisp));
        flags|=REGION_RQGEOM_WEAK_Y;
    }else{
        dg.w=maxof(CF_STDISP_MIN_SZ, region_min_w(stdisp));
        flags|=REGION_RQGEOM_WEAK_X;
    }
    
    split_tree_rqgeom(ws->split_tree, stdispnode, flags, &dg, NULL);
}


void ionws_manage_stdisp(WIonWS *ws, WRegion *stdisp, int corner, 
                         int orientation)
{
    bool mcf=region_may_control_focus((WRegion*)ws);
    bool act=FALSE;
    
    /* corner/orientation not yet supported. */
    
    if(REGION_MANAGER(stdisp)==(WRegion*)ws)
        return;
    
    region_detach_manager(stdisp);

    /* Remove old stdisp if corner and orientation don't match.
     */
    if(ws->stdispnode!=NULL && (corner!=ws->stdispnode->u.d.corner ||
                                orientation!=ws->stdispnode->u.d.orientation)){
        ionws_unmanage_stdisp(ws, TRUE, TRUE);
    }
                              
    if(ws->stdispnode==NULL){
        ionws_create_stdispnode(ws, stdisp, corner, orientation);
    }else{
        if(ws->stdispnode->u.reg!=NULL){
            act=REGION_IS_ACTIVE(ws->stdispnode->u.reg);
            ionws_do_managed_remove(ws, ws->stdispnode->u.reg);
        }
        
        ws->stdispnode->u.reg=stdisp;
        split_tree_set_node_of(stdisp, ws->stdispnode);
        
        ionws_managed_add(ws, stdisp);
        
        region_fit(stdisp, &(ws->stdispnode->geom), REGION_FIT_EXACT);
        
        split_regularise_stdisp(ws->stdispnode);
        
        if(mcf && act)
            region_set_focus(stdisp);
    }
}


/*}}}*/


/*{{{ Create/destroy */


void ionws_managed_add_default(WIonWS *ws, WRegion *reg)
{
    region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
    
    region_add_bindmap_owned(reg, mod_ionws_ionws_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_add_bindmap(reg, mod_ionws_frame_bindmap);
    
    if(REGION_IS_MAPPED(ws))
        region_map(reg);
    
    if(region_may_control_focus((WRegion*)ws)){
        WRegion *curr=ionws_current(ws);
        if(curr==NULL || !REGION_IS_ACTIVE(curr))
            region_warp(reg);
    }
}


void ionws_managed_add(WIonWS *ws, WRegion *reg)
{
    CALL_DYN(ionws_managed_add, ws, (ws, reg));
}


static WRegion *create_initial_frame(WIonWS *ws, WWindow *parent,
                                     const WFitParams *fp)
{
    WRegion *reg=ws->create_frame_fn(parent, fp);

    if(reg==NULL)
        return NULL;
    
    ws->split_tree=create_split_regnode(&(fp->g), reg);
    if(ws->split_tree==NULL){
        warn_err();
        destroy_obj((Obj*)reg);
        return NULL;
    }
    
    ionws_managed_add(ws, reg);

    return reg;
}


static WRegion *create_frame_ionws(WWindow *parent, const WFitParams *fp)
{
    return (WRegion*)create_frame(parent, fp, "frame-ionframe");
}


bool ionws_init(WIonWS *ws, WWindow *parent, const WFitParams *fp, 
                WRegionSimpleCreateFn *create_frame_fn, bool ci)
{
    ws->split_tree=NULL;
    ws->create_frame_fn=(create_frame_fn 
                         ? create_frame_fn
                         : create_frame_ionws);

    if(!genws_init(&(ws->genws), parent, fp))
        return FALSE;
    
    if(ci){
        if(create_initial_frame(ws, parent, fp)==NULL){
            genws_deinit(&(ws->genws));
            return FALSE;
        }
    }
    
    return TRUE;
}


WIonWS *create_ionws(WWindow *parent, const WFitParams *fp, 
                     WRegionSimpleCreateFn *create_frame_fn, bool ci)
{
    CREATEOBJ_IMPL(WIonWS, ionws, (p, parent, fp, create_frame_fn, ci));
}


WIonWS *create_ionws_simple(WWindow *parent, const WFitParams *fp)
{
    return create_ionws(parent, fp, NULL, TRUE);
}


void ionws_deinit(WIonWS *ws)
{
    WRegion *reg;
    
    while(ws->managed_list!=NULL)
        ionws_managed_remove(ws, ws->managed_list);

    genws_deinit(&(ws->genws));
}


bool ionws_managed_may_destroy(WIonWS *ws, WRegion *reg)
{
    if(ws->split_tree!=NULL && 
       ws->split_tree->type==SPLIT_REGNODE &&
       ws->split_tree->u.reg==reg){
        return region_may_destroy((WRegion*)ws);
    }else{
        return TRUE;
    }
}


bool ionws_rescue_clientwins(WIonWS *ws)
{
    return region_rescue_managed_clientwins((WRegion*)ws, ws->managed_list);
}


static WSplit *get_node_check(WIonWS *ws, WRegion *reg)
{
    WSplit *node;

    if(reg==NULL)
        return NULL;
    
    node=split_tree_node_of(reg);
    
    if(node==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return NULL;
    
    return node;
}


void ionws_do_managed_remove(WIonWS *ws, WRegion *reg)
{
    region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));
    region_remove_bindmap_owned(reg, mod_ionws_ionws_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_remove_bindmap(reg, mod_ionws_frame_bindmap);
}


static bool nostdispfilter(WSplit *node)
{
    return (node->type==SPLIT_REGNODE && node->u.reg!=NULL);
}


void ionws_managed_remove(WIonWS *ws, WRegion *reg)
{
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    bool act=REGION_IS_ACTIVE(reg);
    bool mcf=region_may_control_focus((WRegion*)ws);
    WSplit *other=NULL, *node=get_node_check(ws, reg);
    
    assert(node!=NULL);

    ionws_do_managed_remove(ws, reg);

    other=split_closest_leaf(node, nostdispfilter);
    
    if(ws->split_tree!=NULL)
        split_tree_remove(&(ws->split_tree), node, !ds, FALSE);
    
    if(!ds){
        if(other==NULL)
            ioncore_defer_destroy((Obj*)ws);
        else if(act && mcf)
            region_set_focus(other->u.reg);
    }
}


bool ionws_manage_rescue(WIonWS *ws, WClientWin *cwin, WRegion *from)
{
    WSplit *split;
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
    WSplit *node=get_node_check(ws, mgd);
    if(node!=NULL && ws->split_tree!=NULL)
        split_tree_rqgeom(ws->split_tree, node, flags, geom, geomret);
}


/*EXTL_DOC
 * Attempt to resize and/or move the split tree starting at \var{node}
 * (\type{WSplit} or \type{WRegion}). Behaviour and the \var{g} 
 * parameter are as for \fnref{WRegion.rqgeom} operating on
 * \var{node} (if it were a \type{WRegion}).
 */
EXTL_EXPORT_MEMBER
ExtlTab ionws_resize_tree(WIonWS *ws, Obj *node, ExtlTab g)
{
    WRectangle geom, ogeom;
    int flags=REGION_RQGEOM_WEAK_ALL;
    WSplit *snode;
    
    if(ws->split_tree==NULL)
        goto err;
    
    if(node!=NULL && OBJ_IS(node, WRegion)){
        geom=REGION_GEOM((WRegion*)node);
        snode=get_node_check(ws, (WRegion*)node);
        if(snode==NULL)
            goto err;
    }else if(node!=NULL && OBJ_IS(node, WSplit)){
        snode=(WSplit*)node;
        geom=snode->geom;
    }else{
        goto err;
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

    split_tree_rqgeom(ws->split_tree, snode, flags, &geom, &ogeom);
    
    return extl_table_from_rectangle(&ogeom);
    
err:
    warn("Invalid node.");
    return extl_table_none();
}


/*}}}*/


/*{{{ Split/unsplit */


static bool get_split_dir_primn(const char *str, int *dir, int *primn)
{
    if(str==NULL)
        return FALSE;
    
    if(!strcmp(str, "left")){
        *primn=PRIMN_TL;
        *dir=SPLIT_HORIZONTAL;
    }else if(!strcmp(str, "right")){
        *primn=PRIMN_BR;
        *dir=SPLIT_HORIZONTAL;
    }else if(!strcmp(str, "top") || 
             !strcmp(str, "above") || 
             !strcmp(str, "up")){
        *primn=PRIMN_TL;
        *dir=SPLIT_VERTICAL;
    }else if(!strcmp(str, "bottom") || 
             !strcmp(str, "below") ||
             !strcmp(str, "down")){
        *primn=PRIMN_BR;
        *dir=SPLIT_VERTICAL;
    }else{
        return FALSE;
    }
    
    return TRUE;
}


/*EXTL_DOC
 * Create a new frame on \var{ws} above/below/left of/right of
 * all other objects depending on \var{dirstr}
 * (one of ''left'', ''right'', ''top'' or ''bottom'').
 */
EXTL_EXPORT_MEMBER
WFrame *ionws_split_top(WIonWS *ws, const char *dirstr)
{
    int dir, primn, mins;
    WSplit *nnode=NULL;
    
    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    mins=16; /* totally arbitrary */
    
    if(ws->split_tree==NULL)
        return NULL;
    
    nnode=split_tree_split(&(ws->split_tree), ws->split_tree, 
                           dir, primn, mins, ws->create_frame_fn,
                           REGION_PARENT_CHK(ws, WWindow));
    
    if(nnode==NULL)
        return NULL;

    ionws_managed_add(ws, nnode->u.reg);
    region_warp(nnode->u.reg);
    
    return OBJ_CAST(nnode->u.reg, WFrame);
}


/*EXTL_DOC
 * Split \var{frame} creating a new frame to direction \var{dir}
 * (one of ''left'', ''right'', ''top'' or ''bottom'') of \var{frame}.
 * If \var{attach_current} is set, the region currently displayed in
 * \var{frame}, if any, is moved to thenew frame.
 */
EXTL_EXPORT_MEMBER
WFrame *ionws_split_at(WIonWS *ws, WFrame *frame, const char *dirstr, 
                       bool attach_current)
{
    WRegion *curr;
    int dir, primn, mins;
    WSplit *node, *nnode;
    WFrame *newframe;
    
    node=get_node_check(ws, (WRegion*)frame);
    
    if(node==NULL || ws->split_tree==NULL){
        WARN_FUNC("Invalid frame or frame not managed by the workspace.");
        return NULL;
    }
    
    if(!get_split_dir_primn(dirstr, &dir, &primn)){
        WARN_FUNC("Unknown direction parameter to split_at");
        return NULL;
    }
    
    mins=(dir==SPLIT_VERTICAL
          ? region_min_h((WRegion*)frame)
          : region_min_w((WRegion*)frame));
    
    nnode=split_tree_split(&(ws->split_tree), node, dir, primn, mins,
                           ws->create_frame_fn, 
                           REGION_PARENT_CHK(ws, WWindow));
    
    if(nnode==NULL){
        WARN_FUNC("Unable to split");
        return NULL;
    }

    newframe=OBJ_CAST(nnode->u.reg, WFrame);
    assert(newframe!=NULL);

    ionws_managed_add(ws, nnode->u.reg);
    
    curr=mplex_lcurrent(&(frame->mplex), 1);
    
    if(attach_current && curr!=NULL){
        if(mplex_lcount(&(frame->mplex), 1)<=1)
            frame->flags&=~FRAME_DEST_EMPTY;
        mplex_attach_simple(&(newframe->mplex), curr, MPLEX_ATTACH_SWITCHTO);
    }
    
    if(region_may_control_focus((WRegion*)frame))
        region_goto(nnode->u.reg);

    return newframe;
}


/*EXTL_DOC
 * Try to relocate regions managed by \var{frame} to another frame
 * and, if possible, destroy the frame.
 */
EXTL_EXPORT_MEMBER
void ionws_unsplit_at(WIonWS *ws, WFrame *frame)
{
    if(frame==NULL){
        WARN_FUNC("nil frame");
        return;
    }
    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        WARN_FUNC("The frame is not managed by the workspace.");
        return;
    }
    
    if(!region_may_destroy((WRegion*)frame)){
        WARN_FUNC("Frame may not be destroyed");
        return;
    }

    if(!region_rescue_clientwins((WRegion*)frame)){
        WARN_FUNC("Failed to rescue managed objects.");
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
    WSplit *node=NULL;
    if(ws->split_tree!=NULL)
        node=split_current_tl(ws->split_tree, -1, NULL);
    return (node ? node->u.reg : NULL);
}


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT_MEMBER
ExtlTab ionws_managed_list(WIonWS *ws)
{
    return managed_list_to_table(ws->managed_list, NULL);
}


/*EXTL_DOC
 * Returns the root of the split tree.
 */
EXTL_EXPORT_MEMBER
WSplit *ionws_split_tree(WIonWS *ws)
{
    return ws->split_tree;
}


static WRegion *do_get_next_to(WIonWS *ws, WRegion *reg, int dir, int primn)
{
    WSplit *node=get_node_check(ws, reg);
    
    if(node!=NULL){
        if(primn==PRIMN_TL)
            node=split_to_tl(node, dir, NULL);
        else
            node=split_to_br(node, dir, NULL);
    }
    return (node ? node->u.reg : NULL);
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
    WSplit *node=NULL;
    
    if(ws->split_tree!=NULL){
        if(primn==PRIMN_TL)
            node=split_current_tl(ws->split_tree, dir, NULL);
        else
            node=split_current_br(ws->split_tree, dir, NULL);
    }
    
    return (node ? node->u.reg : NULL);
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
    int primn2=(primn==PRIMN_TL ? PRIMN_BR : PRIMN_TL);
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
    int primn2=(primn==PRIMN_TL ? PRIMN_BR : PRIMN_TL);
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
    if(ws->split_tree==NULL)
        return NULL;
    return split_region_at(ws->split_tree, x, y);
}


/*EXTL_DOC
 * For region \var{reg} managed by \var{ws} return the \type{WSplit}
 * a leaf of which \var{reg} is.
 */
EXTL_EXPORT_MEMBER
WSplit *ionws_node_of(WIonWS *ws, WRegion *reg)
{
    if(reg==NULL){
        WARN_FUNC("nil parameter");
        return NULL;
    }
    
    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        WARN_FUNC("Manager doesn't match");
        return NULL;
    }
    
    return split_tree_node_of(reg);
}


/*}}}*/


/*{{{ Misc. */


void ionws_managed_activated(WIonWS *ws, WRegion *reg)
{
    WSplit *node=get_node_check(ws, reg);
    if(node!=NULL)
        split_mark_current(node);
}


/*}}}*/


/*{{{ Save */


static bool get_node_config(WSplit *node, ExtlTab *ret)
{
    ExtlTab tab, tltab, brtab;
    int tls, brs;
    
    assert(node!=NULL);
    
    if(node->type==SPLIT_STDISPNODE){
        tab=extl_create_table();
        extl_table_sets_b(tab, "stdispnode", TRUE);
        *ret=tab;
        return TRUE;
    }
    
    if(node->type==SPLIT_REGNODE){
        if(!region_supports_save(node->u.reg)){
            warn("Unable to get configuration for a %s.",
                 OBJ_TYPESTR(node->u.reg));
            return FALSE;
        }
        *ret=region_get_configuration(node->u.reg);
        return TRUE;
    }

    if(node->type==SPLIT_VERTICAL || node->type==SPLIT_HORIZONTAL){
        if(!get_node_config(node->u.s.tl, &tltab))
            return get_node_config(node->u.s.br, ret);
        if(!get_node_config(node->u.s.br, &brtab)){
            *ret=tltab;
            return TRUE;
        }
        
        tab=extl_create_table();

        tls=split_size(node->u.s.tl, node->type);
        brs=split_size(node->u.s.br, node->type);
        
        extl_table_sets_s(tab, "split_dir",(node->type==SPLIT_VERTICAL
                                            ? "vertical" : "horizontal"));
        
        extl_table_sets_i(tab, "split_tls", tls);
        extl_table_sets_t(tab, "tl", tltab);
        extl_unref_table(tltab);
        
        extl_table_sets_i(tab, "split_brs", brs);
        extl_table_sets_t(tab, "br", brtab);
        extl_unref_table(brtab);
        
        *ret=tab;
        return TRUE;
    }else if(node->type==SPLIT_UNUSED){
        *ret=extl_create_table();
        return TRUE;
    }

    return FALSE;
}


ExtlTab ionws_get_configuration(WIonWS *ws)
{
    ExtlTab tab, split_tree=extl_table_none();
    
    tab=region_get_base_configuration((WRegion*)ws);
    
    if(ws->split_tree!=NULL){
        if(!get_node_config(ws->split_tree, &split_tree))
            warn("Could not get split tree for a WIonWS.");
    }
    
    extl_table_sets_t(tab, "split_tree", split_tree);
    extl_unref_table(split_tree);
    
    return tab;
}


/*}}}*/


/*{{{ Load */


extern void set_split_of(Obj *obj, WSplit *split);

#define MINS 8

static WSplit *load_split(WIonWS *ws, const WRectangle *geom, ExtlTab tab)
{
    WSplit *split;
    char *dir_str;
    int dir, brs, tls;
    ExtlTab subtab;
    WSplit *tl=NULL, *br=NULL;
    WRectangle geom2;
    int set=0;

    set+=(extl_table_gets_i(tab, "split_tls", &tls)==TRUE);
    set+=(extl_table_gets_i(tab, "split_brs", &brs)==TRUE);
    set+=(extl_table_gets_s(tab, "split_dir", &dir_str)==TRUE);
    
    if(set==0){
        if(extl_table_is_bool_set(tab, "stdispnode")){
            WRegion *stdisp=NULL;
            WMPlex *mgr=NULL;
            
            if(ws->stdispnode!=NULL){
                warn("Workspace already has a stdisp node.");
                return NULL;
            }
            
            split=create_split_regnode(geom, NULL);
            split->type=SPLIT_STDISPNODE;
            ws->stdispnode=split;
            return split;
        }
        return create_split_unused(geom);
    }
    
    if(set!=3)
        return NULL;
    
    if(strcmp(dir_str, "vertical")==0){
        dir=SPLIT_VERTICAL;
    }else if(strcmp(dir_str, "horizontal")==0){
        dir=SPLIT_HORIZONTAL;
    }else{
        free(dir_str);
        return NULL;
    }
    free(dir_str);

    split=create_split(geom, dir);
    if(split==NULL){
        warn("Unable to create a split.\n");
        return NULL;
    }

    tls=maxof(tls, MINS);
    brs=maxof(brs, MINS);
        
    geom2=*geom;
    if(dir==SPLIT_HORIZONTAL){
        tls=maxof(0, geom->w)*tls/(tls+brs);
        geom2.w=tls;
    }else{
        tls=maxof(0, geom->h)*tls/(tls+brs);
        geom2.h=tls;
    }
    
    if(extl_table_gets_t(tab, "tl", &subtab)){
        tl=ionws_load_node(ws, &geom2, subtab);
        extl_unref_table(subtab);
    }

    geom2=*geom;
    if(tl!=NULL){
        if(dir==SPLIT_HORIZONTAL){
            geom2.w-=tls;
            geom2.x+=tls;
        }else{
            geom2.h-=tls;
            geom2.y+=tls;
        }
    }
            
    if(extl_table_gets_t(tab, "br", &subtab)){
        br=ionws_load_node(ws, &geom2, subtab);
        extl_unref_table(subtab);
    }
    
    if(tl==NULL || br==NULL){
        free(split);
        return (tl==NULL ? br : tl);
    }
    
    tl->parent=split;
    br->parent=split;

    /*split->tmpsize=tls;*/
    split->u.s.tl=tl;
    split->u.s.br=br;
    
    return split;
}


static WRegion *do_attach(WIonWS *ws, WRegionAttachHandler *handler,
                          void *handlerparams, const WRectangle *geom)
{
    WWindow *par=REGION_PARENT_CHK(ws, WWindow);
    WFitParams fp;
    assert(par!=NULL);
    fp.g=*geom;
    fp.mode=REGION_FIT_EXACT;
    
    return handler(par, &fp, handlerparams);
}

WSplit *ionws_load_node(WIonWS *ws, const WRectangle *geom, ExtlTab tab)
{
    char *typestr=NULL;
    Obj *reference=NULL;
    
    if(extl_table_gets_s(tab, "type", &typestr) ||
       extl_table_gets_o(tab, "reference", &reference)){
        WSplit *node=NULL;
        WRegion *reg;
        
        if(typestr!=NULL)
            free(typestr);
        
        reg=region__attach_load((WRegion*)ws,
                                tab, (WRegionDoAttachFn*)do_attach, 
                                (void*)geom);
        
        if(reg!=NULL){
            node=create_split_regnode(geom, reg);
            if(node==NULL)
                destroy_obj((Obj*)reg);
            else
                ionws_managed_add(ws, reg);
        }
        
        return node;
    }
    
    return load_split(ws, geom, tab);
}


WRegion *ionws_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WIonWS *ws;
    ExtlTab treetab;
    bool ci=TRUE;

    if(extl_table_gets_t(tab, "split_tree", &treetab))
        ci=FALSE;
    
    ws=create_ionws(par, fp, NULL, ci);
    
    if(ws==NULL){
        if(!ci)
            extl_unref_table(treetab);
        return NULL;
    }

    if(!ci){
        ws->split_tree=ionws_load_node(ws, &REGION_GEOM(ws), treetab);
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

    {ionws_managed_add,
     ionws_managed_add_default},
    
    {genws_manage_stdisp,
     ionws_manage_stdisp},

    {genws_unmanage_stdisp,
     ionws_unmanage_stdisp},
    
    END_DYNFUNTAB
};


IMPLCLASS(WIonWS, WGenWS, ionws_deinit, ionws_dynfuntab);

    
/*}}}*/

