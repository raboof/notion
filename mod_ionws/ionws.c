/*
 * ion/mod_ionws/ionws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/ptrlist.h>
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
#include <ioncore/regbind.h>
#include <ioncore/extlconv.h>
#include <ioncore/xwindow.h>
#include "placement.h"
#include "ionws.h"
#include "split.h"
#include "splitfloat.h"
#include "split-stdisp.h"
#include "main.h"



static WIonWSIterTmp ionws_iter_default_tmp;


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


static bool check_node(WIonWS *ws, WSplit *split)
{
    if(split->parent)
        return check_node(ws, (WSplit*)split->parent);
    
    if((split->ws_if_root!=(void*)ws)){
        warn(TR("Split not on workspace."));
        return FALSE;
    }
    return TRUE;
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
    WIonWSIterTmp tmp;
    bool ok=FALSE;
    
    if(par!=NULL){
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
    
        genws_do_reparent(&(ws->genws), par, fp);
    
        if(ws->split_tree!=NULL)
            split_reparent(ws->split_tree, par);
    }
    
    REGION_GEOM(ws)=fp->g;
    
    if(ws->split_tree!=NULL){
        bool done=FALSE;
        if(fp->mode&REGION_FIT_ROTATE)
            ok=split_rotate_to(ws->split_tree, &(fp->g), fp->rotation);
        if(!ok)
            split_resize(ws->split_tree, &(fp->g), PRIMN_ANY, PRIMN_ANY);
    }
    
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


static void restack_handler(WTimer *tmr, Obj *obj)
{
    if(obj!=NULL){
        WIonWS *ws=(WIonWS*)obj;
        split_restack(ws->split_tree, ws->genws.dummywin, Above);
    }
}


bool ionws_managed_goto(WIonWS *ws, WRegion *reg, 
                        WManagedGotoCont *p, int flags)
{
    WSplitRegion *node=get_node_check(ws, reg);
    int rd=mod_ionws_raise_delay;

    if(node!=NULL && node->split.parent!=NULL)
        splitinner_mark_current(node->split.parent, &(node->split));
        
    /* WSplitSplit uses activity based stacking as required on WAutoWS,
     * so we must restack here.
     */
    if(ws->split_tree!=NULL){
        bool use_timer=rd>0 && flags&REGION_GOTO_ENTERWINDOW;
        
        if(use_timer){
            if(restack_timer!=NULL){
                Obj *obj=restack_timer->objwatch.obj;
                if(obj!=(Obj*)ws){
                    timer_reset(restack_timer);
                    restack_handler(restack_timer, obj);
                }
            }else{
                restack_timer=create_timer();
            }
        }
        
        if(use_timer && restack_timer!=NULL){
            timer_set(restack_timer, rd, restack_handler, (Obj*)ws);
        }else{
            split_restack(ws->split_tree, ws->genws.dummywin, Above);
        }
    }

    return region_managed_goto_cont(reg, p, flags);
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
                                    int corner, int orientation, 
                                    bool fullsize)
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
    stdispnode->fullsize=fullsize;
    
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
        split->current=SPLIT_CURRENT_TL;
    }else{
        split->tl=(WSplit*)stdispnode;
        split->br=ws->split_tree;
        split->current=SPLIT_CURRENT_BR;
    }

    ws->split_tree=(WSplit*)split;
    ((WSplit*)split)->ws_if_root=ws;
    ws->stdispnode=stdispnode;
}


void ionws_manage_stdisp(WIonWS *ws, WRegion *stdisp, 
                         const WMPlexSTDispInfo *di)
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
    if(ws->stdispnode!=NULL && (di->pos!=ws->stdispnode->corner ||
                                orientation!=ws->stdispnode->orientation)){
        ionws_unmanage_stdisp(ws, TRUE, TRUE);
    }

    if(ws->stdispnode==NULL){
        ionws_create_stdispnode(ws, stdisp, di->pos, orientation, 
                                di->fullsize);
        if(ws->stdispnode==NULL)
            return;
    }else{
        WRegion *od=ws->stdispnode->regnode.reg;
        if(od!=NULL){
            act=REGION_IS_ACTIVE(od);
            splittree_set_node_of(od, NULL);
            ionws_managed_remove(ws, od);
            assert(ws->stdispnode->regnode.reg==NULL);
        }
        
        ws->stdispnode->fullsize=di->fullsize;
        ws->stdispnode->regnode.reg=stdisp;
        splittree_set_node_of(stdisp, &(ws->stdispnode->regnode));
    }
    
    if(!ionws_managed_add(ws, stdisp)){
        ionws_unmanage_stdisp(ws, TRUE, TRUE);
        return;
    }
    
    dg=((WSplit*)(ws->stdispnode))->geom;
    
    dg.h=stdisp_recommended_h(ws->stdispnode);
    dg.w=stdisp_recommended_w(ws->stdispnode);
    
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


