/*
 * ion/mod_ionws/ionws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libmainloop/defer.h>
#include <libmainloop/signal.h>

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
#include <libextl/extl.h>
#include <ioncore/region-iter.h>
#include <ioncore/regbind.h>
#include <ioncore/extlconv.h>
#include <ioncore/xwindow.h>
#include "placement.h"
#include "ionws.h"
#include "split.h"
#include "splitfloat.h"
#include "split-stdisp.h"
#include "main.h"


/*{{{ Some helper routines */


#define STDISP_OF(WS) \
     ((WS)->stdispnode!=NULL ? (WS)->stdispnode->regnode.reg : NULL)


static WSplitRegion *get_node_check(WIonWS *ws, WRegion *reg)
{
    WSplitRegion *node;

    if(reg==NULL)
        return NULL;
    
    node=splittree_node_of(reg);
    
    if(node==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return NULL;
    
    return node;
}


/*}}}*/


/*{{{ Dynfun implementations */


static void reparent_mgd(WRegion *sub, WWindow *par)
{
    WFitParams subfp;
    subfp.g=REGION_GEOM(sub);
    subfp.mode=REGION_FIT_EXACT;
    if(!region_fitrep(sub, par, &subfp)){
        warn(TR("Error reparenting %s."), region_name(sub));
        region_detach_manager(sub);
    }
}


bool ionws_fitrep(WIonWS *ws, WWindow *par, const WFitParams *fp)
{
    WRegion *sub, *next;
    bool rs;
    
    if(par!=NULL){
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
    
        genws_do_reparent(&(ws->genws), par, fp);
    
        FOR_ALL_MANAGED_ON_LIST_W_NEXT(ws->managed_list, sub, next)
            reparent_mgd(sub, par);
        if(STDISP_OF(ws)!=NULL)
            reparent_mgd(STDISP_OF(ws), par);
    }
    
    REGION_GEOM(ws)=fp->g;
    
    if(ws->split_tree==NULL)
        return TRUE;
    
    split_resize(ws->split_tree, &(fp->g), PRIMN_ANY, PRIMN_ANY);
    
    return TRUE;
}


void ionws_managed_rqgeom(WIonWS *ws, WRegion *mgd, 
                          int flags, const WRectangle *geom,
                          WRectangle *geomret)
{
    WSplitRegion *node=get_node_check(ws, mgd);
    if(node!=NULL && ws->split_tree!=NULL)
        splittree_rqgeom((WSplit*)node, flags, geom, geomret);
}


void ionws_map(WIonWS *ws)
{
    genws_do_map(&(ws->genws));
    
    if(ws->split_tree!=NULL)
        split_map(ws->split_tree);
}


void ionws_unmap(WIonWS *ws)
{
    genws_do_unmap(&(ws->genws));

    if(ws->split_tree!=NULL)
        split_unmap(ws->split_tree);
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


static WTimer *restack_timer=NULL;
static Watch restack_watch=WATCH_INIT;


static void expire(WTimer *tmr, bool done)
{
    WIonWS *ws=(WIonWS*)restack_watch.obj;
    
    if(ws!=NULL)
        split_restack(ws->split_tree, ws->genws.dummywin, Above);
    
    watch_reset(&restack_watch);
    
    if(done && restack_timer!=NULL){
        destroy_obj((Obj*)restack_timer);
        restack_timer=NULL;
    }
}


static void restack_handler(WTimer *tmr)
{
    expire(tmr, TRUE);
}


bool ionws_managed_goto(WIonWS *ws, WRegion *reg, int flags)
{
    WSplitRegion *node=get_node_check(ws, reg);
    int rd=mod_ionws_raise_delay;

    if(!REGION_IS_MAPPED(ws))
        return FALSE;

    if(node!=NULL && node->split.parent!=NULL)
        splitinner_mark_current(node->split.parent, &(node->split));
        
    /* WSplitSplit uses activity based stacking as required on WAutoWS,
     * so we must restack here.
     */
    if(ws->split_tree!=NULL){
        bool use_timer=rd>0 && flags&REGION_GOTO_ENTERWINDOW;
        
        if(use_timer){
            if(restack_timer!=NULL){
                if(restack_watch.obj!=(Obj*)ws)
                    expire(restack_timer, FALSE);
            }else{
                restack_timer=create_timer();
            }
        }
        
        if(use_timer && restack_timer!=NULL){
            watch_setup(&restack_watch, (Obj*)ws, NULL);
            timer_set(restack_timer, rd, restack_handler);
        }else{
            split_restack(ws->split_tree, ws->genws.dummywin, Above);
        }
    }

    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp(reg, !(flags&REGION_GOTO_NOWARP));

    return TRUE;
}



void ionws_restack(WIonWS *ws, Window other, int mode)
{
    xwindow_restack(ws->genws.dummywin, other, mode);
    if(ws->split_tree!=NULL)
        split_restack(ws->split_tree, ws->genws.dummywin, Above);
}


void ionws_stacking(WIonWS *ws, Window *bottomret, Window *topret)
{
    Window sbottom=None, stop=None;
    
    if(ws->split_tree!=None)
        split_stacking(ws->split_tree, &sbottom, &stop);
    
    *bottomret=ws->genws.dummywin;
    *topret=(stop!=None ? stop : ws->genws.dummywin);
}


/*}}}*/


/*{{{ Status display support code */


static bool regnodefilter(WSplit *split)
{
    return OBJ_IS(split, WSplitRegion);
}


void ionws_unmanage_stdisp(WIonWS *ws, bool permanent, bool nofocus)
{
    WSplitRegion *tofocus=NULL;
    bool setfocus=FALSE;
    WRegion *od;
    
    if(ws->stdispnode==NULL)
        return;
    
    od=ws->stdispnode->regnode.reg;
    
    if(od!=NULL){
        if(!nofocus && REGION_IS_ACTIVE(od) && 
           region_may_control_focus((WRegion*)ws)){
            setfocus=TRUE;
            tofocus=(WSplitRegion*)split_nextto((WSplit*)(ws->stdispnode), 
                                                SPLIT_ANY, PRIMN_ANY,
                                                regnodefilter);
        }
        /* Reset node_of info here so ionws_managed_remove will not
         * remove the node.
         */
        splittree_set_node_of(od, NULL);
        ionws_managed_remove(ws, od);
        /*ws->stdispnode->u.reg=NULL;*/
    }
    
    if(permanent){
        WSplit *node=(WSplit*)ws->stdispnode;
        ws->stdispnode=NULL;
        splittree_remove(node, TRUE);
    }
    
    if(setfocus){
        if(tofocus!=NULL)
            region_set_focus(tofocus->reg);
        else
            genws_fallback_focus((WGenWS*)ws, FALSE);
    }
}


static void ionws_create_stdispnode(WIonWS *ws, WRegion *stdisp, 
                                    int corner, int orientation)
{
    int flags=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    WRectangle *wg=&REGION_GEOM(ws), dg;
    WSplitST *stdispnode;
    WSplitSplit *split;

    assert(ws->split_tree!=NULL);
    
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
            
    stdispnode=create_splitst(&dg, stdisp);
    
    if(stdispnode==NULL){
        warn(TR("Unable to create a node for status display."));
        return;
    }
    
    stdispnode->corner=corner;
    stdispnode->orientation=orientation;
    
    split=create_splitsplit(wg, (orientation==REGION_ORIENTATION_HORIZONTAL 
                                 ? SPLIT_VERTICAL
                                 : SPLIT_HORIZONTAL));

    if(split==NULL){
        warn(TR("Unable to create new split for status display."));
        stdispnode->regnode.reg=NULL;
        destroy_obj((Obj*)stdispnode);
        return;
    }

    /* Set up new split tree */
    ((WSplit*)stdispnode)->parent=(WSplitInner*)split;
    ws->split_tree->parent=(WSplitInner*)split;
    ws->split_tree->ws_if_root=NULL;
    
    if((orientation==REGION_ORIENTATION_HORIZONTAL && 
        (corner==MPLEX_STDISP_BL || corner==MPLEX_STDISP_BR)) ||
       (orientation==REGION_ORIENTATION_VERTICAL && 
        (corner==MPLEX_STDISP_TR || corner==MPLEX_STDISP_BR))){
        split->tl=ws->split_tree;
        split->br=(WSplit*)stdispnode;
    }else{
        split->tl=(WSplit*)stdispnode;
        split->br=ws->split_tree;
    }

    ws->split_tree=(WSplit*)split;
    ((WSplit*)split)->ws_if_root=ws;
    ws->stdispnode=stdispnode;
}


void ionws_manage_stdisp(WIonWS *ws, WRegion *stdisp, int corner)
{
    bool mcf=region_may_control_focus((WRegion*)ws);
    int flags=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    int orientation=region_orientation(stdisp);
    bool act=FALSE;
    WRectangle dg, *stdg;
    
    if(orientation!=REGION_ORIENTATION_VERTICAL /*&&
       orientation!=REGION_ORIENTATION_HORIZONTAL*/){
        orientation=REGION_ORIENTATION_HORIZONTAL;
    }
    
    if(ws->stdispnode==NULL || ws->stdispnode->regnode.reg!=stdisp)
        region_detach_manager(stdisp);

    /* Remove old stdisp if corner and orientation don't match.
     */
    if(ws->stdispnode!=NULL && (corner!=ws->stdispnode->corner ||
                                orientation!=ws->stdispnode->orientation)){
        ionws_unmanage_stdisp(ws, TRUE, TRUE);
    }

    if(ws->stdispnode==NULL){
        ionws_create_stdispnode(ws, stdisp, corner, orientation);
    }else{
        WRegion *od=ws->stdispnode->regnode.reg;
        if(od!=NULL){
            act=REGION_IS_ACTIVE(od);
            splittree_set_node_of(od, NULL);
            ionws_managed_remove(ws, od);
            assert(ws->stdispnode->regnode.reg==NULL);
        }
        
        ws->stdispnode->regnode.reg=stdisp;
        splittree_set_node_of(stdisp, &(ws->stdispnode->regnode));
    }
    
    ionws_managed_add(ws, stdisp);
    
    dg=((WSplit*)(ws->stdispnode))->geom;
    
    if(orientation==REGION_ORIENTATION_HORIZONTAL)
        dg.h=maxof(CF_STDISP_MIN_SZ, region_min_h(stdisp));
    else
        dg.w=maxof(CF_STDISP_MIN_SZ, region_min_w(stdisp));
    
    splittree_rqgeom((WSplit*)(ws->stdispnode), flags, &dg, FALSE);
    
    stdg=&(((WSplit*)ws->stdispnode)->geom);
    
    if(stdisp->geom.x!=stdg->x || stdisp->geom.y!=stdg->y ||
       stdisp->geom.w!=stdg->w || stdisp->geom.h!=stdg->h){
        region_fit(stdisp, stdg, REGION_FIT_EXACT);
    }

    /* Restack to ensure the split tree is stacked in the expected order. */
    if(ws->split_tree!=NULL)
        split_restack(ws->split_tree, ws->genws.dummywin, Above);
    
    if(mcf && act)
        region_set_focus(stdisp);
}


/*}}}*/


/*{{{ Create/destroy */


void ionws_managed_add_default(WIonWS *ws, WRegion *reg)
{
    Window bottom=None, top=None;
    
    /*region_stacking((WRegion*)ws, &bottom, &top);
    region_restack(reg, top, Above);*/
    
    if(STDISP_OF(ws)==reg)
        region_set_manager(reg, (WRegion*)ws, NULL);
    else
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
    
    ws->split_tree=(WSplit*)create_splitregion(&(fp->g), reg);
    if(ws->split_tree==NULL){
        destroy_obj((Obj*)reg);
        return NULL;
    }
    ws->split_tree->ws_if_root=ws;
    
    ionws_managed_add(ws, reg);

    return reg;
}


static WRegion *create_frame_ionws(WWindow *parent, const WFitParams *fp)
{
    return (WRegion*)create_frame(parent, fp, "frame-tiled-ionws");
}


bool ionws_init(WIonWS *ws, WWindow *parent, const WFitParams *fp, 
                WRegionSimpleCreateFn *create_frame_fn, bool ci)
{
    ws->split_tree=NULL;
    ws->create_frame_fn=(create_frame_fn 
                         ? create_frame_fn
                         : create_frame_ionws);
    ws->stdispnode=NULL;
    ws->managed_list=NULL;

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
    
    ionws_unmanage_stdisp(ws, TRUE, TRUE);

    if(ws->split_tree!=NULL)
        destroy_obj((Obj*)(ws->split_tree));

    genws_deinit(&(ws->genws));
}


bool ionws_managed_may_destroy(WIonWS *ws, WRegion *reg)
{
    if(ws->split_tree!=NULL){
        WSplitRegion *regnode=OBJ_CAST(ws->split_tree, WSplitRegion);
        if(regnode!=NULL && regnode->reg==reg)
            return region_may_destroy((WRegion*)ws);
    }
    
    return TRUE;
}


bool ionws_rescue_clientwins(WIonWS *ws)
{
    return region_rescue_managed_clientwins((WRegion*)ws, ws->managed_list);
}


void ionws_do_managed_remove(WIonWS *ws, WRegion *reg)
{
    if(STDISP_OF(ws)==reg){
        region_unset_manager(reg, (WRegion*)ws, NULL);
        ws->stdispnode->regnode.reg=NULL;
    }else{
        region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));
    }
    
    region_remove_bindmap_owned(reg, mod_ionws_ionws_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_remove_bindmap(reg, mod_ionws_frame_bindmap);
}


static bool nostdispfilter(WSplit *node)
{
    return (OBJ_IS(node, WSplitRegion) && !OBJ_IS(node, WSplitST));
}


void ionws_managed_remove(WIonWS *ws, WRegion *reg)
{
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    bool act=REGION_IS_ACTIVE(reg);
    bool mcf=region_may_control_focus((WRegion*)ws);
    WSplitRegion *node=get_node_check(ws, reg);
    WRegion *other;

    other=ionws_do_get_nextto(ws, reg, SPLIT_ANY, PRIMN_ANY, FALSE);
    
    ionws_do_managed_remove(ws, reg);

    if(node==(WSplitRegion*)(ws->stdispnode))
        ws->stdispnode=NULL;
    
    if(node==NULL)
        return;
    
    splittree_remove((WSplit*)node, !ds);
    
    if(!ds){
        if(other==NULL)
            mainloop_defer_destroy((Obj*)ws);
        else if(act && mcf)
            region_warp(other);
    }
}


static bool mplexfilter(WSplit *node)
{
    WSplitRegion *regnode=OBJ_CAST(node, WSplitRegion);
    
    return (regnode!=NULL && regnode->reg!=NULL &&
            OBJ_IS(regnode->reg, WMPlex));
}


bool ionws_manage_rescue(WIonWS *ws, WClientWin *cwin, WRegion *from)
{
    WSplitRegion *node=get_node_check(ws, from), *other;
    
    if(REGION_MANAGER(from)!=(WRegion*)ws || node==NULL)
        return FALSE;

    other=(WSplitRegion*)split_nextto((WSplit*)node, SPLIT_ANY, PRIMN_ANY,
                                      mplexfilter);

    if(other==NULL || other->reg==NULL)
        return FALSE;
    
    return (NULL!=mplex_attach_simple((WMPlex*)other->reg, (WRegion*)cwin, 0));
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


static bool get_split_dir_primn_float(const char *str, int *dir, int *primn,
                                      bool *floating)
{
    if(strncmp(str, "floating:", 9)==0){
        *floating=TRUE;
        return get_split_dir_primn(str+9, dir, primn);
    }else{
        *floating=FALSE;
        return get_split_dir_primn(str, dir, primn);
    }
}


#define SPLIT_MINS 16 /* totally arbitrary */


static WFrame *ionws_do_split(WIonWS *ws, WSplit *node, 
                              const char *dirstr, int minw, int minh)
{
    int dir, primn, mins;
    bool floating=FALSE;
    WFrame *newframe;
    WSplitRegion *nnode;
    
    if(node==NULL || ws->split_tree==NULL){
        warn(TR("Invalid node."));
        return NULL;
    }
    
    if(!get_split_dir_primn_float(dirstr, &dir, &primn, &floating)){
        warn(TR("Invalid split type parameter."));
        return NULL;
    }
    
    mins=(dir==SPLIT_VERTICAL ? minh : minw);

    if(!floating){
        nnode=splittree_split(node, dir, primn, mins, 
                              ws->create_frame_fn, 
                              REGION_PARENT(ws));
    }else{
        nnode=splittree_split_floating(node, dir, primn, mins,
                                       ws->create_frame_fn, ws);
    }
    
    if(nnode==NULL){
        warn(TR("Unable to split."));
        return NULL;
    }

    /* We must restack here to ensure the split tree is stacked in the
     * expected order.
     */
    if(ws->split_tree!=NULL)
        split_restack(ws->split_tree, ws->genws.dummywin, Above);

    newframe=OBJ_CAST(nnode->reg, WFrame);
    assert(newframe!=NULL);

    ionws_managed_add(ws, nnode->reg);

    /* Restack */
    if(ws->split_tree!=NULL)
        split_restack(ws->split_tree, ws->genws.dummywin, Above);
    
    return newframe;
}


/*EXTL_DOC
 * Create a new frame on \var{ws} above/below/left of/right of
 * \var{node} as indicated by \var{dirstr}. If \var{dirstr} is 
 * prefixed with ''floating:'' a floating split is created.
 */
EXTL_EXPORT_MEMBER
WFrame *ionws_split(WIonWS *ws, WSplit *node, const char *dirstr)
{
    return ionws_do_split(ws, node, dirstr,
                          SPLIT_MINS, SPLIT_MINS);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.split} at the root of the split tree.
 */
EXTL_EXPORT_MEMBER
WFrame *ionws_split_top(WIonWS *ws, const char *dirstr)
{
    return ionws_do_split(ws, ws->split_tree, dirstr, 
                          SPLIT_MINS, SPLIT_MINS);
}


/*EXTL_DOC
 * Split \var{frame} creating a new frame to direction \var{dirstr}
 * (one of ''left'', ''right'', ''top'' or ''bottom'') of \var{frame}.
 * If \var{attach_current} is set, the region currently displayed in
 * \var{frame}, if any, is moved to thenew frame.
 * If \var{dirstr} is prefixed with ''floating:'' a floating split is
 * created.
 */
EXTL_EXPORT_MEMBER
WFrame *ionws_split_at(WIonWS *ws, WFrame *frame, const char *dirstr, 
                       bool attach_current)
{
    WRegion *curr;
    WSplitRegion *node;
    WFrame *newframe;
    
    if(frame==NULL)
        return NULL;
    
    node=get_node_check(ws, (WRegion*)frame);

    newframe=ionws_do_split(ws, (WSplit*)node, dirstr, 
                            region_min_w((WRegion*)frame),
                            region_min_h((WRegion*)frame));

    if(newframe==NULL)
        return NULL;

    curr=mplex_lcurrent(&(frame->mplex), 1);
    
    if(attach_current && curr!=NULL){
        if(mplex_lcount(&(frame->mplex), 1)<=1)
            frame->flags&=~FRAME_DEST_EMPTY;
        mplex_attach_simple(&(newframe->mplex), curr, MPLEX_ATTACH_SWITCHTO);
    }
    
    if(region_may_control_focus((WRegion*)frame))
        region_goto((WRegion*)newframe);

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
        warn(TR("Nil frame."));
        return;
    }
    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        warn(TR("The frame is not managed by the workspace."));
        return;
    }
    
    if(!region_may_destroy((WRegion*)frame)){
        warn(TR("Frame may not be destroyed."));
        return;
    }

    if(!region_rescue_clientwins((WRegion*)frame)){
        warn(TR("Failed to rescue managed objects."));
        return;
    }

    mainloop_defer_destroy((Obj*)frame);
}


