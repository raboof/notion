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


enum{
    ACT_NOT_FOUND,
    ACT_SPLIT_VERTICAL,
    ACT_SPLIT_HORIZONTAL,
    ACT_REPLACE_UNUSED,
    ACT_ATTACH
};

typedef struct{
    WSplit *split;
    int action;
    int penalty;
    int split_primn;
    int dest_w, dest_h;
} Res;

#define PENALTY_NEVER    INT_MAX
#define PENALTY_INIT     (INT_MAX-1)
#define PENALTY_ENDSCALE (-1)

#define REGFIT_FIT_OFFSET 10

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


static int fit_penalty(int s, int ss)
{
    int d=ss-s;
    
    return interp1(fit_penalty_scale, ss-s);
}

#define hfit_penalty(S1, S2, U, DS) fit_penalty(S1, S2)
#define vfit_penalty(S1, S2, U, DS) fit_penalty(S1, S2)


static int slack_penalty(int s, int ss, int slack, int islack)
{
    int ds=ss-s;
    
    if(ds>=0)
        return PENALTY_NEVER; /* fit will do */
    
    /* If there's enough "immediate" slack, don't penalise more
     * than normal fit would penalise stretching ss to s.
     */
    if(-ds<=islack)
        return interp1(fit_penalty_scale, -ds);
    
    return interp1(slack_penalty_scale, -ds);

}

#define hslack_penalty(S1, S2, U, DS) \
    slack_penalty(S1, S2, (U)->tot_h, (U)->l+(U)->r)
#define vslack_penalty(S1, S2, U, DS) \
    slack_penalty(S1, S2, (U)->tot_v, (U)->t+(U)->b)


static int split_penalty(int s, int ss, int slack)
{
    return interp1(split_penalty_scale, ss-s);
}

#define hsplit_penalty(S1, S2, U, DS) split_penalty(S1, S2, (U)->tot_h)
#define vsplit_penalty(S1, S2, U, DS) split_penalty(S1, S2, (U)->tot_v)


static int regfit_penalty(int s, int ss)
{
    int d=ss-s;
    int res=interp1(fit_penalty_scale, ss-s);
    return (res==PENALTY_NEVER ? res : res+REGFIT_FIT_OFFSET);
}

#define hregfit_penalty(S1, S2) regfit_penalty(S1, S2)
#define vregfit_penalty(S1, S2) regfit_penalty(S1, S2)


static int combine(int h, int v)
{
    if(h==PENALTY_NEVER || v==PENALTY_NEVER)
        return PENALTY_NEVER;
    
    return h+v;
}


/* Only split orthogonally to deepest previous 'static' split? */

enum{ 
    P_FIT=0, 
    P_SLACK=1,
    P_SPLIT=2,
    P_N=3
};

