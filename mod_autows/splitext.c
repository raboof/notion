/*
 * ion/autows/splitext.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <libtu/objp.h>
#include <ioncore/common.h>
#include <mod_ionws/split.h>
#include "splitext.h"


/*{{{ Init/deinit */


bool splitunused_init(WSplitUnused *split, const WRectangle *geom)
{
    return split_init(&(split->split), geom);
}


WSplitUnused *create_splitunused(const WRectangle *geom)
{
    CREATEOBJ_IMPL(WSplitUnused, splitunused, (p, geom));
}


bool splitpane_init(WSplitPane *split, const WRectangle *geom, WSplit *cnt)
{
    split->contents=cnt;
    split->marker=NULL;
    
    return splitinner_init(&(split->isplit), geom);
}


WSplitPane *create_splitpane(const WRectangle *geom, WSplit *cnt)
{
    CREATEOBJ_IMPL(WSplitPane, splitpane, (p, geom, cnt));
}


void splitunused_deinit(WSplitUnused *split)
{
    split_deinit(&(split->split));
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
        split_update_bounds(node->contents, recursive);
        copy_bounds((WSplit*)node, node->contents);
    }else{
        set_unused_bounds((WSplit*)node);
    }
}


static void splitpane_do_resize(WSplitPane *node, const WRectangle *ng, 
                                int hprimn, int vprimn, bool transpose)
{
    ((WSplit*)node)->geom=*ng;
    
    if(node->contents!=NULL)
        split_do_resize(node->contents, ng, hprimn, vprimn, FALSE);
}


static void splitpane_do_rqsize(WSplitPane *pane, WSplit *node, 
                                RootwardAmount *ha, RootwardAmount *va, 
                                WRectangle *rg, bool tryonly)
{
    WSplitInner *par=((WSplit*)pane)->parent;
    
    assert(node==pane->contents && pane->contents!=NULL);
    
    if(par==NULL){
        *rg=((WSplit*)pane)->geom;
    }else{
        splitinner_do_rqsize(par, (WSplit*)pane, ha, va, rg, tryonly);
        if(!tryonly)
            ((WSplit*)pane)->geom=*rg;
    }
}
    

/*}}}*/


/*{{{ Tree manipulation */


static void splitpane_replace(WSplitPane *pane, WSplit *child, WSplit *what)
{
    assert(child==pane->contents && what!=NULL);
    
    pane->contents=what;
    what->parent=(WSplitInner*)pane;
    child->parent=NULL;
}


static void splitpane_remove(WSplitPane *pane, WSplit *child, WSplit **root,
                             bool reclaim_space)
{
    WSplitUnused *un;
    
    assert(child==pane->contents);
    pane->contents=NULL;
    child->parent=NULL;
    
    un=create_splitunused(&(child->geom));
    if(un!=NULL){
        pane->contents=(WSplit*)un;
        ((WSplit*)un)->parent=(WSplitInner*)pane;
    }else{
        WSplitInner *parent=((WSplit*)pane)->parent;
        WARN_FUNC("Could not create a WSplitUnused for empty WSplitPane. "
                  "Destroying.");
        if(parent!=NULL){
            splitinner_remove(parent, (WSplit*)pane, root, reclaim_space);
        }else{
            assert(*root==(WSplit*)pane);
            *root=NULL;
        }
        destroy_obj((Obj*)pane);
    }
}


/*}}}*/


/*{{{ Tree traversal */


static WSplit *splitpane_current_todir(WSplitPane *pane, int dir, int primn,
                                       WSplitFilter *filter)
{
    if(pane->contents==NULL)
        return NULL;
    else
        return split_current_todir(pane->contents, dir, primn, filter);
}


static WSplit *splitpane_nextto(WSplitPane *pane, WSplit *child,
                                int dir, int primn, WSplitFilter *filter)
{
    WSplitInner *parent=((WSplit*)pane)->parent;
    
    if(parent==NULL)
        return NULL;
    else
        return splitinner_nextto(parent, (WSplit*)pane, dir, primn, filter);
}


/*}}}*/


/*{{{ Markers and other exports */


/*EXTL_DOC
 * Get marker.
 */
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
        if(s2==NULL){
            warn_err();
            return FALSE;
        }
    }
    
    if(pane->marker==NULL)
        free(pane->marker);
    
    pane->marker=s2;
    
    return TRUE;
}


/*EXTL_DOC
 * Get root of contained sub-split tree.
 */
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
    {(DynFun*)splitinner_nextto, (DynFun*)splitpane_nextto},
    /*{splitinner_mark_current, splitpane_mark_current},*/
    {(DynFun*)split_get_config, (DynFun*)splitpane_get_config},
    END_DYNFUNTAB,
};


IMPLCLASS(WSplitUnused, WSplit, splitunused_deinit, splitunused_dynfuntab);
IMPLCLASS(WSplitPane, WSplitInner, splitpane_deinit, splitpane_dynfuntab);


/*}}}*/