/*}}}*/


/*{{{ Navigation etc. exports */


/*EXTL_DOC
 * Returns most recently active region on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_current(WIonWS *ws)
{
    WSplitRegion *node=NULL;
    if(ws->split_tree!=NULL){
        node=(WSplitRegion*)split_current_todir(ws->split_tree, SPLIT_ANY,
                                                PRIMN_ANY, NULL);
    }
    return (node ? node->reg : NULL);
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


static WRegion *ionws_do_get_nextto_default(WIonWS *ws, WRegion *reg,
                                            int dir, int primn, bool any)
{
    WSplitFilter *filter=(any ? NULL : nostdispfilter);
    WSplitRegion *node=get_node_check(ws, reg);
    if(node!=NULL)
        node=(WSplitRegion*)split_nextto((WSplit*)node, dir, primn, filter);
    return (node ? node->reg : NULL);
}


WRegion *ionws_do_get_nextto(WIonWS *ws, WRegion *reg,
                             int dir, int primn, bool any)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, ionws_do_get_nextto, ws,
                 (ws, reg, dir, primn, any));
    return ret;
}


/*EXTL_DOC
 * Return the most previously active region next to \var{reg} in
 * direction \var{dirstr} (left/right/up/down). The region \var{reg}
 * must be managed by \var{ws}. If \var{any} is not set, the status display
 * is not considered.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_nextto(WIonWS *ws, WRegion *reg, const char *dirstr,
                      bool any)
{
    int dir=0, primn=0;
    
    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    return ionws_do_get_nextto(ws, reg, dir, primn, any);
}


static WRegion *ionws_do_get_farthest_default(WIonWS *ws,
                                              int dir, int primn, bool any)
{
    WSplitFilter *filter=(any ? NULL : nostdispfilter);
    WSplitRegion *node=NULL;
    if(ws->split_tree!=NULL)
        node=(WSplitRegion*)split_current_todir(ws->split_tree, dir, primn, filter);
    return (node ? node->reg : NULL);
}


WRegion *ionws_do_get_farthest(WIonWS *ws, 
                               int dir, int primn, bool any)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, ionws_do_get_farthest, ws,
                 (ws, dir, primn, any));
    return ret;
}


/*EXTL_DOC
 * Return the most previously active region on \var{ws} with no
 * other regions next to it in  direction \var{dirstr} 
 * (left/right/up/down). If \var{any} is not set, the status 
 * display is not considered.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_farthest(WIonWS *ws, const char *dirstr, bool any)
{
    int dir=0, primn=0;

    if(!get_split_dir_primn(dirstr, &dir, &primn))
        return NULL;
    
    return ionws_do_get_farthest(ws, dir, primn, any);
}


static WRegion *do_goto_dir(WIonWS *ws, int dir, int primn)
{
    int primn2=(primn==PRIMN_TL ? PRIMN_BR : PRIMN_TL);
    WRegion *reg=NULL, *curr=ionws_current(ws);
    if(curr!=NULL)
        reg=ionws_do_get_nextto(ws, curr, dir, primn, FALSE);
    if(reg==NULL)
        reg=ionws_do_get_farthest(ws, dir, primn2, FALSE);
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
        reg=ionws_do_get_nextto(ws, curr, dir, primn, FALSE);
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
/*EXTL_EXPORT_MEMBER
WRegion *ionws_region_at(WIonWS *ws, int x, int y)
{
    if(ws->split_tree==NULL)
        return NULL;
    return split_region_at(ws->split_tree, x, y);
}*/


