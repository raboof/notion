/*
 * ion/panews/splitext.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <limits.h>
#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/rootwin.h>
#include <ioncore/xwindow.h>
#include <ioncore/window.h>
#include <mod_ionws/split.h>
#include "splitext.h"
#include "unusedwin.h"


#define GEOM(X) (((WSplit*)(X))->geom)


/*{{{ Init/deinit */


bool splitunused_init(WSplitUnused *split, const WRectangle *geom,
                      WPaneWS *ws)
{
    WWindow *par=REGION_PARENT(ws);
    WUnusedWin *uwin;
    WFitParams fp;

    assert(par!=NULL);
    
    fp.g=*geom;
    fp.mode=REGION_FIT_EXACT;
    
    uwin=create_unusedwin(par, &fp);
    
    if(uwin==NULL)
        return FALSE;
    
    if(!splitregion_init(&(split->regnode), geom, (WRegion*)uwin)){
        destroy_obj((Obj*)uwin);
        return FALSE;
    }
    
    if(!ionws_managed_add(&ws->ionws, (WRegion*)uwin)){
        split->regnode.reg=NULL;
        destroy_obj((Obj*)uwin);
        return FALSE;
    }
    
    return TRUE;
}


WSplitUnused *create_splitunused(const WRectangle *geom, WPaneWS *ws)
{
    CREATEOBJ_IMPL(WSplitUnused, splitunused, (p, geom, ws));
}


bool splitpane_init(WSplitPane *pane, const WRectangle *geom, WSplit *cnt)
{
    pane->contents=cnt;
    pane->marker=NULL;
    
    if(!splitinner_init(&(pane->isplit), geom))
        return FALSE;

    return TRUE;
}


WSplitPane *create_splitpane(const WRectangle *geom, WSplit *cnt)
{
    CREATEOBJ_IMPL(WSplitPane, splitpane, (p, geom, cnt));
}


void splitunused_deinit(WSplitUnused *split)
{
    if(split->regnode.reg!=NULL){
        destroy_obj((Obj*)split->regnode.reg);
        split->regnode.reg=NULL;
    }
    
    splitregion_deinit(&(split->regnode));
}


void splitpane_deinit(WSplitPane *split)
{
    if(split->contents!=NULL){
        WSplit *tmp=split->contents;
        split->contents=NULL;
        tmp->parent=NULL;
        destroy_obj((Obj*)tmp);
    }
    splitinner_deinit(&(split->isplit));
}


/*}}}*/


/*{{{ X window handling */


static void splitpane_stacking(WSplitPane *pane, 
                               Window *bottomret, Window *topret)
{

    *bottomret=None;
    *topret=None;
    
    if(pane->contents!=NULL)
        split_stacking(pane->contents, bottomret, topret);
}


static void splitpane_restack(WSplitPane *pane, Window other, int mode)
{
    if(pane->contents!=None)
        split_restack(pane->contents, other, mode);
}


static void stack_restack_reg(WRegion *reg, Window *other, int *mode)
{
    Window b=None, t=None;
    
    if(reg!=NULL){
        region_restack(reg, *other, *mode);
        region_stacking(reg, &b, &t);
        if(t!=None){
            *other=t;
            *mode=Above;
        }
    }
}


static void stack_restack_split(WSplit *split, Window *other, int *mode)
{
    Window b=None, t=None;
    
    if(split!=NULL){
        split_restack(split, *other, *mode);
        split_stacking(split, &b, &t);
        if(t!=None){
            *other=t;
            *mode=Above;
        }
    }
}


static void splitpane_reparent(WSplitPane *pane, WWindow *target)
{
    if(pane->contents!=NULL)
        split_reparent(pane->contents, target);
}


static void reparentreg(WRegion *reg, WWindow *target)
{
    WRectangle g=REGION_GEOM(reg);
    region_reparent(reg, target, &g, REGION_FIT_EXACT);
}


/*}}}*/


/*{{{ Geometry */


static void set_unused_bounds(WSplit *node)
{
    node->min_w=0;
    node->min_h=0;
    node->max_w=INT_MAX;
    node->max_h=INT_MAX;
    node->unused_w=node->geom.w;
    node->unused_h=node->geom.h;
}


static void copy_bounds(WSplit *dst, const WSplit *src)
{
    dst->min_w=src->min_w;
    dst->min_h=src->min_h;
    dst->max_w=src->max_w;
    dst->max_h=src->max_h;
    dst->unused_w=src->unused_w;
    dst->unused_h=src->unused_h;
}


static void splitunused_update_bounds(WSplitUnused *node, bool recursive)
{
    set_unused_bounds((WSplit*)node);
}


static void splitpane_update_bounds(WSplitPane *node, bool recursive)
{
    if(node->contents!=NULL){
        if(recursive)
            split_update_bounds(node->contents, recursive);
        copy_bounds((WSplit*)node, node->contents);
    }else{
        set_unused_bounds((WSplit*)node);
    }
}


static int infadd(int x, int y)
{
    return ((x==INT_MAX || y==INT_MAX) ? INT_MAX : (x+y));
}


