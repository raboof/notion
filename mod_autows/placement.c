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

#include <libtu/minmax.h>

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
#include "placement.h"
#include "autows.h"


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

#define PENALTY_NEVER    INT_MAX
#define PENALTY_INIT     (INT_MAX-1)
#define PENALTY_MAX      (INT_MAX-2)


typedef struct{
    WSplit *split;
    int penalty;
    int dest_w, dest_h;
    ExtlTab config;
} Res;


typedef struct{
    WAutoWS *ws;
    WSplit *node;
    WFrame *frame;
    WClientWin *cwin;
    int depth;
    int last_static_dir;
    const WSplitUnused *unused;
    Res res;
} PenaltyParams;


WHook *autows_layout_alt=NULL;


static bool mrsh_layout_extl(ExtlFn fn, PenaltyParams *p)
{
    ExtlTab t=extl_create_table(), ut=extl_create_table();
    bool ret=FALSE;
    
    extl_table_sets_o(t, "ws", (Obj*)p->ws);
    extl_table_sets_o(t, "node", (Obj*)p->node);
    extl_table_sets_o(t, "frame", (Obj*)p->frame);
    extl_table_sets_o(t, "cwin", (Obj*)p->cwin);
    extl_table_sets_i(t, "depth", p->depth);
    extl_table_sets_b(t, "last_static_vert", 
                      (p->last_static_dir==SPLIT_VERTICAL));
    extl_table_sets_i(ut, "l", p->unused->l);
    extl_table_sets_i(ut, "r", p->unused->r);
    extl_table_sets_i(ut, "t", p->unused->t);
    extl_table_sets_i(ut, "b", p->unused->b);
    extl_table_sets_i(ut, "tot_v", p->unused->tot_v);
    extl_table_sets_i(ut, "tot_h", p->unused->tot_h);
    extl_table_sets_t(t, "unused", ut);

    extl_call(fn, "t", "b", t, &ret);
    
    if(ret){
        double d;
        Res *r=&(p->res);
        r->split=p->node;
        r->penalty=PENALTY_NEVER;
        r->dest_w=-1;
        r->dest_h=-1;
        r->config=extl_table_none();

        extl_table_gets_i(t, "dest_w", &(r->dest_w));
        extl_table_gets_i(t, "dest_h", &(r->dest_h));
        if(extl_table_gets_d(t, "penalty", &d) && d>=0)
            r->penalty=(d>=PENALTY_NEVER ? PENALTY_NEVER : d);
        ret=(extl_table_gets_t(t, "config", &(r->config))
             || r->split->type==SPLIT_REGNODE);
    }
    
    extl_unref_table(ut);
    extl_unref_table(t);
    
    return ret;
}


typedef bool PenaltyFn(PenaltyParams *p);


static bool pfn_hook(PenaltyParams *p)
{
    return hook_call_alt_p(autows_layout_alt, p, 
                           (WHookMarshallExtl*)mrsh_layout_extl);
}


static bool pfn_fallback(PenaltyParams *p)
{
    p->res.split=p->node;
    p->res.penalty=PENALTY_MAX;
    p->res.dest_w=-1;
    p->res.dest_h=-1;

    if(p->node->type==SPLIT_UNUSED){
        p->res.config=extl_create_table();
        if(p->res.config==extl_table_none())
            return FALSE;
        extl_table_sets_o(p->res.config, "reference", (Obj*)(p->frame));
    }else if(p->node->type==SPLIT_REGNODE){
        p->res.config=extl_table_none();
    }else{
        return FALSE;
    }
    
    return TRUE;
}


