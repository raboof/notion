/*
 * ion/mod_tiling/tiling.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
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
#include <ioncore/navi.h>
#include "placement.h"
#include "tiling.h"
#include "split.h"
#include "splitfloat.h"
#include "split-stdisp.h"
#include "main.h"



static WTilingIterTmp tiling_iter_default_tmp;


/*{{{ Some helper routines */


static WSplitRegion *get_node_check(WTiling *ws, WRegion *reg)
{
    WSplitRegion *node;

    if(reg==NULL)
        return NULL;
    
    node=splittree_node_of(reg);
    
    if(node==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return NULL;
    
    return node;
}


static bool check_node(WTiling *ws, WSplit *split)
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


bool tiling_fitrep(WTiling *ws, WWindow *par, const WFitParams *fp)
{
    WTilingIterTmp tmp;
    bool ok=FALSE;
    
    if(par!=NULL){
        if(!region_same_rootwin((WRegion*)ws, (WRegion*)par))
            return FALSE;
    
        region_unset_parent((WRegion*)ws);
        
        XReparentWindow(ioncore_g.dpy, ws->dummywin, 
                        par->win, fp->g.x, fp->g.y);
        
        region_set_parent((WRegion*)ws, par);
    
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


void tiling_managed_rqgeom(WTiling *ws, WRegion *mgd, 
                           const WRQGeomParams *rq,
                           WRectangle *geomret)
{
    WSplitRegion *node=get_node_check(ws, mgd);
    if(node!=NULL && ws->split_tree!=NULL)
        splittree_rqgeom((WSplit*)node, rq->flags, &rq->geom, geomret);
}


void tiling_map(WTiling *ws)
{
    REGION_MARK_MAPPED(ws);
    XMapWindow(ioncore_g.dpy, ws->dummywin);
    
    if(ws->split_tree!=NULL)
        split_map(ws->split_tree);
}


void tiling_unmap(WTiling *ws)
{
    REGION_MARK_UNMAPPED(ws);
    XUnmapWindow(ioncore_g.dpy, ws->dummywin);

    if(ws->split_tree!=NULL)
        split_unmap(ws->split_tree);
}


void tiling_fallback_focus(WTiling *ws, bool warp)
{
    region_finalise_focusing((WRegion*)ws, ws->dummywin, warp);
}


void tiling_do_set_focus(WTiling *ws, bool warp)
{
    WRegion *sub=tiling_current(ws);
    
    if(sub==NULL){
        tiling_fallback_focus(ws, warp);
        return;
    }

    region_do_set_focus(sub, warp);
}


static WTimer *restack_timer=NULL;


static void restack_handler(WTimer *tmr, Obj *obj)
{
    if(obj!=NULL){
        WTiling *ws=(WTiling*)obj;
        split_restack(ws->split_tree, ws->dummywin, Above);
    }
}


bool tiling_managed_prepare_focus(WTiling *ws, WRegion *reg, 
                                 int flags, WPrepareFocusResult *res)
{
    WSplitRegion *node; 

    if(!region_prepare_focus((WRegion*)ws, flags, res))
        return FALSE;
    
    node=get_node_check(ws, reg);
    
    if(node!=NULL && node->split.parent!=NULL)
        splitinner_mark_current(node->split.parent, &(node->split));
        
    /* WSplitSplit uses activity based stacking as required on WAutoWS,
     * so we must restack here.
     */
    if(ws->split_tree!=NULL){
        int rd=mod_tiling_raise_delay;
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
            split_restack(ws->split_tree, ws->dummywin, Above);
        }
    }

    res->reg=reg;
    res->flags=flags;
    return TRUE;
}



void tiling_restack(WTiling *ws, Window other, int mode)
{
    xwindow_restack(ws->dummywin, other, mode);
    if(ws->split_tree!=NULL)
        split_restack(ws->split_tree, ws->dummywin, Above);
}


void tiling_stacking(WTiling *ws, Window *bottomret, Window *topret)
{
    Window sbottom=None, stop=None;
    
    if(ws->split_tree!=None)
        split_stacking(ws->split_tree, &sbottom, &stop);
    
    *bottomret=ws->dummywin;
    *topret=(stop!=None ? stop : ws->dummywin);
}


Window tiling_xwindow(const WTiling *ws)
{
    return ws->dummywin;
}


/*}}}*/


/*{{{ Status display support code */


static bool regnodefilter(WSplit *split)
{
    return OBJ_IS(split, WSplitRegion);
}


void tiling_unmanage_stdisp(WTiling *ws, bool permanent, bool nofocus)
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
                                                PRIMN_ANY, PRIMN_ANY,
                                                regnodefilter);
        }
        /* Reset node_of info here so tiling_managed_remove will not
         * remove the node.
         */
        splittree_set_node_of(od, NULL);
        tiling_do_managed_remove(ws, od);
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
            tiling_fallback_focus(ws, FALSE);
    }
}


