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

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/clientwin.h>
#include <ioncore/attach.h>
#include <ioncore/manage.h>
#include <ioncore/extl.h>
#include <ioncore/framep.h>
#include <ioncore/names.h>
#include <ioncore/region-iter.h>
#include <libtu/minmax.h>
#include <mod_ionws/split.h>
#include "placement.h"
#include "autoframe.h"
#include "autows.h"


/*{{{ create_frame_for */


static WFrame *create_frame_for(WAutoWS *ws, WRegion *reg)
{
    WWindow *par=REGION_PARENT_CHK(ws, WWindow);
    WFitParams fp;
    WRectangle mg;
    WAutoFrame *frame;
    
    if(par==NULL)
        return NULL;
    
    fp.g=REGION_GEOM(ws);
    fp.mode=REGION_FIT_BOUNDS;
    
    /*frame=(WFrame*)(ws->ionws.create_frame_fn(par, &fp));*/
    frame=create_autoframe(par, &fp);
    
    if(frame==NULL)
        return NULL;
    
    frame->autocreated=TRUE;
    
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


/*{{{ Code for looking up free space */


#define WS_REALLY_FULL(ws) FALSE

typedef struct{
    WSplit *split;
    int penalty;
    int dir;
    int primn;
    bool grow;
} Res;

#define PENALTY_NEVER    INT_MAX
#define PENALTY_INIT     (INT_MAX-1)
#define PENALTY_ENDSCALE (-1)

/* grow or shrink pixels */
static int fit_penalty_scale[][2]={
    {-500, 1000},
    { -50,  100},
    {   0,    0},
    {  50,  100},
    { 500, 1000},
    {   0, PENALTY_ENDSCALE}
};

/* slack pixels used */
static int slack_penalty_scale[][2]={
    {   0,    0},
    {  50,  200},
    { 200, 1000},
    { 200, PENALTY_NEVER},
    {   0, PENALTY_ENDSCALE}
};


/* pixels remaining in other side of split */
static int split_penalty_scale[][2]={
    {  32, PENALTY_NEVER},
    {  32, 1000},
    { 500,  100},
    {   0, PENALTY_ENDSCALE}
};


/*static int scale(int val, int max_p, int max_v)
{
    return maxof(max_p, val*max_p/max_v);
}*/

static int interp1(int scale[][2], int v)
{
    assert(scale[0][1]!=PENALTY_ENDSCALE);
    
    if(v<=scale[0][0] || scale[1][1]==PENALTY_ENDSCALE)
        return scale[0][1];
    
    if(v<scale[1][0]){
        if(scale[0][1]==PENALTY_NEVER || scale[1][1]==PENALTY_NEVER)
            return PENALTY_NEVER;
        
        return ((scale[1][1]*(v-scale[0][0])+scale[0][1]*(scale[1][0]-v))
                /(scale[1][0]-scale[0][0]));
    }

    return interp1(scale+1, v);
}


static int fit_penalty(int s, int ss, int slack)
{
    int d=ss-s;
    
    return interp1(fit_penalty_scale, ss-s);
}

#define hfit_penalty fit_penalty
#define vfit_penalty fit_penalty


static int slack_penalty(int s, int ss, int slack)
{
    int ds=ss-s;
    
    if(ds>=0)
        return PENALTY_NEVER; /* fit will do */
    
    return interp1(slack_penalty_scale, -ds);

}

#define hslack_penalty slack_penalty
#define vslack_penalty slack_penalty


static int split_penalty(int s, int ss, int slack)
{
    return interp1(split_penalty_scale, ss-s);
}

#define hsplit_penalty split_penalty
#define vsplit_penalty split_penalty


static int combine(int h, int v)
{
    if(h==PENALTY_NEVER || v==PENALTY_NEVER)
        return PENALTY_NEVER;
    
    return h+v;
}


static int getvslack(WSplit *split)
{
    return maxof(0, split->geom.h-split->used_h);
}


static int gethslack(WSplit *split)
{
    return maxof(0, split->geom.w-split->used_w);
}


static void scan(WSplit *split, int depth, int vslack, int hslack, 
                 const WRectangle *geom, Res *best)
{
    int thisvslack=getvslack(split);
    int thishslack=gethslack(split);
    int h_gof, v_gof, h_gol, v_gol;
    
    /*
     hslack=0;
     vslack=0;
     */
    
    /* Use penalty instead */
    /* Only use SPLIT_UNUSED */
    /* Only split orthogonally to deepest previous 'static' split? */
    
    if(split->type==SPLIT_UNUSED){
        int phfit=hfit_penalty(geom->w, split->geom.w, hslack);
        int pvfit=vfit_penalty(geom->h, split->geom.h, vslack);
        int phslack=hslack_penalty(geom->w, split->geom.w, hslack);
        int pvslack=vslack_penalty(geom->h, split->geom.h, vslack);
        int phsplit=hsplit_penalty(geom->w, split->geom.w, hslack);
        int pvsplit=vsplit_penalty(geom->h, split->geom.h, vslack);
        int p;
        
        p=combine(phfit, pvfit);
        fprintf(stderr, "FP: %d\n", p);
        if(p<best->penalty){
            best->split=split;
            best->penalty=p;
            best->dir=SPLIT_UNUSED;
            best->primn=PRIMN_BR;
            best->grow=FALSE;
        }
    
        p=combine(phslack, pvslack);
        fprintf(stderr, "SP: %d\n", p);
        if(p<best->penalty){
            best->split=split;
            best->penalty=p;
            best->dir=SPLIT_UNUSED;
            best->primn=PRIMN_BR;
            best->grow=TRUE;
        }
        
        p=combine(phfit, pvsplit);
        fprintf(stderr, "VSP: %d\n", p);
        if(p<best->penalty){
            best->split=split;
            best->penalty=p;
            best->dir=SPLIT_VERTICAL;
            best->primn=PRIMN_TL; /* Should decide this based on borders */
            best->grow=FALSE;
        }

        p=combine(phsplit, pvfit);
        fprintf(stderr, "HSP: %d\n", p);
        if(p<best->penalty){
            best->split=split;
            best->penalty=p;
            best->dir=SPLIT_HORIZONTAL;
            best->primn=PRIMN_TL; /* Should decide this based on borders */
            best->grow=FALSE;
        }
    }
    
    if(split->type==SPLIT_VERTICAL){
        int tlslack=getvslack(split->u.s.tl);
        int brslack=getvslack(split->u.s.br);
        scan(split->u.s.tl, depth+1, vslack+brslack, hslack, geom, best);
        scan(split->u.s.br, depth+1, vslack+tlslack, hslack, geom, best);
    }else if(split->type==SPLIT_HORIZONTAL){
        int tlslack=gethslack(split->u.s.tl);
        int brslack=gethslack(split->u.s.br);
        scan(split->u.s.tl, depth+1, vslack, hslack+brslack, geom, best);
        scan(split->u.s.br, depth+1, vslack, hslack+tlslack, geom, best);
    }
}


/*}}}*/


/*{{{ Layout initialisation */


WHook *autows_init_layout_alt=NULL;

    
static bool mrsh_init_extl(ExtlFn fn, WAutoWSInitLayoutParam *p)
{
    ExtlTab t=extl_create_table();
    bool ret=FALSE;
    
    extl_table_sets_o(t, "ws", (Obj*)p->ws);
    extl_table_sets_o(t, "reg", (Obj*)p->reg);
    
    extl_call(fn, "t", "b", t, &ret);
    
    if(ret)
        ret=extl_table_gets_t(t, "config", &(p->config));
    
    extl_unref_table(t);
    
    return ret;
}


static void create_initial_configuration(WAutoWS *ws, WRegion *reg)
{
    WAutoWSInitLayoutParam p;
    WWindow *par;

    p.ws=ws;
    p.reg=reg;
    p.config=extl_table_none();
    
    hook_call_alt_p(autows_init_layout_alt, &p, 
                    (WHookMarshallExtl*)mrsh_init_extl);
    
    par=REGION_PARENT_CHK(ws, WWindow);
    if(par==NULL)
        goto ret;
    
    if(ws->ionws.split_tree!=NULL){
        WARN_FUNC("Malfunctioning hook.");
        goto ret;
    }

    if(p.config!=extl_table_none()){
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws), par,
                                             &REGION_GEOM(ws), 
                                             p.config);
        
        if(ws->ionws.split_tree!=NULL){
            if(REGION_MANAGER(reg)!=(WRegion*)ws)
                WARN_FUNC("Malfunctioning hook.");
            goto ret;
        }
    }
    
    ws->ionws.split_tree=create_split_regnode(&REGION_GEOM(ws), reg);
    if(ws->ionws.split_tree!=NULL){
        region_fit(reg, &REGION_GEOM(ws), REGION_FIT_BOUNDS);
        ionws_managed_add(&(ws->ionws), reg);
    }

