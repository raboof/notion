/*
 * ion/mod_panews/panews.c
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
#include <libextl/extl.h>
#include <libmainloop/defer.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/focus.h>
#include <ioncore/manage.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/regbind.h>
#include <ioncore/extlconv.h>
#include <ioncore/frame.h>
#include <mod_ionws/ionws.h>
#include <mod_ionws/split.h>
#include "panews.h"
#include "placement.h"
#include "main.h"
#include "splitext.h"


/*{{{ Create/destroy */


bool panews_managed_add(WPaneWS *ws, WRegion *reg)
{
    region_add_bindmap_owned(reg, mod_panews_panews_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_add_bindmap(reg, mod_panews_frame_bindmap);
    
    return ionws_managed_add_default(&(ws->ionws), reg);
}


static WRegion *create_frame_panews(WWindow *parent, const WFitParams *fp)
{
    return (WRegion*)create_frame(parent, fp, "frame-tiled-panews");
}


static bool mrsh_init_layout_extl(ExtlFn fn, WPaneWSInitParams *p)
{
    ExtlTab t=extl_create_table();
    bool ret=FALSE;
    
    extl_table_sets_o(t, "ws", (Obj*)p->ws);

    extl_protect(NULL);
    ret=extl_call(fn, "t", "b", t, &ret);
    extl_unprotect(NULL);
    
    if(ret)
        ret=extl_table_gets_t(t, "layout", &(p->layout));
        
    extl_unref_table(t);
    return ret;
}


static bool panews_init_layout(WPaneWS *ws)
{
    WPaneWSInitParams p;
    
    p.ws=ws;
    p.layout=extl_table_none();
	
    hook_call_p(panews_init_layout_alt, &p,
                (WHookMarshallExtl*)mrsh_init_layout_extl);

    if(p.layout!=extl_table_none()){            
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws),
                                             &REGION_GEOM(ws), 
                                             p.layout);
        extl_unref_table(p.layout);
    }
         
    if(ws->ionws.split_tree==NULL)
        ws->ionws.split_tree=(WSplit*)create_splitunused(&REGION_GEOM(ws), ws);
        
    if(ws->ionws.split_tree!=NULL)
        ws->ionws.split_tree->ws_if_root=&(ws->ionws);
    
    return (ws->ionws.split_tree!=NULL);
}


bool panews_init(WPaneWS *ws, WWindow *parent, const WFitParams *fp, 
                 bool ilo)
{
    if(!ionws_init(&(ws->ionws), parent, fp, 
                   create_frame_panews, FALSE))
        return FALSE;
    
    assert(ws->ionws.split_tree==NULL);
    
    if(ilo){
        if(!panews_init_layout(ws)){
            panews_deinit(ws);
            return FALSE;
        }
    }
    
    return TRUE;
}


WPaneWS *create_panews(WWindow *parent, const WFitParams *fp, bool cu)
{
    CREATEOBJ_IMPL(WPaneWS, panews, (p, parent, fp, cu));
}


WPaneWS *create_panews_simple(WWindow *parent, const WFitParams *fp)
{
    return create_panews(parent, fp, TRUE);
}