static void tiling_create_stdispnode(WTiling *ws, WRegion *stdisp, 
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


void tiling_manage_stdisp(WTiling *ws, WRegion *stdisp, 
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
        tiling_unmanage_stdisp(ws, TRUE, TRUE);
    }

    if(ws->stdispnode==NULL){
        tiling_create_stdispnode(ws, stdisp, di->pos, orientation, 
                                di->fullsize);
        if(ws->stdispnode==NULL)
            return;
    }else{
        WRegion *od=ws->stdispnode->regnode.reg;
        if(od!=NULL){
            act=REGION_IS_ACTIVE(od);
            splittree_set_node_of(od, NULL);
            tiling_managed_remove(ws, od);
            assert(ws->stdispnode->regnode.reg==NULL);
        }
        
        ws->stdispnode->fullsize=di->fullsize;
        ws->stdispnode->regnode.reg=stdisp;
        splittree_set_node_of(stdisp, &(ws->stdispnode->regnode));
    }
    
    if(!tiling_managed_add(ws, stdisp)){
        tiling_unmanage_stdisp(ws, TRUE, TRUE);
        return;
    }

    stdisp->flags|=REGION_SKIP_FOCUS;
    
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
        split_restack(ws->split_tree, ws->dummywin, Above);
    
    if(mcf && act)
        region_set_focus(stdisp);
}


/*}}}*/


/*{{{ Create/destroy */


bool tiling_managed_add_default(WTiling *ws, WRegion *reg)
{
    Window bottom=None, top=None;
    WFrame *frame;
    
    if(TILING_STDISP_OF(ws)!=reg){
        if(!ptrlist_insert_last(&(ws->managed_list), reg))
            return FALSE;
    }
    
    region_set_manager(reg, (WRegion*)ws);
    
    frame=OBJ_CAST(reg, WFrame);
    if(frame!=NULL){
        if(framemode_unalt(frame_mode(frame))!=FRAME_MODE_TILED)
            frame_set_mode(frame, FRAME_MODE_TILED);
    }
    
    if(REGION_IS_MAPPED(ws))
        region_map(reg);
    
    if(region_may_control_focus((WRegion*)ws)){
        WRegion *curr=tiling_current(ws);
        if(curr==NULL || !REGION_IS_ACTIVE(curr))
            region_warp(reg);
    }
    
    return TRUE;
}


bool tiling_managed_add(WTiling *ws, WRegion *reg)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, tiling_managed_add, ws, (ws, reg));
    return ret;
}


bool tiling_do_attach_initial(WTiling *ws, WRegion *reg)
{
    assert(ws->split_tree==NULL);
    
    ws->split_tree=(WSplit*)create_splitregion(&REGION_GEOM(reg), reg);
    if(ws->split_tree==NULL)
        return FALSE;
    
    ws->split_tree->ws_if_root=ws;
    
    if(!tiling_managed_add(ws, reg)){
        destroy_obj((Obj*)ws->split_tree);
        ws->split_tree=NULL;
        return FALSE;
    }
    
    return TRUE;
}
                                      

static WRegion *create_frame_tiling(WWindow *parent, const WFitParams *fp)
{
    return (WRegion*)create_frame(parent, fp, FRAME_MODE_TILED);
}