bool ionws_managed_add_default(WIonWS *ws, WRegion *reg)
{
    Window bottom=None, top=None;
    
    if(STDISP_OF(ws)!=reg){
        if(!ptrlist_insert_last(&(ws->managed_list), reg))
            return FALSE;
    }
    
    region_set_manager(reg, (WRegion*)ws);
    
    if(OBJ_IS(reg, WFrame))
        region_add_bindmap(reg, mod_ionws_frame_bindmap);
    
    if(REGION_IS_MAPPED(ws))
        region_map(reg);
    
    if(region_may_control_focus((WRegion*)ws)){
        WRegion *curr=ionws_current(ws);
        if(curr==NULL || !REGION_IS_ACTIVE(curr))
            region_warp(reg);
    }
    
    return TRUE;
}


bool ionws_managed_add(WIonWS *ws, WRegion *reg)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, ionws_managed_add, ws, (ws, reg));
    return ret;
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
    
    if(!ionws_managed_add(ws, reg)){
        destroy_obj((Obj*)reg);
        destroy_obj((Obj*)ws->split_tree);
        return NULL;
    }

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
    
    ((WRegion*)ws)->flags|=REGION_GRAB_ON_PARENT;
    region_add_bindmap((WRegion*)ws, mod_ionws_ionws_bindmap);
    
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
    WIonWSIterTmp tmp;

    ionws_unmanage_stdisp(ws, FALSE, TRUE);

    FOR_ALL_MANAGED_BY_IONWS(reg, ws, tmp){
        destroy_obj((Obj*)reg);
    }

    FOR_ALL_MANAGED_BY_IONWS(reg, ws, tmp){
        assert(FALSE);
    }
    
    if(ws->split_tree!=NULL)
        destroy_obj((Obj*)(ws->split_tree));

    genws_deinit(&(ws->genws));
}


bool ionws_managed_may_destroy(WIonWS *ws, WRegion *reg)
{
    WIonWSIterTmp tmp;
    WRegion *mgd;

    FOR_ALL_MANAGED_BY_IONWS(mgd, ws, tmp){
        if(mgd!=STDISP_OF(ws) && mgd!=reg){
            return TRUE;
        }
    }
    
    return region_manager_allows_destroying((WRegion*)ws);
}


bool ionws_may_destroy(WIonWS *ws, WRegion *reg)
{
    WIonWSIterTmp tmp;
    WRegion *mgd;
    
    FOR_ALL_MANAGED_BY_IONWS(mgd, ws, tmp){
        if(mgd!=STDISP_OF(ws)){
            warn(TR("Workspace not empty - refusing to destroy."));
            return FALSE;
        }
    }
    
    return TRUE;
}


bool ionws_rescue_clientwins(WIonWS *ws, WPHolder *ph)
{
    WIonWSIterTmp tmp;
    
    ptrlist_iter_init(&tmp, ws->managed_list);
    
    return region_rescue_some_clientwins((WRegion*)ws, ph,
                                         (WRegionIterator*)ptrlist_iter, 
                                         &tmp);
}


