/*
 * ion/mod_autows/placement.h
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
#include "autows.h"
#include "splitext.h"


/*{{{ create_frame_for */


static WFrame *create_frame_for(WAutoWS *ws, WRegion *reg)
{
    WWindow *par=REGION_PARENT_CHK(ws, WWindow);
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


/*{{{ Code to find a suitable frame when there is no more free space */


static WRegion *find_suitable_target(WIonWS *ws)
{
    WRegion *r=ionws_current(ws);
    
    if(r!=NULL)
        return r;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r)
        return r;
    
    return NULL;
}


/*}}}*/


/*{{{ Placement scan */


typedef struct{
    WAutoWS *ws;
    WFrame *frame;
    WClientWin *cwin;
    
    WSplit *res_node;
    ExtlTab res_config;
    int res_w, res_h;
} PlacementParams;


WHook *autows_layout_alt=NULL;


static bool mrsh_layout_extl(ExtlFn fn, PlacementParams *p)
{
    ExtlTab t=extl_create_table();
    bool ret=FALSE;
    
    extl_table_sets_o(t, "ws", (Obj*)p->ws);
    extl_table_sets_o(t, "frame", (Obj*)p->frame);
    extl_table_sets_o(t, "cwin", (Obj*)p->cwin);

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


static bool fallback_layout(PlacementParams *p)
{
    if(p->ws->ionws.split_tree==NULL)
        return FALSE;
    
    p->res_node=split_current_todir(p->ws->ionws.split_tree, SPLIT_ANY,
                                    PRIMN_ANY, fallback_filter);

    if(p->res_node!=NULL && OBJ_IS(p->res_node, WSplitUnused)){
        p->res_config=extl_create_table();
        if(p->res_config==extl_table_none())
            return FALSE;
        extl_table_sets_o(p->res_config, "reg", (Obj*)(p->frame));
    }
    
    return (p->res_node!=NULL);
}


/*}}}*/


/*{{{ Split/replace unused code */


static bool do_replace(WAutoWS *ws, WFrame *frame, WClientWin *cwin, 
                       PlacementParams *rs)
{
    WSplit *u=rs->res_node;
    WSplitInner *p=rs->res_node->parent;
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
    
    if(p==NULL){
        assert(ws->ionws.split_tree==u);
        ws->ionws.split_tree=node;
    }else{
        splitinner_replace(p, u, node);
    }
    
    u->parent=NULL;
    ioncore_defer_destroy((Obj*)u);
    
    if(ws->ionws.stdispnode!=NULL)
        split_regularise_stdisp(ws->ionws.stdispnode);

    return TRUE;
}

/*}}}*/


/*{{{ The main dynfun */


bool autows_manage_clientwin(WAutoWS *ws, WClientWin *cwin,
                             const WManageParams *param, int redir)
{
    WRegion *target=NULL;
    WFrame *frame=create_frame_for(ws, (WRegion*)cwin);
    
    assert(ws->ionws.split_tree!=NULL);
    
    if(frame!=NULL){
        WSplit **tree=&(ws->ionws.split_tree);
        PlacementParams rs;
        
        rs.ws=ws;
        rs.frame=frame;
        rs.cwin=cwin;
        rs.res_node=NULL;
        rs.res_config=extl_table_none();
        rs.res_w=-1;
        rs.res_h=-1;

        split_update_bounds(*tree, TRUE);
        
        assert(autows_layout_alt!=NULL);
        
        hook_call_p(autows_layout_alt, &rs,
                    (WHookMarshallExtl*)mrsh_layout_extl);
        
        if(rs.res_node==NULL)
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
                
                splittree_rqgeom(*tree, rs.res_node, gflags, &grq, NULL);
            }
            
            if(OBJ_IS(rs.res_node, WSplitUnused)){
                if(do_replace(ws, frame, cwin, &rs))
                    target=(WRegion*)frame;
            }else{
                assert(OBJ_IS(rs.res_node, WSplitRegion));
                target=((WSplitRegion*)rs.res_node)->reg;
            }
            
            extl_unref_table(rs.res_config);
        }
        
        if(target!=(WRegion*)frame)
            destroy_obj((Obj*)frame);
    }
    
    if(target==NULL)
        target=find_suitable_target(&(ws->ionws));

    if(target!=NULL){
        if(region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES)){
            return TRUE;
        }
    }

err:
    warn(TR("Ooops... could not find a region to attach client window to "
            "on workspace %s."), region_name((WRegion*)ws));
    return FALSE;
}


/*}}}*/

