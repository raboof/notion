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
#include <libtu/minmax.h>
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
#include "splitext.h"


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
    return (WRegion*)create_frame(parent, fp, "frame-tiled-autows");
}



typedef struct{
    WAutoWS *ws;
    ExtlTab layout;
} InitParams;


static bool mrsh_init_layout_extl(ExtlFn fn, InitParams *p)
{
    ExtlTab t=extl_create_table();
    bool ret=FALSE;
    
    extl_table_sets_o(t, "ws", (Obj*)p->ws);

    ret=extl_call(fn, "t", "b", t, &ret);
    
    if(ret)
        ret=extl_table_gets_t(t, "layout", &(p->layout));
        
    extl_unref_table(t);
    return ret;
}


static bool autows_init_layout(WAutoWS *ws)
{
    InitParams p;
    
    p.ws=ws;
    p.layout=extl_table_none();
	
    hook_call_p(autows_init_layout_alt, &p,
                (WHookMarshallExtl*)mrsh_init_layout_extl);

    if(p.layout!=extl_table_none()){            
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws),
                                             &REGION_GEOM(ws), 
                                             p.layout);
        extl_unref_table(p.layout);
    }
         
    if(ws->ionws.split_tree==NULL)
        ws->ionws.split_tree=(WSplit*)create_splitunused(&REGION_GEOM(ws), ws);
        
    return (ws->ionws.split_tree!=NULL);
}


bool autows_init(WAutoWS *ws, WWindow *parent, const WFitParams *fp, 
                 bool ilo)
{
    if(!ionws_init(&(ws->ionws), parent, fp, 
                   create_frame_autows, FALSE))
        return FALSE;
    
    assert(ws->ionws.split_tree==NULL);
    
    if(ilo){
        if(!autows_init_layout(ws)){
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
        splittree_remove((WSplit*)node, !ds);
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
                warn(TR("Unable to re-initialise workspace. Destroying."));
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


static WSplit *load_splitunused(WAutoWS *ws, const WRectangle *geom, 
                                ExtlTab tab)
{
    return (WSplit*)create_splitunused(geom, (WAutoWS*)ws);
}


static WSplit *load_splitpane(WAutoWS *ws, const WRectangle *geom, ExtlTab tab)
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

#define MINS 8

static void adjust_tls_brs(int *tls, int *brs, int total)
{
    if(*tls+*brs<total){
        *tls=total*(*tls)/(*tls+*brs);
        *brs=total-(*tls);
    }
        
    *tls=minof(maxof(MINS, *tls), total);
    *brs=minof(maxof(MINS, *brs), total);
}


static WSplit *load_splitfloat(WAutoWS *ws, const WRectangle *geom, 
                               ExtlTab tab)
{
    WSplit *tl=NULL, *br=NULL;
    WSplitFloat *split;
    char *dir_str;
    int dir, brs, tls;
    ExtlTab subtab;
    WRectangle tlg, brg;
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

    split=create_splitfloat(geom, ws, dir);
    if(split==NULL)
        return NULL;

    tlg=*geom;
    brg=*geom;
    
    if(dir==SPLIT_HORIZONTAL){
        adjust_tls_brs(&tls, &brs, geom->w);
        tlg.w=tls;
        brg.w=brs;
        brg.x=geom->x+geom->w-brs;
    }else{
        adjust_tls_brs(&tls, &brs, geom->h);
        tlg.h=tls;
        brg.h=brs;
        brg.y=geom->y+geom->h-brs;
    }

    splitfloat_update_handles(split, &tlg, &brg);
    
    if(extl_table_gets_t(tab, "tl", &subtab)){
        WRectangle g=tlg;
        splitfloat_tl_pwin_to_cnt(split, &g);
        tl=ionws_load_node((WIonWS*)ws, &g, subtab);
        extl_unref_table(subtab);
    }
    
    if(extl_table_gets_t(tab, "br", &subtab)){
        WRectangle g;
        if(tl==NULL){
            g=*geom;
        }else{
            g=brg;
            splitfloat_br_pwin_to_cnt(split, &g);
        }
        br=ionws_load_node((WIonWS*)ws, &brg, subtab);
        extl_unref_table(subtab);
    }
    
    if(tl==NULL || br==NULL){
        destroy_obj((Obj*)split);
        return (tl==NULL ? br : tl);
    }
    
    tl->parent=(WSplitInner*)split;
    br->parent=(WSplitInner*)split;

    /*split->tmpsize=tls;*/
    split->ssplit.tl=tl;
    split->ssplit.br=br;
    
    return (WSplit*)split;
}


static WSplit *autows_load_node(WAutoWS *ws, const WRectangle *geom, 
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
        else if(strcmp(s, "WSplitFloat")==0)
            return load_splitfloat(ws, geom, tab);
    }

    return ionws_load_node_default(&(ws->ionws), geom, tab);
}


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
        if(!autows_init_layout(ws)){
            destroy_obj((Obj*)ws);
            return NULL;
        }
    }
    
    split_restack(ws->ionws.split_tree, ((WGenWS*)ws)->dummywin, Above);
    
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
    
    {(DynFun*)ionws_load_node,
     (DynFun*)autows_load_node},

    {(DynFun*)ionws_do_get_nextto,
     (DynFun*)autows_do_get_nextto},

    {(DynFun*)ionws_do_get_farthest,
     (DynFun*)autows_do_get_farthest},

/*
    {(DynFun*)region_managed_control_focus,
     (DynFun*)autows_managed_control_focus},
    */
    END_DYNFUNTAB
};


IMPLCLASS(WAutoWS, WIonWS, autows_deinit, autows_dynfuntab);

    
/*}}}*/