/*EXTL_DOC
 * For region \var{reg} managed by \var{ws} return the \type{WSplit}
 * a leaf of which \var{reg} is.
 */
EXTL_EXPORT_MEMBER
WSplitRegion *ionws_node_of(WIonWS *ws, WRegion *reg)
{
    if(reg==NULL){
        warn(TR("Nil parameter."));
        return NULL;
    }
    
    if(REGION_MANAGER(reg)!=(WRegion*)ws){
        warn(TR("Manager doesn't match."));
        return NULL;
    }
    
    return splittree_node_of(reg);
}


/*}}}*/


/*{{{ Save */


ExtlTab ionws_get_configuration(WIonWS *ws)
{
    ExtlTab tab, split_tree=extl_table_none();
    
    tab=region_get_base_configuration((WRegion*)ws);
    
    if(ws->split_tree!=NULL){
        if(!split_get_config(ws->split_tree, &split_tree))
            warn(TR("Could not get split tree."));
    }
    
    extl_table_sets_t(tab, "split_tree", split_tree);
    extl_unref_table(split_tree);
    
    return tab;
}


/*}}}*/


/*{{{ Load */


WSplit *load_splitst(WIonWS *ws, const WRectangle *geom, ExtlTab tab)
{
    WSplitST *st;

    if(ws->stdispnode!=NULL){
        warn(TR("Workspace already has a status display node."));
        return NULL;
    }

    st=create_splitst(geom, NULL);
    ws->stdispnode=st;
    return (WSplit*)st;
}