void ionws_do_managed_remove(WIonWS *ws, WRegion *reg)
{
    region_unset_manager(reg, (WRegion*)ws);
    
    if(STDISP_OF(ws)==reg){
        ws->stdispnode->regnode.reg=NULL;
    }else{
        ptrlist_remove(&(ws->managed_list), reg);
    }
    
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


static WPHolder *find_ph_result=NULL;
static WRegion *find_ph_param=NULL;


static bool find_ph(WSplit *split)
{
    WSplitRegion *sr=OBJ_CAST(split, WSplitRegion);

    assert(find_ph_result==NULL);
    
    if(sr==NULL || sr->reg==NULL)
        return FALSE;
    
    find_ph_result=region_get_rescue_pholder_for(sr->reg, find_ph_param);

    return (find_ph_result!=NULL);
}


WPHolder *ionws_get_rescue_pholder_for(WIonWS *ws, WRegion *mgd)
{
    WSplit *node=(WSplit*)get_node_check(ws, mgd);
    WPHolder *ph;
    
    find_ph_result=NULL;
    find_ph_param=mgd;
    
    if(node==NULL){
        if(ws->split_tree!=NULL){
            split_current_todir(ws->split_tree, SPLIT_ANY, PRIMN_ANY, 
                                find_ph);
        }
    }else{
        while(node!=NULL){
            split_nextto(node, SPLIT_ANY, PRIMN_ANY, find_ph);
            if(find_ph_result!=NULL)
                break;
            node=(WSplit*)node->parent;
        }
    }
    
    ph=find_ph_result;
    find_ph_result=NULL;
    find_ph_param=NULL;
     
    return ph;
}


/*}}}*/


/*{{{ Split/unsplit */


static bool get_split_dir_primn(const char *str, int *dir, int *primn)
{
    if(str==NULL){
        warn(TR("Invalid split type parameter."));
        return FALSE;
    }
    
    if(!strcmp(str, "any")){
        *primn=PRIMN_ANY;
        *dir=SPLIT_ANY;
    }else if(!strcmp(str, "left")){
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
        warn(TR("Invalid split type parameter."));
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
    
    if(!get_split_dir_primn_float(dirstr, &dir, &primn, &floating))
        return NULL;
    
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

    if(!ionws_managed_add(ws, nnode->reg)){
        nnode->reg=NULL;
        destroy_obj((Obj*)nnode);
        destroy_obj((Obj*)newframe);
        return NULL;
    }

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
    if(!check_node(ws, node))
        return NULL;
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
bool ionws_unsplit_at(WIonWS *ws, WFrame *frame)
{
    if(frame==NULL){
        warn(TR("Nil frame."));
        return FALSE;
    }
    
    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        warn(TR("The frame is not managed by the workspace."));
        return FALSE;
    }
    
    return region_rqclose((WRegion*)frame, TRUE);
}


/*}}}*/


/*{{{ Navigation etc. exports */


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
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab ionws_managed_list(WIonWS *ws)
{
    PtrListIterTmp tmp;
    
    ptrlist_iter_init(&tmp, ws->managed_list);
    
    return extl_obj_iterable_to_table((ObjIterator*)ptrlist_iter, &tmp);
}


/*EXTL_DOC
 * Returns the root of the split tree.
 */
EXTL_SAFE
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
EXTL_SAFE
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
EXTL_SAFE
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
    WRegion *reg=NULL, *curr=ionws_current(ws);
    if(curr!=NULL)
        reg=ionws_do_get_nextto(ws, curr, dir, primn, FALSE);
    if(reg==NULL && primn!=PRIMN_ANY){
        int primn2=(primn==PRIMN_TL ? PRIMN_BR : PRIMN_TL);
        reg=ionws_do_get_farthest(ws, dir, primn2, FALSE);
    }
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
EXTL_SAFE
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


/*{{{ Flip and transpose */


static WSplitSplit *get_at_split(WIonWS *ws, WRegion *reg)
{
    WSplit *node;
    WSplitSplit *split;
    
    if(reg==NULL){
        split=OBJ_CAST(ws->split_tree, WSplitSplit);
        if(split==NULL)
            return NULL;
        else if(split->br==(WSplit*)ws->stdispnode)
            return OBJ_CAST(split->tl, WSplitSplit);
        else if(split->tl==(WSplit*)ws->stdispnode)
            return OBJ_CAST(split->br, WSplitSplit);
        else
            return split;
    }
    
    node=(WSplit*)get_node_check(ws, reg);
    
    if(node==NULL)
        return NULL;
    
    if(node==(WSplit*)ws->stdispnode){
        warn(TR("The status display is not a valid parameter for "
                "this routine."));
        return NULL;
    }
        
    split=OBJ_CAST(node->parent, WSplitSplit);
    
    if(split!=NULL && (split->tl==(WSplit*)ws->stdispnode ||
                       split->br==(WSplit*)ws->stdispnode)){
        split=OBJ_CAST(((WSplit*)split)->parent, WSplitSplit);
    }
       
    return split;
}


/*EXTL_DOC
 * Flip \var{ws} at \var{reg} or root if nil.
 */
EXTL_EXPORT_MEMBER
bool iowns_flip_at(WIonWS *ws, WRegion *reg)
{
    WSplitSplit *split=get_at_split(ws, reg);
    
    if(split==NULL){
        return FALSE;
    }else{
        splitsplit_flip(split);
        return TRUE;
    }
}


/*EXTL_DOC
 * Transpose \var{ws} at \var{reg} or root if nil.
 */
EXTL_EXPORT_MEMBER
bool iowns_transpose_at(WIonWS *ws, WRegion *reg)
{
    WSplitSplit *split=get_at_split(ws, reg);
    
    if(split==NULL){
        return FALSE;
    }else{
        split_transpose((WSplit*)split);
        return TRUE;
    }
}


/*}}}*/


/*{{{ Floating toggle */


static void replace(WSplitSplit *split, WSplitSplit *nsplit)
{
    WSplitInner *psplit=split->isplit.split.parent;

    nsplit->tl=split->tl;
    split->tl=NULL;
    nsplit->tl->parent=(WSplitInner*)nsplit;
    
    nsplit->br=split->br;
    split->br=NULL;
    nsplit->br->parent=(WSplitInner*)nsplit;
    
    if(psplit!=NULL){
        splitinner_replace((WSplitInner*)psplit, (WSplit*)split, 
                           (WSplit*)nsplit);
    }else{
        splittree_changeroot((WSplit*)split, (WSplit*)nsplit);
    }
}


WSplitSplit *ionws_set_floating(WIonWS *ws, WSplitSplit *split, int sp)
{
    bool set=OBJ_IS(split, WSplitFloat);
    bool nset=libtu_do_setparam(sp, set);
    const WRectangle *g=&((WSplit*)split)->geom;
    WSplitSplit *ns;
    
    if(!XOR(nset, set))
        return split;
    
    if(nset){
        ns=(WSplitSplit*)create_splitfloat(g, ws, split->dir);
    }else{
        if(OBJ_IS(split->tl, WSplitST) || OBJ_IS(split->br, WSplitST)){
            warn(TR("Refusing to float split directly containing the "
                    "status display."));
            return NULL;
        }
        ns=create_splitsplit(g, split->dir);
    }

    if(ns!=NULL){
        replace(split, ns);
        split_resize((WSplit*)ns, g, PRIMN_ANY, PRIMN_ANY);
        mainloop_defer_destroy((Obj*)split);
    }
    
    return ns;
}


/*EXTL_DOC
 * Toggle floating of a split's sides at \var{split} as indicated by the 
 * parameter \var{how} (set/unset/toggle). A split of the appropriate is 
 * returned, if there was a change.
 */
EXTL_EXPORT_AS(WIonWS, set_floating)
WSplitSplit *ionws_set_floating_extl(WIonWS *ws, WSplitSplit *split, 
                                     const char *how)
{
    if(!check_node(ws, (WSplit*)split))
        return NULL;
    return ionws_set_floating(ws, split, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Toggle floating of the sides of a split containin \var{reg} as indicated 
 * by the parameters \var{how} (set/unset/toggle) and \var{dirstr}
 * (left/right/up/down/any). The new status is returned (and \code{false}
 * also on error).
 */
EXTL_EXPORT_AS(WIonWS, set_floating_at)
bool ionws_set_floating_at_extl(WIonWS *ws, WRegion *reg, const char *how,
                                const char *dirstr)
{
    WSplit *node;
    WSplitSplit *split;
    WSplitSplit *nsplit;
    int dir=SPLIT_ANY, primn=PRIMN_ANY;
    
    node=(WSplit*)get_node_check(ws, reg);
    if(node==NULL)
        return FALSE;
    
    if(dirstr!=NULL && !get_split_dir_primn(dirstr, &dir, &primn))
        return FALSE;

    while(TRUE){
        split=OBJ_CAST(node->parent, WSplitSplit);
        if(split==NULL){
            warn(TR("No suitable split here."));
            return FALSE;
        }

        if(!(primn==PRIMN_TL && node!=split->br) &&
           !(primn==PRIMN_BR && node!=split->tl) &&
           !(dir!=SPLIT_ANY && split->dir!=dir) &&
           !(OBJ_IS(split->tl, WSplitST) || OBJ_IS(split->br, WSplitST))){
            break;
        }
        
        node=(WSplit*)split;
    }
    
    nsplit=ionws_set_floating(ws, split, libtu_string_to_setparam(how));
    
    return OBJ_IS((Obj*)(nsplit==NULL ? split : nsplit), WSplitFloat);
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
        if(node==NULL){
            destroy_obj((Obj*)reg);
        }else{
            if(!ionws_managed_add(ws, reg)){
                node->reg=NULL;
                destroy_obj((Obj*)node);
                destroy_obj((Obj*)reg);
                return NULL;
            }
        }
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


/*{{{ Dynamic function table and class implementation */


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
    
    {(DynFun*)region_prepare_manage, 
     (DynFun*)ionws_prepare_manage},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)ionws_rescue_clientwins},

    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)ionws_get_rescue_pholder_for},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)ionws_get_configuration},

    {(DynFun*)region_managed_may_destroy,
     (DynFun*)ionws_managed_may_destroy},

    {(DynFun*)region_may_destroy,
     (DynFun*)ionws_may_destroy},

    {(DynFun*)region_current,
     (DynFun*)ionws_current},

    {(DynFun*)ionws_managed_add,
     (DynFun*)ionws_managed_add_default},
    
    {region_manage_stdisp,
     ionws_manage_stdisp},

    {region_unmanage_stdisp,
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


EXTL_EXPORT
IMPLCLASS(WIonWS, WGenWS, ionws_deinit, ionws_dynfuntab);

    
/*}}}*/