static void scan(WSplit *split, PenaltyFn *pfn, WAutoWS *ws, WFrame *frame, 
                 WClientWin *cwin, int depth, int last_static_dir, 
                 const WSplitUnused *uspc, Res *best)
{
    if(split->type==SPLIT_REGNODE && !OBJ_IS(split->u.reg, WMPlex))
        return;
    
    if(split->type==SPLIT_UNUSED || split->type==SPLIT_REGNODE){
        PenaltyParams p;

        p.ws=ws;
        p.node=split;
        p.frame=frame;
        p.cwin=cwin;
        p.depth=depth;
        p.last_static_dir=last_static_dir;
        p.unused=uspc;

        if(pfn(&p)){
            if(p.res.penalty<best->penalty)
                *best=p.res;
            else
                extl_unref_table(p.res.config);
        }
        
    }else if(split->type==SPLIT_VERTICAL || split->type==SPLIT_HORIZONTAL){
        WSplit *tl=split->u.s.tl, *br=split->u.s.br;
        WSplitUnused tlunused, brunused, un;
        
        if(split->is_static)
            last_static_dir=split->type;

        split_get_unused(tl, &tlunused);
        split_get_unused(br, &brunused);
        if(split->type==SPLIT_VERTICAL){
            un=*uspc;
            UNUSED_B_ADD(un, brunused, br);
            un.tot_v=brunused.tot_v;
            scan(tl, pfn, ws, frame, cwin, depth+1, last_static_dir, &un, best);
            un=*uspc;
            UNUSED_T_ADD(un, tlunused, tl);
            un.tot_v=tlunused.tot_v;
            scan(br, pfn, ws, frame, cwin, depth+1, last_static_dir, &un, best);
        }else if(split->type==SPLIT_HORIZONTAL){
            un=*uspc;
            UNUSED_R_ADD(un, brunused, br);
            un.tot_h=brunused.tot_h;
            scan(tl, pfn, ws, frame, cwin, depth+1, last_static_dir, &un, best);
            un=*uspc;
            UNUSED_L_ADD(un, tlunused, tl);
            un.tot_h=tlunused.tot_h;
            scan(br, pfn, ws, frame, cwin, depth+1, last_static_dir, &un, best);
        }
    }
}


/*}}}*/


/*{{{ Split/replace unused code */


static bool do_replace(WAutoWS *ws, WFrame *frame, WClientWin *cwin, 
                       Res *rs)
{
    WSplit *u=rs->split;
    WSplit *p=u->parent;
    WSplit *node=ionws_load_node(&(ws->ionws), &(u->geom), rs->config);
    
    assert(u->type==SPLIT_UNUSED);
    
    if(node==NULL){
        WARN_FUNC("Malfunctioning hook #1.");
        return FALSE;
    }

    if(REGION_MANAGER(frame)!=(WRegion*)ws){
        WARN_FUNC("Malfunctioning hook #2.");
        destroy_obj((Obj*)node);
        return FALSE;
    }
    
    if(p==NULL){
        assert(ws->ionws.split_tree==u);
        ws->ionws.split_tree=node;
    }else if(p->u.s.tl==u){
        p->u.s.tl=node;
    }else{
        assert(p->u.s.br==u);
        p->u.s.br=node;
    }
    
    node->parent=p;
    
    u->parent=NULL;
    ioncore_defer_destroy((Obj*)u);
    
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
        int i;
        PenaltyFn *pfns[]={pfn_hook, pfn_fallback};
        
        for(i=0; i<2; i++){
            /*      split action         penalty       primn      w    h */
            Res rs={NULL, PENALTY_INIT, -1, -1, 0};
            WSplitUnused unused={0, 0, 0, 0, 0, 0};
            WSplit **tree=&(ws->ionws.split_tree);
            
            rs.config=extl_table_none();
            
            split_update_bounds(*tree, TRUE);
            scan(*tree, pfns[i], ws, frame, cwin, 0, SPLIT_UNUSED, &unused, 
                 &rs);
            
            if(rs.split!=NULL){
                /* Resize */
                if(rs.dest_w>0 || rs.dest_h>0){
                    WRectangle grq=rs.split->geom;
                    int gflags=REGION_RQGEOM_WEAK_ALL;
                    
                    if(rs.dest_w>0){
                        grq.w=rs.dest_w;
                        gflags&=~REGION_RQGEOM_WEAK_W;
                    }
                    
                    if(rs.dest_h>0){
                        grq.h=rs.dest_h;
                        gflags&=~REGION_RQGEOM_WEAK_H;
                    }
                    
                    split_tree_rqgeom(*tree, rs.split, gflags, &grq, NULL);
                }
                
                if(rs.split->type==SPLIT_UNUSED){
                    if(do_replace(ws, frame, cwin, &rs))
                        target=(WRegion*)frame;
                }else if(rs.split->type==SPLIT_REGNODE){
                    target=rs.split->u.reg;
                }else{
                    warn("There's a bug in placement code.");
                }
                
                extl_unref_table(rs.config);
            }
            
            if(target==NULL){
                warn("Scan #%d failed to find a placement for new client "
                     "window/frame.", i);
            }else{
                break;
            }
        }
        
        if(target!=(WRegion*)frame)
            destroy_obj((Obj*)frame);
    }else{
        target=find_suitable_target(&(ws->ionws));
    }

    if(target!=NULL){
        if(region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES)){
            return TRUE;
        }
    }
    
err:
    warn("Ooops... could not find a region to attach client window to "
         "on workspace %s.", region_name((WRegion*)ws));
    return FALSE;
}


/*}}}*/