static WRegion *do_attach(WIonWS *ws, WRegionAttachHandler *handler,
                          void *handlerparams, const WRectangle *geom)
{
    WWindow *par=REGION_PARENT(ws);
    WFitParams fp;
    assert(par!=NULL);
    fp.g=*geom;
    fp.mode=REGION_FIT_EXACT;
    
    return handler(par, &fp, handlerparams);
}


WSplit *load_splitregion_doit(WIonWS *ws, const WRectangle *geom, ExtlTab rt)
{
    WSplitRegion *node=NULL;
    WRegion *reg;
    
    reg=region__attach_load((WRegion*)ws,
                            rt, (WRegionDoAttachFn*)do_attach, 
                            (void*)geom);
        
    if(reg!=NULL){
        node=create_splitregion(geom, reg);
        if(node==NULL)
            destroy_obj((Obj*)reg);
        else
            ionws_managed_add(ws, reg);
    }
    
    return (WSplit*)node;
}


WSplit *load_splitregion(WIonWS *ws, const WRectangle *geom, ExtlTab tab)
{
    WSplit *node=NULL;
    ExtlTab rt;
    
    if(!extl_table_gets_t(tab, "regparams", &rt)){
        warn(TR("Missing region parameters."));
        return NULL;
    }
    
    node=load_splitregion_doit(ws, geom, rt);

    extl_unref_table(rt);
    
    return node;
}