bool tiling_init(WTiling *ws, WWindow *parent, const WFitParams *fp, 
                WRegionSimpleCreateFn *create_frame_fn, bool ci)
{
    ws->split_tree=NULL;
    ws->create_frame_fn=(create_frame_fn 
                         ? create_frame_fn
                         : create_frame_tiling);
    ws->stdispnode=NULL;
    ws->managed_list=NULL;
    ws->batchop=FALSE;
    
    ws->dummywin=XCreateWindow(ioncore_g.dpy, parent->win,
                                fp->g.x, fp->g.y, 1, 1, 0,
                                CopyFromParent, InputOnly,
                                CopyFromParent, 0, NULL);
    if(ws->dummywin==None)
        return FALSE;

    region_init(&(ws->reg), parent, fp);

    ws->reg.flags|=(REGION_GRAB_ON_PARENT|
                    REGION_PLEASE_WARP);
    
    if(ci){
        WRegionAttachData data;
        WRegion *res;
        
        data.type=REGION_ATTACH_NEW;
        data.u.n.fn=(WRegionCreateFn*)ws->create_frame_fn;
        data.u.n.param=NULL;
    
        res=region_attach_helper((WRegion*)ws, parent, fp,
                                 (WRegionDoAttachFn*)tiling_do_attach_initial, 
                                 NULL, &data);

        if(res==NULL){
            XDestroyWindow(ioncore_g.dpy, ws->dummywin);
            return FALSE;
        }
    }
    
    XSelectInput(ioncore_g.dpy, ws->dummywin,
                 FocusChangeMask|KeyPressMask|KeyReleaseMask|
                 ButtonPressMask|ButtonReleaseMask);
    XSaveContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context,
                 (XPointer)ws);
    
    region_register(&(ws->reg));
    region_add_bindmap((WRegion*)ws, mod_tiling_tiling_bindmap);
    
    return TRUE;
}


WTiling *create_tiling(WWindow *parent, const WFitParams *fp, 
                     WRegionSimpleCreateFn *create_frame_fn, bool ci)
{
    CREATEOBJ_IMPL(WTiling, tiling, (p, parent, fp, create_frame_fn, ci));
}


WTiling *create_tiling_simple(WWindow *parent, const WFitParams *fp)
{
    return create_tiling(parent, fp, NULL, TRUE);
}


void tiling_deinit(WTiling *ws)
{
    WRegion *reg;
    WTilingIterTmp tmp;
    WMPlex *remanage_mplex=NULL;
    
    tiling_unmanage_stdisp(ws, FALSE, TRUE);

    FOR_ALL_MANAGED_BY_TILING(reg, ws, tmp){
        destroy_obj((Obj*)reg);
    }

    FOR_ALL_MANAGED_BY_TILING(reg, ws, tmp){
        assert(FALSE);
    }
    
    if(ws->split_tree!=NULL)
        destroy_obj((Obj*)(ws->split_tree));

    XDeleteContext(ioncore_g.dpy, ws->dummywin, ioncore_g.win_context);
    XDestroyWindow(ioncore_g.dpy, ws->dummywin);
    ws->dummywin=None;

    region_deinit(&(ws->reg));
}


WRegion *tiling_managed_disposeroot(WTiling *ws, WRegion *reg)
{
    WTilingIterTmp tmp;
    WRegion *mgd;
    
    if(ws->batchop)
        return reg;
    
    FOR_ALL_MANAGED_BY_TILING(mgd, ws, tmp){
        if(mgd!=TILING_STDISP_OF(ws) && mgd!=reg)
            return reg;
    }
    
    return region_disposeroot((WRegion*)ws);
}


bool tiling_rescue_clientwins(WTiling *ws, WRescueInfo *info)
{
    WTilingIterTmp tmp;
    
    ptrlist_iter_init(&tmp, ws->managed_list);
    
    return region_rescue_some_clientwins((WRegion*)ws, info,
                                         (WRegionIterator*)ptrlist_iter, 
                                         &tmp);
}