static void splitpane_do_resize(WSplitPane *pane, const WRectangle *ng, 
                                int hprimn, int vprimn, bool transpose)
{
    if(transpose && pane->marker!=NULL){
        char *growdir=strchr(pane->marker, ':');
        if(growdir!=NULL){
            const char *newdir=NULL;
            growdir++;
            
            if(strcmp(growdir, "right")==0)
                newdir="down";
            else if(strcmp(growdir, "left")==0)
                newdir="up";
            if(strcmp(growdir, "down")==0)
                newdir="right";
            else if(strcmp(growdir, "up")==0)
                newdir="left";
            
            if(newdir!=NULL){
                char *newmarker=NULL;
                *growdir='\0';
                libtu_asprintf(&newmarker, "%s:%s", pane->marker, newdir);
                if(newmarker==NULL){
                    *growdir=':';
                }else{
                    free(pane->marker);
                    pane->marker=newmarker;
                }
            }
        }
        
    }
    
    ((WSplit*)pane)->geom=*ng;
    
    if(pane->contents!=NULL)
        split_do_resize(pane->contents, ng, hprimn, vprimn, transpose);
}


static void splitpane_do_rqsize(WSplitPane *pane, WSplit *node, 
                                RootwardAmount *ha, RootwardAmount *va, 
                                WRectangle *rg, bool tryonly)
{
    WSplitInner *par=((WSplit*)pane)->parent;
    
    if(par!=NULL){
        splitinner_do_rqsize(par, (WSplit*)pane, ha, va, rg, tryonly);
        if(!tryonly)
            ((WSplit*)pane)->geom=*rg;
    }else{
        *rg=GEOM(pane);
    }
}


/*}}}*/


/*{{{ Tree manipulation */


static void splitpane_replace(WSplitPane *pane, WSplit *child, WSplit *what)
{
    assert(child==pane->contents && what!=NULL);
    
    child->parent=NULL;
    pane->contents=what;
    what->parent=(WSplitInner*)pane;
    what->ws_if_root=NULL; /* May not be needed */
}


static WPaneWS *find_ws(WSplit *split)
{
    if(split->parent!=NULL)
        return find_ws((WSplit*)split->parent);
    
    if(split->ws_if_root!=NULL)
        return OBJ_CAST(split->ws_if_root, WPaneWS);
    
    return NULL;
}


static void splitpane_remove(WSplitPane *pane, WSplit *child, 
                             bool reclaim_space)
{
    WSplitInner *parent=((WSplit*)pane)->parent;
    WSplitUnused *un;
    WPaneWS *ws=find_ws((WSplit*)pane);
    
    assert(child==pane->contents);
    
    pane->contents=NULL;
    child->parent=NULL;

    if(ws!=NULL
       && !OBJ_IS_BEING_DESTROYED(ws)
       && !OBJ_IS_BEING_DESTROYED(pane)){
        pane->contents=(WSplit*)create_splitunused(&GEOM(pane), ws);
        if(pane->contents!=NULL){
            pane->contents->parent=(WSplitInner*)pane;
            return;
        }
    }
    
    if(parent!=NULL)
        splitinner_remove(parent, (WSplit*)pane, reclaim_space);
    else
        splittree_changeroot((WSplit*)pane, NULL);
    
    destroy_obj((Obj*)pane);
}


/*}}}*/


/*{{{ Tree traversal */


static bool filter_any(WSplit *split)
{
    return OBJ_IS(split, WSplitRegion);
}


static bool filter_no_unused(WSplit *split)
{
    return (OBJ_IS(split, WSplitRegion)
            && !OBJ_IS(split, WSplitUnused));
}


static bool filter_no_stdisp(WSplit *split)
{
    return (OBJ_IS(split, WSplitRegion)
            && !OBJ_IS(split, WSplitST));
}


static bool filter_no_stdisp_unused(WSplit *split)
{
    return (OBJ_IS(split, WSplitRegion)
            && !OBJ_IS(split, WSplitST)
            && !OBJ_IS(split, WSplitUnused));
            
}


static WSplit *splitpane_current_todir(WSplitPane *pane, int dir, int primn,
                                       WSplitFilter *filter)
{
    WSplit *ret=NULL;
    
    if(pane->contents==NULL)
        return NULL;
    
    /* Try non-unused first */
    if(filter==filter_no_stdisp){
        ret=split_current_todir(pane->contents, dir, primn, 
                                filter_no_stdisp_unused);
    }else if(filter==filter_any){
        ret=split_current_todir(pane->contents, dir, primn, 
                                filter_no_unused);
    }
    
    if(ret==NULL)
        ret=split_current_todir(pane->contents, dir, primn, filter);
    
    return ret;
}


static void splitpane_forall(WSplitPane *pane, WSplitFn *fn)
{
    if(pane->contents!=NULL)
        fn(pane->contents);
}


static WSplit *splitpane_current(WSplitPane *pane)
{
    return pane->contents;
}