#define MINS 1

WSplit *load_splitsplit(WIonWS *ws, const WRectangle *geom, ExtlTab tab)
{
    WSplit *tl=NULL, *br=NULL;
    WSplitSplit *split;
    char *dir_str;
    int dir, brs, tls;
    ExtlTab subtab;
    WRectangle geom2;
    int set=0;

    set+=(extl_table_gets_i(tab, "tls", &tls)==TRUE);
    set+=(extl_table_gets_i(tab, "brs", &brs)==TRUE);
    set+=(extl_table_gets_s(tab, "dir", &dir_str)==TRUE);
    
    if(set!=3)
        return NULL;
    
    if(strcmp(dir_str, "vertical")==0){
        dir=SPLIT_VERTICAL;
    }else if(strcmp(dir_str, "horizontal")==0){
        dir=SPLIT_HORIZONTAL;
    }else{
        warn(TR("Invalid direction."));
        free(dir_str);
        return NULL;
    }
    free(dir_str);

    split=create_splitsplit(geom, dir);
    if(split==NULL)
        return NULL;

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
    if(dir==SPLIT_HORIZONTAL){
        geom2.w-=tls;
        geom2.x+=tls;
    }else{
        geom2.h-=tls;
        geom2.y+=tls;
    }
            
    if(extl_table_gets_t(tab, "br", &subtab)){
        br=ionws_load_node(ws, &geom2, subtab);
        extl_unref_table(subtab);
    }
    
    if(tl==NULL || br==NULL){
        destroy_obj((Obj*)split);
        if(tl!=NULL){
            split_do_resize(tl, geom, PRIMN_ANY, PRIMN_ANY, FALSE);
            return tl;
        }
        if(br!=NULL){
            split_do_resize(br, geom, PRIMN_ANY, PRIMN_ANY, FALSE);
            return br;
        }
        return NULL;
    }
    
    tl->parent=(WSplitInner*)split;
    br->parent=(WSplitInner*)split;

    /*split->tmpsize=tls;*/
    split->tl=tl;
    split->br=br;
    
    return (WSplit*)split;
}


