/*
 * ion/mod_panews/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <math.h>
#include <string.h>

#include <libtu/minmax.h>
#include <libtu/objp.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/clientwin.h>
#include <ioncore/attach.h>
#include <ioncore/manage.h>
#include <ioncore/extl.h>
#include <ioncore/framep.h>
#include <ioncore/names.h>
#include <ioncore/region-iter.h>
#include <ioncore/resize.h>
#include <ioncore/defer.h>
#include <mod_ionws/split.h>
#include <mod_ionws/split-stdisp.h>
#include "placement.h"
#include "panews.h"
#include "splitext.h"
#include "unusedwin.h"


WHook *panews_init_layout_alt=NULL;
WHook *panews_make_placement_alt=NULL;


/*{{{ create_frame_for */


static WFrame *create_frame_for(WPaneWS *ws, WRegion *reg)
{
    WWindow *par=REGION_PARENT(ws);
    WFitParams fp;
    WRectangle mg;
    WFrame *frame;
    
    if(par==NULL)
        return NULL;
    
    fp.g=REGION_GEOM(ws);
    fp.mode=REGION_FIT_BOUNDS;
    
    frame=(WFrame*)ws->ionws.create_frame_fn(par, &fp);
    
    if(frame==NULL)
        return NULL;

    frame->flags|=FRAME_DEST_EMPTY;
    
    mplex_managed_geom((WMPlex*)frame, &mg);
    
    fp.g.w=REGION_GEOM(reg).w+(REGION_GEOM(frame).w-mg.w);
    fp.g.h=REGION_GEOM(reg).h+(REGION_GEOM(frame).h-mg.h);
    fp.mode=REGION_FIT_EXACT;
    
    region_fitrep((WRegion*)frame, NULL, &fp);
    
    return (WFrame*)frame;
}


/*}}}*/


/*{{{ Placement scan */


static bool mrsh_layout_extl(ExtlFn fn, WPaneWSPlacementParams *p)
{
    ExtlTab t=extl_create_table();
    bool ret=FALSE;
    
    extl_table_sets_o(t, "ws", (Obj*)p->ws);
    extl_table_sets_o(t, "frame", (Obj*)p->frame);
    extl_table_sets_o(t, "reg", (Obj*)p->reg);
    extl_table_sets_o(t, "specifier", (Obj*)p->specifier);

    extl_call(fn, "t", "b", t, &ret);
    
    if(ret){
        ret=FALSE;

        extl_table_gets_i(t, "res_w", &(p->res_w));
        extl_table_gets_i(t, "res_h", &(p->res_h));
        
        if(extl_table_gets_o(t, "res_node", (Obj**)&(p->res_node))){
            if(OBJ_IS(p->res_node, WSplitUnused)){
                if(!extl_table_gets_t(t, "res_config", &(p->res_config))){
                    warn(TR("Malfunctioning placement hook; condition #%d."), 1);
                    goto err;
                }
            }else if(!OBJ_IS(p->res_node, WSplitRegion)){
                warn(TR("Malfunctioning placement hook; condition #%d."), 2);
                goto err;
            }
        }
    }
    
    extl_unref_table(t);
    
    return ret;
    
err:    
    p->res_node=NULL;
    extl_unref_table(t);
    return FALSE;
}


static bool plainregionfilter(WSplit *node)
{
    return (strcmp(OBJ_TYPESTR(node), "WSplitRegion")==0);
}


static bool fallback_filter(WSplit *node)
{
    return (OBJ_IS(node, WSplitUnused) || plainregionfilter(node));
}


static bool fallback_layout(WPaneWSPlacementParams *p)
{
    if(p->ws->ionws.split_tree==NULL)
        return FALSE;
    
    if(p->specifier!=NULL){
        p->res_node=(WSplit*)p->specifier;
    }else{
        p->res_node=split_current_todir(p->ws->ionws.split_tree, SPLIT_ANY,
                                        PRIMN_ANY, fallback_filter);
    }

    if(p->res_node!=NULL && OBJ_IS(p->res_node, WSplitUnused)){
        p->res_config=extl_create_table();
        if(p->res_config==extl_table_none() || p->frame==NULL)
            return FALSE;
        extl_table_sets_o(p->res_config, "reg", (Obj*)(p->frame));
    }
    
    return (p->res_node!=NULL);
}


/*}}}*/


/*{{{ Split/replace unused code */