void panews_deinit(WPaneWS *ws)
{
    ionws_deinit(&(ws->ionws));
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


static void panews_do_managed_remove(WPaneWS *ws, WRegion *reg)
{
    ionws_do_managed_remove(&(ws->ionws), reg);
    region_remove_bindmap_owned(reg, mod_panews_panews_bindmap, (WRegion*)ws);
    if(OBJ_IS(reg, WFrame))
        region_remove_bindmap(reg, mod_panews_frame_bindmap);
}


static bool plainregionfilter(WSplit *node)
{
    return (strcmp(OBJ_TYPESTR(node), "WSplitRegion")==0);
}



void panews_managed_remove(WPaneWS *ws, WRegion *reg)
{
    bool ds=OBJ_IS_BEING_DESTROYED(ws);
    bool act=REGION_IS_ACTIVE(reg);
    bool mcf=region_may_control_focus((WRegion*)ws);
    WSplitRegion *node=get_node_check(ws, reg);
    WRegion *other=NULL;

    other=ionws_do_get_nextto(&(ws->ionws), reg, SPLIT_ANY, PRIMN_ANY, FALSE);
    
    panews_do_managed_remove(ws, reg);

    if(node==(WSplitRegion*)(ws->ionws.stdispnode))
        ws->ionws.stdispnode=NULL;
    
    if(node==NULL)
        return;
    
    splittree_remove((WSplit*)node, !ds);
    
    if(!ds){
        if(other==NULL){
            if(ws->ionws.split_tree==NULL){
                warn(TR("Unable to re-initialise workspace. Destroying."));
                mainloop_defer_destroy((Obj*)ws);
            }else if(act && mcf){
                /* We don't want to give the stdisp focus, even if one exists. 
                 * Or do we?
                 */
                genws_fallback_focus((WGenWS*)ws, FALSE);
            }
        }else if(act && mcf){
            region_warp(other);
        }
    }
}


/*}}}*/


/*{{{ Misc. */


bool panews_managed_goto(WPaneWS *ws, WRegion *reg, int flags)
{
    if(flags&REGION_GOTO_ENTERWINDOW){
        WSplitRegion *other, *node=get_node_check(ws, reg);
        if(node!=NULL && OBJ_IS(node, WSplitUnused)){
            /* An unused region - do not focus unless there are no
             * normal regions in its pane. 
             */
            other=split_tree_find_region_in_pane_of((WSplit*)node);
            if(other!=NULL){
                ionws_managed_goto(&(ws->ionws), other->reg, 
                                   flags&~REGION_GOTO_ENTERWINDOW);
                return FALSE;
            }
        }
    }
        
    return ionws_managed_goto(&(ws->ionws), reg, flags);
}


static bool filter_no_stdisp_unused(WSplit *split)
{
    return (OBJ_IS(split, WSplitRegion)
            && !OBJ_IS(split, WSplitST)
            && !OBJ_IS(split, WSplitUnused));
}


bool panews_managed_may_destroy(WPaneWS *ws, WRegion *reg)
{
    if(region_manager_allows_destroying((WRegion*)ws))
        return TRUE;
    
    if(ionws_do_get_nextto(&(ws->ionws), reg, 
                           SPLIT_ANY, PRIMN_ANY, FALSE)==NULL){
        return FALSE;
    }
    
    return TRUE;
}


bool panews_may_destroy(WPaneWS *ws)
{
    if(split_current_todir(ws->ionws.split_tree, SPLIT_ANY, PRIMN_ANY, 
                           filter_no_stdisp_unused)!=NULL){
        warn(TR("Refusing to close non-empty workspace."));
        return FALSE;
    }
    
    return TRUE;
}


/*
static WRegion *panews_rqclose_propagate(WPaneWS *ws, WRegion *sub)
{
    WSplitRegion *node=NULL;
    WRegion *reg=NULL;
    
    if(sub==NULL){
        if(ws->ionws.split_tree!=NULL){
            node=(WSplitRegion*)split_current_todir(ws->ionws.split_tree, 
                                                    SPLIT_ANY, PRIMN_ANY, 
                                                    filter_no_stdisp_unused);
        }
        if(node==NULL){
            mainloop_defer_destroy((Obj*)ws);
            return (WRegion*)ws;
        }
        if(node->reg==NULL)
            return NULL;
        sub=node->reg;
    }else{
        node=get_node_check(ws, sub);
        if(node==NULL)
            return NULL;
    }
    
    
    return (region_rqclose(sub) ? sub : NULL);
}
*/


/*}}}*/


/*{{{ Save */


ExtlTab panews_get_configuration(WPaneWS *ws)
{
    return ionws_get_configuration(&(ws->ionws));
}


/*}}}*/


/*{{{ Load */


static WSplit *load_splitunused(WPaneWS *ws, const WRectangle *geom, 
                                ExtlTab tab)
{
    return (WSplit*)create_splitunused(geom, (WPaneWS*)ws);
}


static WSplit *load_splitpane(WPaneWS *ws, const WRectangle *geom, ExtlTab tab)
{
    ExtlTab t;
    WSplitPane *pane;
    WSplit *cnt;

    pane=create_splitpane(geom, NULL);
    if(pane==NULL)
        return NULL;
    
    if(extl_table_gets_t(tab, "contents", &t)){
        cnt=ionws_load_node(&(ws->ionws), geom, t);
        extl_unref_table(t);
    }else{
        cnt=load_splitunused(ws, geom, extl_table_none());
    }
    
    if(cnt==NULL){
        destroy_obj((Obj*)pane);
        return NULL;
    }
    
    pane->contents=cnt;
    cnt->parent=&(pane->isplit);
    
    assert(pane->marker==NULL);
    extl_table_gets_s(tab, "marker", &(pane->marker));

    return (WSplit*)pane;
}


static WSplit *panews_load_node(WPaneWS *ws, const WRectangle *geom, 
                                ExtlTab tab)
{
    char *s=NULL;
    
    if(!extl_table_gets_s(tab, "type", &s)){
        WRegion *reg=NULL;
        /* Shortcuts for templates.lua */
        if(extl_table_gets_o(tab, "reg", (Obj**)&reg)){
            if(OBJ_IS(reg, WRegion))
                return load_splitregion_doit(&(ws->ionws), geom, tab);
        }else{
            return load_splitunused(ws, geom, tab);
        }
    }else{
        if(strcmp(s, "WSplitPane")==0)
            return load_splitpane(ws, geom, tab);
        else if(strcmp(s, "WSplitUnused")==0)
            return load_splitunused(ws, geom, tab);
    }

    return ionws_load_node_default(&(ws->ionws), geom, tab);
}


WRegion *panews_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WPaneWS *ws;
    ExtlTab treetab;

    ws=create_panews(par, fp, FALSE);
    
    if(ws==NULL)
        return NULL;
 
    if(extl_table_gets_t(tab, "split_tree", &treetab)){
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws), &REGION_GEOM(ws), 
                                             treetab);
        extl_unref_table(treetab);
    }
    
    if(ws->ionws.split_tree==NULL){
        if(!panews_init_layout(ws)){
            destroy_obj((Obj*)ws);
            return NULL;
        }
    }
    
    ws->ionws.split_tree->ws_if_root=ws;
    split_restack(ws->ionws.split_tree, ((WGenWS*)ws)->dummywin, Above);
    
    return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab panews_dynfuntab[]={
    {region_managed_remove, 
     panews_managed_remove},

    {(DynFun*)region_manage_clientwin, 
     (DynFun*)panews_manage_clientwin},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)panews_get_configuration},

    {(DynFun*)region_may_destroy,
     (DynFun*)panews_may_destroy},

    {(DynFun*)region_managed_may_destroy,
     (DynFun*)panews_managed_may_destroy},

    {(DynFun*)ionws_managed_add,
     (DynFun*)panews_managed_add},
    
    {(DynFun*)ionws_load_node,
     (DynFun*)panews_load_node},

    {(DynFun*)ionws_do_get_nextto,
     (DynFun*)panews_do_get_nextto},

    {(DynFun*)ionws_do_get_farthest,
     (DynFun*)panews_do_get_farthest},

    {(DynFun*)region_managed_goto,
     (DynFun*)panews_managed_goto},

    /*
    {(DynFun*)region_rqclose_propagate,
     (DynFun*)panews_rqclose_propagate},*/
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WPaneWS, WIonWS, panews_deinit, panews_dynfuntab);

    
/*}}}*/