static WSplitRegion *get_node_check(WPaneWS *ws, WRegion *reg)
{
    WSplitRegion *node;

    if(reg==NULL)
        return NULL;
    
    node=splittree_node_of(reg);
    
    if(node==NULL || REGION_MANAGER(reg)!=(WRegion*)ws)
        return NULL;
    
    return node;
}


static WSplitRegion *do_get_nextto(WSplit *node, int dir, int primn, 
                                   bool any, bool paneonly)
{
    WSplitFilter *filter=(any ? filter_no_unused : filter_no_stdisp_unused);
    WSplit *nextto=NULL;

    while(node->parent!=NULL){
        if(OBJ_IS(node, WSplitPane)){
            if(paneonly)
                break;
            filter=(any ? filter_any : filter_no_stdisp);
        }
        nextto=splitinner_nextto(node->parent, node, dir, primn, filter);
        if(nextto!=NULL)
            break;
        node=(WSplit*)(node->parent);
    }
    
    if(OBJ_IS(nextto, WSplitRegion))
        return (WSplitRegion*)nextto;
    return NULL;
}


WRegion *panews_do_get_nextto(WPaneWS *ws, WRegion *reg,
                              int dir, int primn, bool any)
{
    WSplitRegion *node=get_node_check(ws, reg), *nextto=NULL;

    if(node==NULL)
        return NULL;
    
    nextto=do_get_nextto((WSplit*)node, dir, primn, TRUE, FALSE);
    
    if(nextto!=NULL)
        return nextto->reg;

    return NULL;
}

WRegion *panews_do_get_farthest(WPaneWS *ws,
                                int dir, int primn, bool any)
{
    WSplitFilter *filter=(any ? filter_any : filter_no_stdisp);
    WSplit *node=NULL;
    if(ws->ionws.split_tree!=NULL)
        node=split_current_todir(ws->ionws.split_tree, dir, primn, filter);
    if(node!=NULL && OBJ_IS(node, WSplitRegion))
        return ((WSplitRegion*)node)->reg;
    return NULL;
}


WSplitRegion *split_tree_find_region_in_pane_of(WSplit *node)
{
    return do_get_nextto(node, SPLIT_ANY, PRIMN_ANY, FALSE, TRUE);
}


/*}}}*/


/*{{{ Markers and other exports */


/*EXTL_DOC
 * Get marker.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
const char *splitpane_marker(WSplitPane *pane)
{
    return pane->marker;
}


/*EXTL_DOC
 * Set marker.
 */
EXTL_EXPORT_MEMBER
bool splitpane_set_marker(WSplitPane *pane, const char *s)
{
    char *s2=NULL;
    
    if(s!=NULL){
        s2=scopy(s);
        if(s2==NULL)
            return FALSE;
    }
    
    if(pane->marker==NULL)
        free(pane->marker);
    
    pane->marker=s2;
    
    return TRUE;
}


/*EXTL_DOC
 * Get root of contained sub-split tree.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplit *splitpane_contents(WSplitPane *pane)
{
    return pane->contents;
}


/*}}}*/


/*{{{ Save support */


static bool splitunused_get_config(WSplitUnused *node, ExtlTab *ret)
{
    *ret=split_base_config((WSplit*)node);
    return TRUE;
}


static bool splitpane_get_config(WSplitPane *pane, ExtlTab *ret)
{
    *ret=split_base_config((WSplit*)pane);
    
    if(pane->contents!=NULL){
        ExtlTab t;
        if(!split_get_config(pane->contents, &t)){
            extl_unref_table(*ret);
            return FALSE;
        }
        extl_table_sets_t(*ret, "contents", t);
        extl_unref_table(t);
    }
    
    extl_table_sets_s(*ret, "marker", pane->marker);

    return TRUE;
}


/*}}}*/


/*{{{ The classes */


static DynFunTab splitunused_dynfuntab[]={
    {split_update_bounds, splitunused_update_bounds},
    {(DynFun*)split_get_config, (DynFun*)splitunused_get_config},
    END_DYNFUNTAB,
};


static DynFunTab splitpane_dynfuntab[]={
    {split_update_bounds, splitpane_update_bounds},
    {split_do_resize, splitpane_do_resize},
    {splitinner_do_rqsize, splitpane_do_rqsize},
    {splitinner_replace, splitpane_replace},
    {splitinner_remove, splitpane_remove},
    {(DynFun*)split_current_todir, (DynFun*)splitpane_current_todir},
    {(DynFun*)splitinner_current, (DynFun*)splitpane_current},
    {(DynFun*)split_get_config, (DynFun*)splitpane_get_config},
    {splitinner_forall, splitpane_forall},
    {split_stacking, splitpane_stacking},
    {split_restack, splitpane_restack},
    {split_reparent, splitpane_reparent},
    END_DYNFUNTAB,
};


EXTL_EXPORT
IMPLCLASS(WSplitUnused, WSplitRegion, splitunused_deinit, splitunused_dynfuntab);

EXTL_EXPORT
IMPLCLASS(WSplitPane, WSplitInner, splitpane_deinit, splitpane_dynfuntab);


/*}}}*/