static void scan(WSplit *split, int depth, const WRectangle *geom,
                 const WSplitUnused *unused, Res *best)
{
    if(split->type==SPLIT_UNUSED){
        int d_w[P_N]={-1, -1, -1};
        int d_h[P_N]={-1, -1, -1};
        int p, p_h[P_N], p_v[P_N];
        int rqw=geom->w, rqh=geom->h, sw=split->geom.w, sh=split->geom.h;
        int i, j;
        
        p_h[P_FIT]=hfit_penalty(rqw, sw, unused, &d_w[P_FIT]);
        p_v[P_FIT]=vfit_penalty(rqh, sh, unused, &d_h[P_FIT]);
        p_h[P_SLACK]=hslack_penalty(rqw, sw, unused, d_w[P_SLACK]);
        p_v[P_SLACK]=vslack_penalty(rqh, sh, unused, d_h[P_SLACK]);
        p_h[P_SPLIT]=hsplit_penalty(rqw, sw, unused, d_w[P_SPLIT]);
        p_v[P_SPLIT]=vsplit_penalty(rqh, sh, unused, d_h[P_SPLIT]);
        
        for(i=0; i<P_N; i++){
            for(j=0; j<P_N; j++){
                /* Don't split both ways. */
                if(i==P_SPLIT && j==P_SPLIT)
                    continue;
                
                p=combine(p_h[i], p_v[j]);
                if(p<best->penalty){
                    best->split=split;
                    best->penalty=p;
                    best->dest_w=d_w[i];
                    best->dest_h=d_h[j];
                
                    if(i==P_SPLIT || j==P_SPLIT){
                        best->action=(i==P_SPLIT
                                      ? ACT_SPLIT_HORIZONTAL
                                      : ACT_SPLIT_VERTICAL);
                         /* Should decide this based on location of split */
                        best->split_primn=PRIMN_TL;
                    }else{
                        best->action=ACT_REPLACE_UNUSED;
                    }
                }
            }
        }
        
    }else if(split->type==SPLIT_REGNODE){
        int rqw=geom->w, rqh=geom->h, sw=split->geom.w, sh=split->geom.h;
        int p_h=hregfit_penalty(rqw, sw);
        int p_v=vregfit_penalty(rqh, sh);
        int p=combine(p_h, p_v);

        if(p<best->penalty && OBJ_IS(split->u.reg, WFrame)){
            best->split=split;
            best->penalty=p;
            best->dest_w=-1;
            best->dest_h=-1;
            best->action=ACT_ATTACH;
        }
        
    }else if(split->type==SPLIT_VERTICAL || split->type==SPLIT_HORIZONTAL){
        WSplit *tl=split->u.s.tl, *br=split->u.s.br;
        WSplitUnused tlunused, brunused, un;

        split_get_unused(tl, &tlunused);
        split_get_unused(br, &brunused);
        if(split->type==SPLIT_VERTICAL){
            un=*unused;
            UNUSED_B_ADD(un, brunused, br);
            un.tot_v=brunused.tot_v;
            scan(tl, depth+1, geom, &un, best);
            un=*unused;
            UNUSED_T_ADD(un, tlunused, tl);
            un.tot_v=tlunused.tot_v;
            scan(br, depth+1, geom, &un, best);
        }else if(split->type==SPLIT_HORIZONTAL){
            un=*unused;
            UNUSED_R_ADD(un, brunused, br);
            un.tot_h=brunused.tot_h;
            scan(tl, depth+1, geom, &un, best);
            un=*unused;
            UNUSED_L_ADD(un, tlunused, tl);
            un.tot_h=tlunused.tot_h;
            scan(br, depth+1, geom, &un, best);
        }
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

    p.ws=ws;
    p.reg=reg;
    p.config=extl_table_none();
    
    hook_call_alt_p(autows_init_layout_alt, &p, 
                    (WHookMarshallExtl*)mrsh_init_extl);
    
    if(ws->ionws.split_tree!=NULL){
        WARN_FUNC("Malfunctioning hook/#1.");
        goto ret;
    }

    if(p.config!=extl_table_none()){
        ws->ionws.split_tree=ionws_load_node(&(ws->ionws), &REGION_GEOM(ws), 
                                             p.config);
        
        if(ws->ionws.split_tree!=NULL){
            if(REGION_MANAGER(reg)!=(WRegion*)ws)
                WARN_FUNC("Malfunctioning hook/#2.");
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


/*{{{ Split/replace unused code */


static bool do_replace(Res *rs, WSplit **tree, WFrame *frame)
{
    if(split_tree_set_node_of((WRegion*)frame, rs->split)){
        rs->split->type=SPLIT_REGNODE;
        rs->split->u.reg=(WRegion*)frame;
        region_fit((WRegion*)frame, &(rs->split->geom), REGION_FIT_EXACT);
        return TRUE;
    }
    
    return FALSE;
}



static WFrame *mke_frame=NULL;

static WRegion *mke(WWindow *par, const WFitParams *fp)
{
    assert(mke_frame!=NULL);
    /*rectangle_debugprint(&(fp->g), "rg");*/
    region_fitrep((WRegion*)mke_frame, par, fp);
    return (WRegion*)mke_frame;
}


static bool do_split(Res *rs, WSplit **tree, WFrame *frame)
{
    WSplit *node;
    int dir=(rs->action==ACT_SPLIT_VERTICAL
             ? SPLIT_VERTICAL
             : SPLIT_HORIZONTAL);
    int mins=(rs->action==ACT_SPLIT_VERTICAL 
              ? REGION_GEOM(frame).h
              : REGION_GEOM(frame).w);
    WWindow *par=REGION_PARENT_CHK(frame, WWindow);
    
    assert(mke_frame==NULL);
    mke_frame=frame;
    node=split_tree_split(tree, rs->split, dir, rs->split_primn, 
                          mins, mke, par);
    if(node!=NULL)
        node->parent->is_lazy=TRUE;
    
    mke_frame=NULL;
    return (node!=NULL);
}

/*}}}*/


/*{{{ The main dynfun */


bool autows_manage_clientwin(WAutoWS *ws, WClientWin *cwin,
                             const WManageParams *param, int redir)
{
    WRegion *target=NULL;
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
        /*      split action         penalty       primn      w    h */
        Res rs={NULL, ACT_NOT_FOUND, PENALTY_INIT, PRIMN_ANY, -1, -1};
        WSplitUnused unused={0, 0, 0, 0, 0, 0};
        WSplit **tree=&(ws->ionws.split_tree);
        
        split_update_bounds(*tree, TRUE);
        scan(*tree, 0, &REGION_GEOM(frame), &unused, &rs);
        
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
        
        /* Put the frame there */
        if(rs.action==ACT_SPLIT_HORIZONTAL || 
           rs.action==ACT_SPLIT_VERTICAL){
            if(do_split(&rs, tree, frame))
                target=(WRegion*)frame;
        }else if(rs.action==ACT_REPLACE_UNUSED){
            if(do_replace(&rs, tree, frame))
                target=(WRegion*)frame;
        }else if(rs.action==ACT_ATTACH){
            target=rs.split->u.reg;
        }else{
            warn("Placement method unimplemented.");
        }
        
        if(target!=(WRegion*)frame)
            destroy_obj((Obj*)frame);
        else
            ionws_managed_add(&(ws->ionws), (WRegion*)frame);
        
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