static bool do_replace(WPaneWS *ws, WFrame *frame, WRegion *reg, 
                       WPaneWSPlacementParams *rs)
{
    WSplit *u=rs->res_node;
    WSplit *node=ionws_load_node(&(ws->ionws), &(u->geom), rs->res_config);
    
    assert(OBJ_IS(u, WSplitUnused));
    
    if(node==NULL){
        warn(TR("Malfunctioning placement hook; condition #%d."), 3);
        return FALSE;
    }

    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        warn(TR("Malfunctioning placement hook; condition #%d."), 4);
        destroy_obj((Obj*)node);
        return FALSE;
    }
    
    if(u->parent!=NULL)
        splitinner_replace(u->parent, u, node);
    else
        splittree_changeroot((WSplit*)u, node);
    
    u->parent=NULL;
    ioncore_defer_destroy((Obj*)u);
    
    if(ws->ionws.stdispnode!=NULL)
        split_regularise_stdisp(ws->ionws.stdispnode);

    if(ws->ionws.split_tree!=NULL)
        split_restack(ws->ionws.split_tree, ((WGenWS*)ws)->dummywin, Above);

    return TRUE;
}

/*}}}*/


/*{{{ The main dynfun */


static bool current_unused(WPaneWS *ws)
{
    return OBJ_IS(ionws_current(&ws->ionws), WUnusedWin);
}


static WRegion *panews_get_target(WPaneWS *ws, WSplitUnused *specifier,
                                  WRegion *reg)
{
    WRegion *target=NULL;
    WFrame *frame=create_frame_for(ws, reg);
    WSplit **tree=&(ws->ionws.split_tree);
    WPaneWSPlacementParams rs;
    
    assert(ws->ionws.split_tree!=NULL);

    rs.ws=ws;
    rs.frame=frame;
    rs.reg=reg;
    rs.specifier=specifier;
    rs.res_node=NULL;
    rs.res_config=extl_table_none();
    rs.res_w=-1;
    rs.res_h=-1;
    
    if(frame!=NULL){
        split_update_bounds(*tree, TRUE);
        
        assert(panews_make_placement_alt!=NULL);
        
        hook_call_p(panews_make_placement_alt, &rs,
                    (WHookMarshallExtl*)mrsh_layout_extl);
    }
        
    if(rs.res_node==NULL && specifier==NULL)
        fallback_layout(&rs);
        
    if(rs.res_node!=NULL){
        /* Resize */
        if(rs.res_w>0 || rs.res_h>0){
            WRectangle grq=rs.res_node->geom;
            int gflags=REGION_RQGEOM_WEAK_ALL;
            
            if(rs.res_w>0){
                grq.w=rs.res_w;
                gflags&=~REGION_RQGEOM_WEAK_W;
            }
            
            if(rs.res_h>0){
                grq.h=rs.res_h;
                gflags&=~REGION_RQGEOM_WEAK_H;
            }
            
            splittree_rqgeom(rs.res_node, gflags, &grq, NULL);
        }
        
        if(OBJ_IS(rs.res_node, WSplitUnused)){
            if(frame!=NULL){
                if(do_replace(ws, frame, reg, &rs))
                    target=(WRegion*)frame;
            }
        }else{
            assert(OBJ_IS(rs.res_node, WSplitRegion));
            target=((WSplitRegion*)rs.res_node)->reg;
        }
        
        extl_unref_table(rs.res_config);
    }
        
    if(frame!=NULL && target!=(WRegion*)frame)
        destroy_obj((Obj*)frame);
    
    if(target!=NULL && current_unused(ws))
        region_goto(target);
    
    return target;
}


bool panews_manage_clientwin(WPaneWS *ws, WClientWin *cwin,
                             const WManageParams *param, int redir)
{
    WRegion *target=panews_get_target(ws, NULL, (WRegion*)cwin);
    
    if(target!=NULL){
        if(region_manage_clientwin(target, cwin, param,
                                   MANAGE_REDIR_PREFER_YES)){
            return TRUE;
        }
    }

    warn(TR("Ooops... could not find a region to attach client window to "
            "on workspace %s."), region_name((WRegion*)ws));
    return FALSE;
}


bool panews_handle_unused_drop(WPaneWS *ws, WSplitUnused *specifier, 
                               WRegion *reg)
{
    WRegion *target=panews_get_target(ws, specifier, reg);
    
    if(target==NULL || !OBJ_IS(target, WMPlex))
        return FALSE;
    
    return (mplex_attach_simple((WMPlex*)target, reg, 
                                MPLEX_ATTACH_SWITCHTO)!=NULL);
}


/*}}}*/