ret:
    extl_unref_table(p.config);
    return;
}


/*}}}*/


/*{{{ The main dynfun */


static WFrame *mke_frame=NULL;

static WRegion *mke(WWindow *par, const WFitParams *fp)
{
    assert(mke_frame!=NULL);
    /*rectangle_debugprint(&(fp->g), "rg");*/
    region_fitrep((WRegion*)mke_frame, par, fp);
    return (WRegion*)mke_frame;
}



bool autows_manage_clientwin(WAutoWS *ws, WClientWin *cwin,
                             const WManageParams *param, int redir)
{
    WRegion *target=NULL;
    WWindow *par=REGION_PARENT_CHK(ws, WWindow);
    
    if(par==NULL)
        return FALSE;
    
    if(!WS_REALLY_FULL(ws)){
        WFrame *frame=create_frame_for(ws, (WRegion*)cwin);
     
        if(frame!=NULL && ws->ionws.split_tree==NULL){
            create_initial_configuration(ws, (WRegion*)frame);
            
            if(REGION_MANAGER(frame)==NULL){
                destroy_obj((Obj*)frame);
                warn("Unable to initialise layout for a new window.");
            }else{
                target=(WRegion*)frame;
            }
        }else if(frame!=NULL){
            Res rs={NULL, PENALTY_INIT, SPLIT_VERTICAL, PRIMN_ANY, FALSE};
            
            split_update_bounds(ws->ionws.split_tree, TRUE);
            scan(ws->ionws.split_tree, 0, 0, 0, &REGION_GEOM(frame), &rs);
            
            if(rs.split!=NULL && rs.dir==SPLIT_UNUSED){
                fprintf(stderr, "use unused!\n");
                if(split_tree_set_node_of((WRegion*)frame, rs.split)){
                    target=(WRegion*)frame;
                    rs.split->type=SPLIT_REGNODE;
                    rs.split->u.reg=target;
                    region_fit(target, &(rs.split->geom), REGION_FIT_EXACT);
                }
            }else if(rs.split!=NULL){
                fprintf(stderr, "split %d\n", rs.dir==SPLIT_VERTICAL);
                
                WSplit *node;
                int mins=(rs.dir==SPLIT_VERTICAL 
                          ? REGION_GEOM(frame).h
                          : REGION_GEOM(frame).w);
                /* Should resize first! */
                assert(mke_frame==NULL);
                mke_frame=frame;
                /*fprintf(stderr, "vert: %d\n", rs.dir==SPLIT_VERTICAL);
                rectangle_debugprint(&REGION_GEOM(frame), "fIg");
                rectangle_debugprint(&(rs.split->geom), "sg");*/
                node=split_tree_split(&(ws->ionws.split_tree), rs.split, 
                                      rs.dir, rs.primn, mins, mke, par);
                /*rectangle_debugprint(&(rs.split->geom), "sRg");*/
                mke_frame=NULL;
                if(node!=NULL)
                    target=(WRegion*)frame;
            }
            
            if(target==NULL)
                destroy_obj((Obj*)frame);
            else
                ionws_managed_add(&(ws->ionws), target);

        }
    }

    if(target!=NULL && REGION_MANAGER(target)==(WRegion*)ws){
        if(region_manage_clientwin(target, cwin, param, 
                                   MANAGE_REDIR_PREFER_YES))
            return TRUE;
    }

    target=find_suitable_target(&(ws->ionws));
    
    if(target!=NULL){
        return region_manage_clientwin(target, cwin, param, 
                                       MANAGE_REDIR_PREFER_YES);
    }
    
err:
    warn("Ooops... could not find a region to attach client window to "
         "on workspace %s.", region_name((WRegion*)ws));
    return FALSE;
}


/*}}}*/