void tiling_do_managed_remove(WTiling *ws, WRegion *reg)
{
    if(TILING_STDISP_OF(ws)==reg){
        ws->stdispnode->regnode.reg=NULL;
    }else{
        ptrlist_remove(&(ws->managed_list), reg);
    }

    region_unset_manager(reg, (WRegion*)ws);
    splittree_set_node_of(reg, NULL);
}


static bool nostdispfilter(WSplit *node)
{
    return (OBJ_IS(node, WSplitRegion) && !OBJ_IS(node, WSplitST));
}


void tiling_managed_remove(WTiling *ws, WRegion *reg)
{
    bool act=REGION_IS_ACTIVE(reg);
    bool mcf=region_may_control_focus((WRegion*)ws);
    WSplitRegion *node=get_node_check(ws, reg);
    bool norestore=(OBJ_IS_BEING_DESTROYED(ws) || ws->batchop);
    WRegion *other=NULL;

    if(!norestore)
        other=tiling_do_navi_next(ws, reg, REGION_NAVI_ANY, TRUE, FALSE);
    
    tiling_do_managed_remove(ws, reg);

    if(node==(WSplitRegion*)(ws->stdispnode))
        ws->stdispnode=NULL;
    
    if(node!=NULL){
        bool reused=FALSE;
        
        if(other==NULL && !norestore){
            WWindow *par=REGION_PARENT(ws);
            WFitParams fp;
            
            assert(par!=NULL);
            
            fp.g=node->split.geom;
            fp.mode=REGION_FIT_EXACT;
            
            other=(ws->create_frame_fn)(par, &fp);
            
            if(other!=NULL){
                node->reg=other;
                splittree_set_node_of(other, node);
                tiling_managed_add(ws, other);
                reused=TRUE;
            }else{
                warn(TR("Tiling in useless state."));
            }
        }
        
        if(!reused)
            splittree_remove((WSplit*)node, (!norestore && other!=NULL));
    }
    
    if(!norestore && other!=NULL && act && mcf)
        region_warp(other);
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


WPHolder *tiling_get_rescue_pholder_for(WTiling *ws, WRegion *mgd)
{
    WSplit *node=(WSplit*)get_node_check(ws, mgd);
    WPHolder *ph;
    
    find_ph_result=NULL;
    find_ph_param=mgd;
    
    if(node==NULL){
        if(ws->split_tree!=NULL){
            split_current_todir(ws->split_tree, PRIMN_ANY, PRIMN_ANY, 
                                find_ph);
        }
    }else{
        while(node!=NULL){
            split_nextto(node, PRIMN_ANY, PRIMN_ANY, find_ph);
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


/*{{{ Navigation */


static void navi_to_primn(WRegionNavi nh, WPrimn *hprimn, WPrimn *vprimn, 
                          WPrimn choice)
{
    /* choice should be PRIMN_ANY or PRIMN_NONE */
    
    switch(nh){
    case REGION_NAVI_BEG:
        *vprimn=PRIMN_TL;
        *hprimn=PRIMN_TL;
        break;
        
    case REGION_NAVI_END:
        *vprimn=PRIMN_BR;
        *hprimn=PRIMN_BR;
        break;
        
    case REGION_NAVI_LEFT:
        *hprimn=PRIMN_TL;
        *vprimn=choice;
        break;
        
    case REGION_NAVI_RIGHT:
        *hprimn=PRIMN_BR;
        *vprimn=choice;
        break;
        
    case REGION_NAVI_TOP:
        *hprimn=choice;
        *vprimn=PRIMN_TL;
        break;
        
    case REGION_NAVI_BOTTOM:
        *hprimn=choice;
        *vprimn=PRIMN_BR;
        break;
        
    default:
    case REGION_NAVI_ANY:
        *hprimn=PRIMN_ANY;
        *vprimn=PRIMN_ANY;
        break;
    }
}


static WRegion *node_reg(WSplit *node)
{
    WSplitRegion *rnode=OBJ_CAST(node, WSplitRegion);
    return (rnode!=NULL ? rnode->reg : NULL);
}

    
WRegion *tiling_do_navi_next(WTiling *ws, WRegion *reg, 
                             WRegionNavi nh, bool nowrap,
                             bool any)
{
    WSplitFilter *filter=(any ? NULL : nostdispfilter);
    WPrimn hprimn, vprimn;
    WRegion *nxt=NULL;

    navi_to_primn(nh, &hprimn, &vprimn, PRIMN_NONE);
    
    if(reg==NULL)
        reg=tiling_current(ws);
    
    if(reg!=NULL){
        WSplitRegion *node=get_node_check(ws, reg);
        if(node!=NULL){
            nxt=node_reg(split_nextto((WSplit*)node, hprimn, vprimn, 
                                      filter));
        }
    }
    
    if(nxt==NULL && !nowrap){
        nxt=node_reg(split_current_todir(ws->split_tree, 
                                         primn_none2any(primn_invert(hprimn)),
                                         primn_none2any(primn_invert(vprimn)),
                                         filter));
    }
    
    return nxt;
}


WRegion *tiling_do_navi_first(WTiling *ws, WRegionNavi nh, bool any)
{
    WSplitFilter *filter=(any ? NULL : nostdispfilter);
    WPrimn hprimn, vprimn;
    
    navi_to_primn(nh, &hprimn, &vprimn, PRIMN_ANY);

    return node_reg(split_current_todir(ws->split_tree, 
                                        hprimn, vprimn, filter));
}


WRegion *tiling_navi_next(WTiling *ws, WRegion *reg, 
                          WRegionNavi nh, WRegionNaviData *data)
{
    WRegion *nxt=tiling_do_navi_next(ws, reg, nh, TRUE, FALSE);
    
    return region_navi_cont(&ws->reg, nxt, data);
}


WRegion *tiling_navi_first(WTiling *ws, WRegionNavi nh,
                           WRegionNaviData *data)
{
    WRegion *reg=tiling_do_navi_first(ws, nh, FALSE);
    
    return region_navi_cont(&ws->reg, reg, data);
}


/*}}}*/


/*{{{ Split/unsplit */


static bool get_split_dir_primn(const char *str, int *dir, int *primn)
{
    WPrimn hprimn, vprimn;
    WRegionNavi nh;

    if(!ioncore_string_to_navi(str, &nh))
        return FALSE;
    
    navi_to_primn(nh, &hprimn, &vprimn, PRIMN_NONE);
    
    if(hprimn==PRIMN_NONE){
        *dir=SPLIT_VERTICAL;
        *primn=vprimn;
    }else if(vprimn==PRIMN_NONE){
        *dir=SPLIT_HORIZONTAL;
        *primn=hprimn;
    }else{
        warn(TR("Invalid direction"));
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


static WFrame *tiling_do_split(WTiling *ws, WSplit *node, 
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
        split_restack(ws->split_tree, ws->dummywin, Above);

    newframe=OBJ_CAST(nnode->reg, WFrame);
    assert(newframe!=NULL);

    if(!tiling_managed_add(ws, nnode->reg)){
        nnode->reg=NULL;
        destroy_obj((Obj*)nnode);
        destroy_obj((Obj*)newframe);
        return NULL;
    }
    
    return newframe;
}


/*EXTL_DOC
 * Create a new frame on \var{ws} \codestr{above}, \codestr{below}
 * \codestr{left} of, or \codestr{right} of \var{node} as indicated
 *  by \var{dirstr}. If \var{dirstr} is  prefixed with 
 * \codestr{floating:} a floating split is created.
 */
EXTL_EXPORT_MEMBER
WFrame *tiling_split(WTiling *ws, WSplit *node, const char *dirstr)
{
    if(!check_node(ws, node))
        return NULL;
    
    return tiling_do_split(ws, node, dirstr,
                          SPLIT_MINS, SPLIT_MINS);
}


/*EXTL_DOC
 * Same as \fnref{WTiling.split} at the root of the split tree.
 */
EXTL_EXPORT_MEMBER
WFrame *tiling_split_top(WTiling *ws, const char *dirstr)
{
    return tiling_do_split(ws, ws->split_tree, dirstr, 
                          SPLIT_MINS, SPLIT_MINS);
}


/*EXTL_DOC
 * Split \var{frame} creating a new frame to direction \var{dirstr}
 * (one of \codestr{left}, \codestr{right}, \codestr{top} or 
 * \codestr{bottom}) of \var{frame}.
 * If \var{attach_current} is set, the region currently displayed in
 * \var{frame}, if any, is moved to thenew frame.
 * If \var{dirstr} is prefixed with \codestr{floating:}, a floating
 * split is created.
 */
EXTL_EXPORT_MEMBER
WFrame *tiling_split_at(WTiling *ws, WFrame *frame, const char *dirstr, 
                       bool attach_current)
{
    WRegion *curr;
    WSplitRegion *node;
    WFrame *newframe;
    
    if(frame==NULL)
        return NULL;
    
    node=get_node_check(ws, (WRegion*)frame);

    newframe=tiling_do_split(ws, (WSplit*)node, dirstr, 
                            region_min_w((WRegion*)frame),
                            region_min_h((WRegion*)frame));

    if(newframe==NULL)
        return NULL;

    curr=mplex_mx_current(&(frame->mplex));
    
    if(attach_current && curr!=NULL)
        mplex_attach_simple(&(newframe->mplex), curr, MPLEX_ATTACH_SWITCHTO);
    
    if(region_may_control_focus((WRegion*)frame))
        region_goto((WRegion*)newframe);

    return newframe;
}


/*EXTL_DOC
 * Try to relocate regions managed by \var{reg} to another frame
 * and, if possible, destroy it.
 */
EXTL_EXPORT_MEMBER
void tiling_unsplit_at(WTiling *ws, WRegion *reg)
{
    WPHolder *ph;
    
    if(reg==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return;
    
    ph=region_get_rescue_pholder_for((WRegion*)ws, reg);
    
    if(ph!=NULL){
        region_rescue(reg, ph, REGION_RESCUE_NODEEP|REGION_RESCUE_PHFLAGS_OK);
        destroy_obj((Obj*)ph);
    }
    
    region_defer_rqdispose(reg);
}


/*}}}*/


/*{{{ Navigation etc. exports */


WRegion *tiling_current(WTiling *ws)
{
    WSplitRegion *node=NULL;
    if(ws->split_tree!=NULL){
        node=(WSplitRegion*)split_current_todir(ws->split_tree, 
                                                PRIMN_ANY, PRIMN_ANY, NULL);
    }
    return (node ? node->reg : NULL);
}


/*EXTL_DOC
 * Iterate over managed regions of \var{ws} until \var{iterfn} returns
 * \code{false}.
 * The function is called in protected mode.
 * This routine returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool tiling_managed_i(WTiling *ws, ExtlFn iterfn)
{
    PtrListIterTmp tmp;
    
    ptrlist_iter_init(&tmp, ws->managed_list);
    
    return extl_iter_objlist_(iterfn, (ObjIterator*)ptrlist_iter, &tmp);
}


/*EXTL_DOC
 * Returns the root of the split tree.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplit *tiling_split_tree(WTiling *ws)
{
    return ws->split_tree;
}


/*EXTL_DOC
 * Return the most previously active region next to \var{reg} in
 * direction \var{dirstr} (\codestr{left}, \codestr{right}, \codestr{up},
 * or \codestr{down}). The region \var{reg}
 * must be managed by \var{ws}. If \var{any} is not set, the status display
 * is not considered.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *tiling_nextto(WTiling *ws, WRegion *reg, const char *dirstr,
                       bool any)
{
    WRegionNavi nh;
    
    if(!ioncore_string_to_navi(dirstr, &nh))
        return NULL;
    
    return tiling_do_navi_next(ws, reg, nh, FALSE, any);
}


/*EXTL_DOC
 * Return the most previously active region on \var{ws} with no
 * other regions next to it in  direction \var{dirstr} 
 * (\codestr{left}, \codestr{right}, \codestr{up}, or \codestr{down}). 
 * If \var{any} is not set, the status display is not considered.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *tiling_farthest(WTiling *ws, const char *dirstr, bool any)
{
    WRegionNavi nh;
    
    if(!ioncore_string_to_navi(dirstr, &nh))
        return NULL;
    
    return tiling_do_navi_first(ws, nh, any);
}


/*EXTL_DOC
 * For region \var{reg} managed by \var{ws} return the \type{WSplit}
 * a leaf of which \var{reg} is.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplitRegion *tiling_node_of(WTiling *ws, WRegion *reg)
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


static WSplitSplit *get_at_split(WTiling *ws, WRegion *reg)
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
bool iowns_flip_at(WTiling *ws, WRegion *reg)
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
bool iowns_transpose_at(WTiling *ws, WRegion *reg)
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


WSplitSplit *tiling_set_floating(WTiling *ws, WSplitSplit *split, int sp)
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
 * parameter \var{how} (\codestr{set}, \codestr{unset}, or \codestr{toggle}).
 * A split of the appropriate is returned, if there was a change.
 */
EXTL_EXPORT_AS(WTiling, set_floating)
WSplitSplit *tiling_set_floating_extl(WTiling *ws, WSplitSplit *split, 
                                     const char *how)
{
    if(!check_node(ws, (WSplit*)split))
        return NULL;
    return tiling_set_floating(ws, split, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Toggle floating of the sides of a split containin \var{reg} as indicated 
 * by the parameters \var{how} (\codestr{set}, \codestr{unset}, or 
 * \codestr{toggle}) and \var{dirstr} (\codestr{left}, \codestr{right}, 
 * \codestr{up}, or \codestr{down}). The new status is returned 
 * (and \code{false} also on error).
 */
EXTL_EXPORT_AS(WTiling, set_floating_at)
bool tiling_set_floating_at_extl(WTiling *ws, WRegion *reg, const char *how,
                                const char *dirstr)
{
    WPrimn hprimn=PRIMN_ANY, vprimn=PRIMN_ANY;
    WSplitSplit *split, *nsplit;
    WSplit *node;
    
    node=(WSplit*)get_node_check(ws, reg);
    if(node==NULL)
        return FALSE;
    
    
    if(dirstr!=NULL){
        WRegionNavi nh;
        
        if(!ioncore_string_to_navi(dirstr, &nh))
            return FALSE;
    
        navi_to_primn(nh, &hprimn, &vprimn, PRIMN_NONE);
    }

    while(TRUE){
        split=OBJ_CAST(node->parent, WSplitSplit);
        if(split==NULL){
            warn(TR("No suitable split here."));
            return FALSE;
        }

        if(!OBJ_IS(split->tl, WSplitST) && !OBJ_IS(split->br, WSplitST)){
            WPrimn tmp=(split->dir==SPLIT_VERTICAL ? vprimn : hprimn);
            if(tmp==PRIMN_ANY 
               || (node==split->tl && tmp==PRIMN_BR)
               || (node==split->br && tmp==PRIMN_TL)){
                break;
            }
        }
        
        node=(WSplit*)split;
    }
    
    nsplit=tiling_set_floating(ws, split, libtu_string_to_setparam(how));
    
    return OBJ_IS((Obj*)(nsplit==NULL ? split : nsplit), WSplitFloat);
}


/*}}}*/


/*{{{ Save */


ExtlTab tiling_get_configuration(WTiling *ws)
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


WSplit *load_splitst(WTiling *ws, const WRectangle *geom, ExtlTab tab)
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


static bool do_attach(WTiling *ws, WRegion *reg, void *p)
{
    WSplitRegion *node=create_splitregion(&REGION_GEOM(reg), reg);
    
    if(node==NULL)
        return FALSE;
            
    if(!tiling_managed_add(ws, reg)){
        node->reg=NULL;
        destroy_obj((Obj*)node);
        return FALSE;
    }
    
    *(WSplitRegion**)p=node;
    
    return TRUE;
}


WSplit *load_splitregion(WTiling *ws, const WRectangle *geom, ExtlTab tab)
{
    WWindow *par=REGION_PARENT(ws);
    WRegionAttachData data;
    WSplit *node=NULL;
    WFitParams fp;
    ExtlTab rt;
    
    if(!extl_table_gets_t(tab, "regparams", &rt)){
        warn(TR("Missing region parameters."));
        return NULL;
    }
    
    data.type=REGION_ATTACH_LOAD;
    data.u.tab=rt;
    
    assert(par!=NULL);
    fp.g=*geom;
    fp.mode=REGION_FIT_EXACT;
    
    region_attach_helper((WRegion*)ws, par, &fp, 
                         (WRegionDoAttachFn*)do_attach, &node, &data);

    extl_unref_table(rt);
    
    return node;
}


#define MINS 1

WSplit *load_splitsplit(WTiling *ws, const WRectangle *geom, ExtlTab tab)
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
        tl=tiling_load_node(ws, &geom2, subtab);
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
        br=tiling_load_node(ws, &geom2, subtab);
        extl_unref_table(subtab);
    }
    
    if(tl==NULL || br==NULL){
        /* PRIMN_TL/BR instead of ANY because of stdisp. */
        destroy_obj((Obj*)split);
        if(tl!=NULL){
            split_do_resize(tl, geom, PRIMN_BR, PRIMN_BR, FALSE);
            return tl;
        }
        if(br!=NULL){
            split_do_resize(br, geom, PRIMN_TL, PRIMN_TL, FALSE);
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


WSplit *tiling_load_node_default(WTiling *ws, const WRectangle *geom, 
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


WSplit *tiling_load_node(WTiling *ws, const WRectangle *geom, ExtlTab tab)
{
    WSplit *ret=NULL;
    CALL_DYN_RET(ret, WSplit*, tiling_load_node, ws, (ws, geom, tab));
    return ret;
}



WRegion *tiling_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WTiling *ws;
    ExtlTab treetab;
    bool ci=TRUE;

    if(extl_table_gets_t(tab, "split_tree", &treetab))
        ci=FALSE;
    
    ws=create_tiling(par, fp, NULL, ci);
    
    if(ws==NULL){
        if(!ci)
            extl_unref_table(treetab);
        return NULL;
    }

    if(!ci){
        ws->split_tree=tiling_load_node(ws, &REGION_GEOM(ws), treetab);
        extl_unref_table(treetab);
    }
    
    if(ws->split_tree==NULL){
        warn(TR("The workspace is empty."));
        destroy_obj((Obj*)ws);
        return NULL;
    }
    
    ws->split_tree->ws_if_root=ws;
    split_restack(ws->split_tree, ws->dummywin, Above);
    
    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab tiling_dynfuntab[]={
    {region_map, 
     tiling_map},
    
    {region_unmap, 
     tiling_unmap},
    
    {region_do_set_focus, 
     tiling_do_set_focus},
    
    {(DynFun*)region_fitrep,
     (DynFun*)tiling_fitrep},
    
    {region_managed_rqgeom, 
     tiling_managed_rqgeom},
    
    {region_managed_remove, 
     tiling_managed_remove},
    
    {(DynFun*)region_managed_prepare_focus,
     (DynFun*)tiling_managed_prepare_focus},
    
    {(DynFun*)region_prepare_manage, 
     (DynFun*)tiling_prepare_manage},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)tiling_rescue_clientwins},

    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)tiling_get_rescue_pholder_for},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)tiling_get_configuration},

    {(DynFun*)region_managed_disposeroot,
     (DynFun*)tiling_managed_disposeroot},

    {(DynFun*)region_current,
     (DynFun*)tiling_current},

    {(DynFun*)tiling_managed_add,
     (DynFun*)tiling_managed_add_default},
    
    {region_manage_stdisp,
     tiling_manage_stdisp},

    {region_unmanage_stdisp,
     tiling_unmanage_stdisp},
    
    {(DynFun*)tiling_load_node,
     (DynFun*)tiling_load_node_default},
            
    {region_restack,
     tiling_restack},

    {region_stacking,
     tiling_stacking},
    
    {(DynFun*)region_navi_first,
     (DynFun*)tiling_navi_first},
    
    {(DynFun*)region_navi_next,
     (DynFun*)tiling_navi_next},
    
    {(DynFun*)region_xwindow,
     (DynFun*)tiling_xwindow},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WTiling, WRegion, tiling_deinit, tiling_dynfuntab);

    
/*}}}*/