WSplit *ionws_load_node_default(WIonWS *ws, const WRectangle *geom, 
                                ExtlTab tab)
{
    char *typestr=NULL;
    WSplit *node=NULL;

    extl_table_gets_s(tab, "type", &typestr);
    
    if(typestr==NULL){
        warn(TR("No split type given."));
        return NULL;
    }
    
    if(strcmp(typestr, "WSplitRegion")==0)
        node=load_splitregion(ws, geom, tab);
    else if(strcmp(typestr, "WSplitSplit")==0)
        node=load_splitsplit(ws, geom, tab);
    else if(strcmp(typestr, "WSplitFloat")==0)
        node=load_splitfloat(ws, geom, tab);
    else if(strcmp(typestr, "WSplitST")==0)
        node=NULL;/*load_splitst(ws, geom, tab);*/
    else
        warn(TR("Unknown split type."));
    
    free(typestr);
    
    return node;
}


WSplit *ionws_load_node(WIonWS *ws, const WRectangle *geom, ExtlTab tab)
{
    WSplit *ret=NULL;
    CALL_DYN_RET(ret, WSplit*, ionws_load_node, ws, (ws, geom, tab));
    return ret;
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
        warn(TR("The workspace is empty."));
        destroy_obj((Obj*)ws);
        return NULL;
    }
    
    ws->split_tree->ws_if_root=ws;
    split_restack(ws->split_tree, ws->genws.dummywin, Above);
    
    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionws_dynfuntab[]={
    {region_map, 
     ionws_map},
    
    {region_unmap, 
     ionws_unmap},
    
    {region_do_set_focus, 
     ionws_do_set_focus},
    
    {(DynFun*)region_fitrep,
     (DynFun*)ionws_fitrep},
    
    {region_managed_rqgeom, 
     ionws_managed_rqgeom},
    
    {region_managed_remove, 
     ionws_managed_remove},
    
    {(DynFun*)region_managed_goto,
     (DynFun*)ionws_managed_goto},
    
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
    
    {(DynFun*)ionws_load_node,
     (DynFun*)ionws_load_node_default},
            
    {region_restack,
     ionws_restack},

    {region_stacking,
     ionws_stacking},
    
    {(DynFun*)ionws_do_get_nextto,
     (DynFun*)ionws_do_get_nextto_default},

    {(DynFun*)ionws_do_get_farthest,
     (DynFun*)ionws_do_get_farthest_default},
     
    END_DYNFUNTAB
};


IMPLCLASS(WIonWS, WGenWS, ionws_deinit, ionws_dynfuntab);

    
/*}}}*/

