/*
 * ion/mod_ionws/split.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <string.h>
#include <X11/Xmd.h>

#include <libtu/minmax.h>
#include <libtu/rb.h>
#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/resize.h>
#include <ioncore/attach.h>
#include <ioncore/defer.h>
#include <ioncore/manage.h>
#include <ioncore/extlconv.h>
#include <ioncore/region-iter.h>
#include <ioncore/rectangle.h>
#include "ionws.h"
#include "split.h"


IMPLCLASS(WSplit, Obj, split_deinit, NULL);

#define CHKNODE(NODE)                                              \
    assert(((NODE)->type==SPLIT_REGNODE && (NODE)->u.reg!=NULL) || \
           ((NODE)->type==SPLIT_UNUSED) ||                         \
           (((NODE)->type==SPLIT_VERTICAL ||                       \
             (NODE)->type==SPLIT_HORIZONTAL)                       \
            && ((NODE)->u.s.tl!=NULL && (NODE)->u.s.br!=NULL)))

#define CHKSPLIT(NODE)                                          \
    assert((NODE)->type!=SPLIT_REGNODE &&                       \
           ((NODE)->u.s.tl!=NULL && (NODE)->u.s.br!=NULL));

static Rb_node split_of_map=NULL;


/*{{{ Geometry helper functions */


int split_size(WSplit *split, int dir)
{
    return (dir==SPLIT_HORIZONTAL ? split->geom.w : split->geom.h);
}

int split_other_size(WSplit *split, int dir)
{
    return (dir==SPLIT_VERTICAL ? split->geom.w : split->geom.h);
}

int split_pos(WSplit *split, int dir)
{
    return (dir==SPLIT_HORIZONTAL ? split->geom.x : split->geom.y);
}

int split_other_pos(WSplit *split, int dir)
{
    return (dir==SPLIT_VERTICAL ? split->geom.x : split->geom.y);
}



static int reg_calcresize(WRegion *reg, int dir, int nsize)
{
    int tmp;
    
    if(dir==SPLIT_HORIZONTAL)
        tmp=region_min_w(reg);
    else
        tmp=region_min_h(reg);
    
    return (nsize<tmp ? tmp : nsize);
}


/* No, these are not even supposed to be proper/consistent 
 * Z \cup {\infty, -\infty} calculation rules. 
 */

static int infadd(int x, int y)
{
    if(x==INT_MAX || y==INT_MAX)
        return INT_MAX;
    else
        return x+y;
}

static int infsub(int x, int y)
{
    if(x==INT_MAX)
        return INT_MAX;
    else if(y==INT_MAX)
        return 0;
    else
        return x-y;
}


static void bound(int *what, int min, int max)
{
    if(*what<min)
        *what=min;
    else if(*what>max)
        *what=max;
}


static void get_minmaxused(WSplit *node, int dir, 
                           int *min, int *max, int *used)
{
    if(dir==SPLIT_VERTICAL){
        *min=node->min_h;
        *max=maxof(*min, node->max_h);
        *used=node->used_h;
    }else{
        *min=node->min_w;
        *max=maxof(*min, node->max_w);
        *used=node->used_w;
    }
}


/*}}}*/


/*{{{ Functions to get and set a region's containing node */


#define node_of_reg split_tree_node_of

WSplit *split_tree_node_of(WRegion *reg)
{
    Rb_node node=NULL;
    int found=0;
    
    /*assert(REGION_MANAGER_CHK(reg, WIonWS)!=NULL);*/
    
    if(split_of_map!=NULL){
        node=rb_find_pkey_n(split_of_map, reg, &found);
        if(found)
            return (WSplit*)(node->v.val);
    }
    
    return NULL;
}


WSplit *split_tree_split_of(WRegion *reg)
{
    WSplit *node=node_of_reg(reg);
    if(node!=NULL)
        return node->parent;
    return NULL;
}


#define set_node_of_reg split_tree_set_node_of


bool split_tree_set_node_of(WRegion *reg, WSplit *split)
{
    Rb_node node=NULL;
    int found;
    
    /*assert(REGION_MANAGER_CHK(reg, WIonWS)!=NULL);*/
    
    if(split_of_map==NULL){
        if(split==NULL)
            return TRUE;
        split_of_map=make_rb();
        if(split_of_map==NULL){
            warn_err();
            return FALSE;
        }
    }
    
    node=rb_find_pkey_n(split_of_map, reg, &found);
    if(found)
        rb_delete_node(node);
    
    return (rb_insertp(split_of_map, reg, split)!=NULL);
}


/*}}}*/


/*{{{ Deinit */


void split_deinit(WSplit *split)
{
    assert(split->parent==NULL);

    if(split->type==SPLIT_REGNODE){
        if(split->u.reg!=NULL)
            set_node_of_reg(split->u.reg, NULL);
    }else if(split->type==SPLIT_VERTICAL || split->type==SPLIT_HORIZONTAL){
        if(split->u.s.tl!=NULL){
            split->u.s.tl->parent=NULL;
            destroy_obj((Obj*)(split->u.s.tl));
        }
        if(split->u.s.br!=NULL){
            split->u.s.br->parent=NULL;
            destroy_obj((Obj*)(split->u.s.br));
        }
    }
}

    
/*}}}*/


/*{{{ Size bounds management */


static void split_update_region_bounds(WSplit *node, WRegion *reg)
{
    XSizeHints hints;
    uint relw, relh;
    region_resize_hints(reg, &hints, &relw, &relh);
    
    node->used_w=node->geom.w;/*REGION_GEOM(reg).w;*/
    node->min_w=maxof(1, ((hints.flags&PMinSize ? hints.min_width : 1)
                          +REGION_GEOM(reg).w-relw));
    node->max_w=INT_MAX;

    node->used_h=node->geom.h;/*REGION_GEOM(reg).h;*/
    node->min_h=maxof(1, ((hints.flags&PMinSize ? hints.min_height : 1)
                          +REGION_GEOM(reg).h-relh));
    node->max_h=INT_MAX;
}


void split_update_bounds(WSplit *node, bool recursive)
{
    int tlmax, tlmin, brmax, brmin;
    WSplit *tl, *br;
    
    CHKNODE(node);
    
    if(node->type==SPLIT_REGNODE){
        split_update_region_bounds(node, node->u.reg);
        return;
    }
    
    if(node->type==SPLIT_UNUSED){
        node->used_w=0;
        node->used_h=0;
        node->min_w=0;
        node->min_h=0;
        node->max_w=INT_MAX;
        node->max_h=INT_MAX;
        return;
    }
    
    tl=node->u.s.tl;
    br=node->u.s.br;
    
    if(recursive){
        split_update_bounds(tl, TRUE);
        split_update_bounds(br, TRUE);
    }
    
    if(node->type==SPLIT_HORIZONTAL){
        node->used_w=infadd(tl->used_w, br->used_w);
        node->max_w=infadd(tl->max_w, br->max_w);
        node->min_w=infadd(tl->min_w, br->min_w);
        node->used_h=maxof(tl->used_h, br->used_h);
        node->max_h=maxof(tl->max_h, br->max_h);
        node->min_h=maxof(tl->min_h, br->min_h);
    }else{
        node->used_h=infadd(tl->used_h, br->used_h);
        node->max_h=infadd(tl->max_h, br->max_h);
        node->min_h=infadd(tl->min_h, br->min_h);
        node->used_w=maxof(tl->used_w, br->used_w);
        node->max_w=maxof(tl->max_w, br->max_w);
        node->min_w=maxof(tl->min_w, br->min_w);
    }
}


/*}}}*/


/*{{{ Low-level resize code */


static int other_dir(int dir)
{
    return (dir==SPLIT_VERTICAL ? SPLIT_HORIZONTAL : SPLIT_VERTICAL);
}


static void adjust_sizes(int *tls_, int *brs_, int nsize, int sz, 
                         int tlmin, int brmin, int tlmax, int brmax,
                         int primn)
{
    int tls=*tls_;
    int brs=*brs_;
    
    if(primn==PRIMN_TL){
        tls=tls+nsize-sz;
        bound(&tls, tlmin, tlmax);
        brs=nsize-tls;
        bound(&brs, brmin, brmax);
        tls=nsize-brs;
        bound(&tls, tlmin, tlmax);
    }else if(primn==PRIMN_BR){
        brs=brs+nsize-sz;
        bound(&brs, brmin, brmax);
        tls=nsize-brs;
        bound(&tls, tlmin, tlmax);
        brs=nsize-tls;
        bound(&brs, brmin, brmax);
    }else{ /* && PRIMN_ANY */
        tls=tls*nsize/sz;
        bound(&tls, tlmin, tlmax);
        brs=nsize-tls;
        bound(&brs, brmin, brmax);
        tls=nsize-brs;
        bound(&tls, tlmin, tlmax);
    }
    
    *tls_=tls;
    *brs_=brs;
}


bool split_do_resize(WSplit *node, const WRectangle *ng, 
                     int hprimn, int vprimn, bool transpose)
{
    bool ret=TRUE;
    
    CHKNODE(node);

    assert(!transpose || (hprimn==PRIMN_ANY && vprimn==PRIMN_ANY));
    
    if(node->type==SPLIT_REGNODE){
        region_fit(node->u.reg, ng, REGION_FIT_EXACT);
    }else if(node->type!=SPLIT_UNUSED){
        WSplit *tl=node->u.s.tl, *br=node->u.s.br;
        int sz=split_size(node, node->type);
        int tls=split_size(tl, node->type);
        int brs=split_size(br, node->type);
        int dir=(transpose ? other_dir(node->type) : node->type);
        int nsize=(dir==SPLIT_VERTICAL ? ng->h : ng->w);
        int primn=(dir==SPLIT_VERTICAL ? vprimn : hprimn);
        int tlmin, tlmax, tlused, brmin, brmax, brused;
        WRectangle tlg=*ng, brg=*ng;

        get_minmaxused(tl, dir, &tlmin, &tlmax, &tlused);
        get_minmaxused(br, dir, &brmin, &brmax, &brused);
        /* tlmin,  brmin >= 1 => (tls>=tlmin, brs>=brmin => sz>0) */

        if(sz>2){
            if(nsize>=tlused+brused){
                /* Just remove slack if shrinking */
                adjust_sizes(&tls, &brs, nsize, sz, 
                             tlused, brused, tlmax, brmax,
                             primn);
            }else{
                /* Actual shrink needed */
                adjust_sizes(&tls, &brs, nsize, sz, 
                             tlmin, brmin, tlmax, brmax,
                             primn);
            }
        }

        if(tls+brs!=nsize){
            /* Bad fit; just size proportionally. */
            if(sz<=2){
                tls=nsize/2;
                brs=nsize-tls;
            }else{
                tls=split_size(tl, node->type)*nsize/sz;
                brs=nsize-tls;
            }
        }
        
        if(dir==SPLIT_VERTICAL){
            tlg.h=tls;
            brg.y+=tls;
            brg.h=brs;
        }else{
            tlg.w=tls;
            brg.x+=tls;
            brg.w=brs;
        }

        split_do_resize(tl, &tlg, hprimn, vprimn, transpose);
        split_do_resize(br, &brg, hprimn, vprimn, transpose);
        
        node->type=dir;
    }
    
    node->geom=*ng;
    split_update_bounds(node, FALSE);
    
    return ret;
}


void split_resize(WSplit *node, const WRectangle *ng, int hprimn, int vprimn)
{
    split_update_bounds(node, TRUE);
    split_do_resize(node, ng, hprimn, vprimn, FALSE);
}


typedef struct{
    int tltot, brtot;
    int tlforce, brforce;
    bool any;
} RootwardAmount;


static void flexibility(WSplit *node, int dir, int *unused, int *force,
                        int *stretch)
{
    if(dir==SPLIT_VERTICAL){
        *force=maxof(0, node->geom.h-node->min_h);
        *unused=maxof(0, node->geom.h-node->used_h);
        *stretch=INT_MAX;
    }else{
        *force=maxof(0, node->geom.w-node->min_w);
        *unused=maxof(0, node->geom.w-node->used_w);
        *stretch=INT_MAX;
    }
}


static void calc_amount(int *amountunused, int *amountforce,
                        int tot, int force, WSplit *other, int dir)
{
    int unusedfree, forcefree, stretch;
    
    flexibility(other, dir, &unusedfree, &forcefree, &stretch);

    *amountforce=0;
    *amountunused=0;
    
    if(tot>0){
        *amountunused=minof(unusedfree, tot);
        if(*amountunused<tot && force>0){
            *amountforce=minof(minof(force, tot-*amountunused),
                               forcefree-unusedfree);
        }
    }else if(tot<0){
        *amountunused=-minof(-tot, stretch);
    }
}


static void split_do_resize_rootward(WSplit *node, RootwardAmount *ha,
                                     RootwardAmount *va, WRectangle *rg,
                                     bool tryonly)
{
    WSplit *p=node->parent, *other;
    int amountunused, amountforce, amount;
    WRectangle og, pg, ng;
    RootwardAmount *ca;
    int thisnode;
    
    assert(!ha->any || ha->tltot==0);
    assert(!va->any || va->tltot==0);
    
    if(p==NULL){
        *rg=node->geom;
        return;
    }

    if(p->u.s.tl==node){
        other=p->u.s.br;
        thisnode=PRIMN_TL;
    }else{
        other=p->u.s.tl;
        thisnode=PRIMN_BR;
    }
    
    ca=(p->type==SPLIT_VERTICAL ? va : ha);

    if(thisnode==PRIMN_TL || ca->any){
        calc_amount(&amountunused, &amountforce,
                    ca->brtot, ca->brforce, other, p->type);
        ca->brtot-=amountunused;
        ca->brforce-=amountforce;
    }else/*if(thisnode==PRIMN_BR)*/{
        calc_amount(&amountunused, &amountforce,
                    ca->tltot, ca->tlforce, other, p->type);
        ca->tltot-=amountunused;
        ca->tlforce-=amountforce;
    }
    
    amount=amountunused+amountforce;

    split_do_resize_rootward(p, ha, va, &pg, tryonly);
    
    og=pg;
    ng=pg;
    
    if(p->type==SPLIT_VERTICAL){
        og.h=other->geom.h-amount;
        ng.h=pg.h-og.h;
        if(thisnode==PRIMN_TL)
            og.y=pg.y+pg.h-og.h;
        else
            ng.y=pg.y+pg.h-ng.h;
    }else{
        og.w=other->geom.w-amount;
        ng.w=pg.w-og.w;
        if(thisnode==PRIMN_TL)
            og.x=pg.x+pg.w-og.w;
        else
            ng.x=pg.x+pg.w-ng.w;
    }
    
    if(!tryonly){
        split_do_resize(other, &og, PRIMN_ANY, PRIMN_ANY, FALSE);
        p->geom=pg;
    }
    
    *rg=ng;
}


static void initra(RootwardAmount *ra, int p, int s, int op, int os, 
                   bool any)
{
    ra->any=any;
    ra->tlforce=0;
    ra->brforce=0;
    ra->tltot=op-p;
    ra->brtot=(p+s)-(op+os);
    if(any){
        ra->brtot+=ra->tltot;
        ra->tltot=0;
    }
}


static void updatera(RootwardAmount *ra, int p, int s, int op, int os, 
                     bool any)
{
    int tlf=ra->tltot, brf=ra->brtot;
    initra(ra, p, s, op, os, any);
    ra->tlforce=tlf;
    ra->brforce=brf;
}


static void split_resize_rootward_(WSplit *node, const WRectangle *ng, 
                                   bool hany, bool vany, bool tryonly,
                                   WRectangle *rg)
{
    RootwardAmount ha, va;

    initra(&ha, ng->x, ng->w, node->geom.x, node->geom.w, hany);
    initra(&va, ng->y, ng->h, node->geom.y, node->geom.h, vany);
    
    split_do_resize_rootward(node, &ha, &va, rg, TRUE);
    
    updatera(&ha, ng->x, ng->w, node->geom.x, node->geom.w, hany);
    updatera(&va, ng->y, ng->h, node->geom.y, node->geom.h, vany);

    split_do_resize_rootward(node, &ha, &va, rg, tryonly);
}


void split_resize_rootward(WSplit *node, const WRectangle *ng, 
                           bool hany, bool vany, bool tryonly,
                           WRectangle *rg)
{
    WRectangle rg_;
    
    if(rg==NULL)
        rg=&rg_;

    split_resize_rootward_(node, ng, hany, vany, tryonly, rg);
    
    if(!tryonly)
        split_do_resize(node, rg, PRIMN_ANY, PRIMN_ANY, FALSE);
}


/*}}}*/


/*{{{ Resize interface */


static void bnd(int *pos, int *sz, int opos, int osz, int minsz, int maxsz)
{
    int ud=abs(*pos-opos);
    int dd=abs((*pos+*sz)-(opos+osz));
    int szrq=*sz;
    
    if(ud+dd!=0){
        bound(sz, minsz, maxsz);
        *pos+=(szrq-*sz)*ud/(ud+dd);
    }
}


void split_tree_rqgeom(WSplit *root, WSplit *sub, int flags, 
                       const WRectangle *geom_, WRectangle *geomret)
{
    bool hany=flags&REGION_RQGEOM_WEAK_X;
    bool vany=flags&REGION_RQGEOM_WEAK_Y;
    WRectangle geom=*geom_;

    split_update_bounds(root, TRUE);
    /*split_update_bounds(sub, TRUE);*/

    /* Handle internal size bounds */
    bnd(&(geom.x), &(geom.w), sub->geom.x, sub->geom.w, 
        sub->min_w, sub->max_w);
    bnd(&(geom.y), &(geom.h), sub->geom.y, sub->geom.h, 
        sub->min_h, sub->max_h);

    /* Check if we should resize to both tl and br */
    
    if(hany){
        geom.w+=sub->geom.x-geom.x;
        geom.x=sub->geom.x;
    }

    if(vany){
        geom.h+=sub->geom.y-geom.y;
        geom.y=sub->geom.y;
    }
    
    split_resize_rootward(sub, &geom, hany, vany, 
                          flags&REGION_RQGEOM_TRYONLY, geomret);
}
                          

/*}}}*/


/*{{{ Split */


static WSplit *do_create_split(const WRectangle *geom)
{
    WSplit *split=ALLOC(WSplit);
    
    if(split==NULL){
        warn_err();
    }else{
        OBJ_INIT(split, WSplit);
        split->parent=NULL;
        split->geom=*geom;
        split->min_w=0;
        split->min_h=0;
        split->max_w=INT_MAX;
        split->max_h=INT_MAX;
        split->used_w=0;
        split->used_h=0;
    }
    return split;
}
        

WSplit *create_split(const WRectangle *geom, int dir, WSplit *tl, WSplit *br)
{
    WSplit *split=do_create_split(geom);
    
    if(split!=NULL){
        split->type=dir;
        split->u.s.tl=tl;
        split->u.s.br=br;
        split->u.s.current=0;
    }
    
    return split;
}


WSplit *create_split_regnode(const WRectangle *geom, WRegion *reg)
{
    WSplit *split=do_create_split(geom);
    
    if(split!=NULL){
        split->type=SPLIT_REGNODE;
        split->u.reg=reg;
        set_node_of_reg(reg, split);
    }
    
    return split;
}


WSplit *create_split_unused(const WRectangle *geom)
{
    WSplit *split=do_create_split(geom);
    
    if(split!=NULL)
        split->type=SPLIT_UNUSED;
    
    return split;
}


WSplit *split_tree_split(WSplit **root, WSplit *node, int dir, int primn,
                         int minsize, WRegionSimpleCreateFn *fn, 
                         WWindow *parent)
{
    int objmin, objmax;
    int s, sn, so, pos;
    WSplit *psplit, *nsplit, *nnode;
    WRegion *nreg;
    WFitParams fp;
    WRectangle ng, rg;
    
    assert(root!=NULL && *root!=NULL && node!=NULL && parent!=NULL);
    
    if(primn!=PRIMN_TL && primn!=PRIMN_BR)
        primn=PRIMN_BR;
    if(dir!=SPLIT_HORIZONTAL && dir!=SPLIT_VERTICAL)
        dir=SPLIT_VERTICAL;

    split_update_bounds(node, TRUE);
    objmin=(dir==SPLIT_VERTICAL ? node->min_h : node->min_w);

    s=split_size(node, dir);
    sn=maxof(minsize, s/2);
    so=maxof(objmin, s-sn);

    
    if(sn+so!=s){
        int rs;
        ng=node->geom;
        if(dir==SPLIT_VERTICAL)
            ng.h=sn+so;
        else
            ng.w=sn+so;
        split_resize_rootward_(node, &ng, TRUE, TRUE, TRUE, &rg);
        rs=(dir==SPLIT_VERTICAL ? rg.h : rg.w);
        if(rs<minsize+objmin){
            warn("Unable to split: not enough free space.");
            return NULL;
        }
        split_resize_rootward_(node, &ng, TRUE, TRUE, FALSE, &rg);
        rs=(dir==SPLIT_VERTICAL ? rg.h : rg.w);
        if(minsize>rs/2){
            sn=minsize;
            so=rs-sn;
        }else{
            so=maxof(rs/2, objmin);
            sn=rs-so;
        }
    }else{
        rg=node->geom;
    }

    /* Create split and new window
     */
    fp.mode=REGION_FIT_BOUNDS;
    fp.g=rg;
    
    nsplit=create_split(&(fp.g), dir, NULL, NULL);
    
    if(nsplit==NULL)
        return NULL;

    if(dir==SPLIT_VERTICAL){
        if(primn==PRIMN_BR)
            fp.g.y+=so;
        fp.g.h=sn;
    }else{
        if(primn==PRIMN_BR)
            fp.g.x+=so;
        fp.g.w=sn;
    }
    
    nreg=fn(parent, &fp);
    
    if(nreg==NULL){
        free(nsplit);
        return NULL;
    }

    nnode=create_split_regnode(&(fp.g), nreg);
    if(nnode==NULL){
        warn_err();
        destroy_obj((Obj*)nreg);
        free(nsplit);
        return NULL;
    }
    
    /* Now that everything's ok, resize and move original node.
     */    
    ng=rg;
    if(dir==SPLIT_VERTICAL){
        ng.h=so;
        if(primn==PRIMN_TL)
            ng.y+=sn;
    }else{
        ng.w=so;
        if(primn==PRIMN_TL)
            ng.x+=sn;
    }
    
    split_do_resize(node, &ng, 
                    (dir==SPLIT_HORIZONTAL ? primn : PRIMN_ANY),
                    (dir==SPLIT_VERTICAL ? primn : PRIMN_ANY),
                    FALSE);
    
    /* Set up split structure
     */
    psplit=node->parent;
    node->parent=nsplit;
    nnode->parent=nsplit;
    
    if(primn==PRIMN_BR){
        nsplit->u.s.tl=node;
        nsplit->u.s.br=nnode;
    }else{
        nsplit->u.s.tl=nnode;
        nsplit->u.s.br=node;
    }
    
    if(psplit!=NULL){
        if(node==psplit->u.s.tl)
            psplit->u.s.tl=nsplit;
        else
            psplit->u.s.br=nsplit;
        nsplit->parent=psplit;
    }else{
        *root=nsplit;
    }
    
    return nnode;
}


/*}}}*/


/*{{{ Remove */


static bool split_tree_remove_split(WSplit **root, WSplit *split, 
                                    int primn, bool reclaim_space)
{
    WSplit *split2, *other;
    int osize, nsize, npos;
    
    if(primn==PRIMN_BR)
        other=split->u.s.br;
    else
        other=split->u.s.tl;

    split2=split->parent;
    
    if(split2!=NULL){
        if(split==split2->u.s.tl)
            split2->u.s.tl=other;
        else
            split2->u.s.br=other;
    }else{
        *root=other;
    }
    
    if(other==NULL)
        return FALSE;
    
    other->parent=split2;
    
    if(reclaim_space)
        split_resize(other, &(split->geom), PRIMN_ANY, PRIMN_ANY);
    
    split->u.s.tl=NULL;
    split->u.s.br=NULL;
    split->parent=NULL;
    destroy_obj((Obj*)split);
    
    return TRUE;
}


WSplit *split_tree_remove(WSplit **root, WSplit *node, bool reclaim_space)
{
    WSplit *split=node->parent;
    WSplit *other=NULL;
    
    if(split!=NULL){
        if(split->u.s.tl==node){
            split->u.s.tl=NULL;
            other=split_current_tl(split->u.s.br, split->type);
            split_tree_remove_split(root, split, PRIMN_BR, reclaim_space);
        }else{
            split->u.s.br=NULL;
            other=split_current_br(split->u.s.tl, split->type);
            split_tree_remove_split(root, split, PRIMN_TL, reclaim_space);
        }
        node->parent=NULL;
    }else{
        *root=NULL;
    }
    destroy_obj((Obj*)node);
    return other;
}


/*}}}*/


/*{{{ iowns_manage_rescue */


static WMPlex *find_mplex_descend(WSplit *node, int primn)
{
    WMPlex *mplex;
    
    do{
        CHKNODE(node);
        
        if(node->type==SPLIT_UNUSED)
            return NULL;
        
        if(node->type==SPLIT_REGNODE)
            return OBJ_CAST(node->u.reg, WMPlex);
        
        if(primn==PRIMN_TL)
            mplex=find_mplex_descend(node->u.s.tl, primn);
        else
            mplex=find_mplex_descend(node->u.s.br, primn);
        
        if(mplex!=NULL)
            return mplex;
        
        if(primn==PRIMN_TL)
            node=node->u.s.br;
        else
            node=node->u.s.tl;
    }while(1);
}


WMPlex *split_tree_find_mplex(WRegion *from)
{
    WMPlex *nmgr=NULL;
    WSplit *split=NULL, *node=NULL;
    
    node=node_of_reg(from);
    if(node!=NULL)
        split=node->parent;
    
    while(split!=NULL){
        if(split->u.s.tl==node)
            nmgr=find_mplex_descend(split->u.s.br, PRIMN_TL);
        else
            nmgr=find_mplex_descend(split->u.s.tl, PRIMN_BR);
        if(nmgr!=NULL)
            break;
        node=split;
        split=split->parent;
    }
    
    return nmgr;
}


/*}}}*/


/*{{{ Tree traversal */


WSplit *split_current_tl(WSplit *node, int dir)
{
    WSplit *nnode=NULL;
    
    if(node==NULL)
        return NULL;
    
    CHKNODE(node);
        
    if(node->type==SPLIT_UNUSED){
        /* nothing */
    }else if(node->type==SPLIT_REGNODE){
        nnode=node;
    }else if(node->type==dir || node->u.s.current==0){
        nnode=split_current_tl(node->u.s.tl, dir);
        if(nnode==NULL)
            nnode=split_current_tl(node->u.s.br, dir);
    }else{
        nnode=split_current_tl(node->u.s.br, dir);
        if(nnode==NULL)
            nnode=split_current_tl(node->u.s.tl, dir);
    }
    
    return nnode;
}


WSplit *split_current_br(WSplit *node, int dir)
{
    WSplit *nnode=NULL;
    
    if(node==NULL)
        return NULL;
    
    CHKNODE(node);

    if(node->type==SPLIT_UNUSED){
        /* nothing */
    }else if(node->type==SPLIT_REGNODE){
        nnode=node;
    }else if(node->type==dir || node->u.s.current!=0){
        nnode=split_current_tl(node->u.s.br, dir);
        if(nnode==NULL)
            nnode=split_current_tl(node->u.s.tl, dir);
    }else{
        nnode=split_current_tl(node->u.s.tl, dir);
        if(nnode==NULL)
            nnode=split_current_tl(node->u.s.br, dir);
    }
    
    return nnode;
}



static WSplit *find_split(WSplit *node, int dir, int *from)
{
    WSplit *split=node->parent;
    
    while(split!=NULL){
        CHKSPLIT(split);
        
        if(split->type==dir){
            if(node==split->u.s.tl)
                *from=PRIMN_TL;
            else
                *from=PRIMN_BR;
            break;
        }
        
        node=split;
        split=split->parent;
    }
    
    return split;
}


WSplit *split_to_br(WSplit *node, int dir)
{
    WSplit *split;
    int from;
    
    if(node==NULL)
        return NULL;
    
    while(1){
        split=find_split(node, dir, &from);
        
        if(split==NULL)
            break;
        
        if(from==PRIMN_TL)
            return split_current_tl(split->u.s.br, dir);
        
        node=split;
    }
    
    return NULL;
}


WSplit *split_to_tl(WSplit *node, int dir)
{
    WSplit *split;
    int from;
    
    if(node==NULL)
        return NULL;
    
    while(1){
        split=find_split(node, dir, &from);
        
        if(split==NULL)
            break;
        
        if(from==PRIMN_BR)
            return split_current_br(split->u.s.tl, dir);
        
        node=split;
    }
    
    return NULL;
}



/*}}}*/


/*{{{ Misc. exports */


/*EXTL_DOC
 * Return parent split for \var{split}.
 */
EXTL_EXPORT_MEMBER
WSplit *split_parent(WSplit *split)
{
    return split->parent;
}


Obj *split_hoist(WSplit *split)
{
    if(split->type==SPLIT_UNUSED)
        return NULL;
    if(split->type==SPLIT_REGNODE)
        return (Obj*)split->u.reg;
    return (Obj*)split;
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to top or left
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT_MEMBER
Obj *split_tl(WSplit *split)
{
    CHKNODE(split);
    
    if(split->type==SPLIT_REGNODE || split->type==SPLIT_UNUSED)
        return NULL;
    return split_hoist(split->u.s.tl);
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to bottom or right
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT_MEMBER
Obj *split_br(WSplit *split)
{
    CHKNODE(split);
    
    if(split->type==SPLIT_REGNODE || split->type==SPLIT_UNUSED)
        return NULL;
    return split_hoist(split->u.s.br);
}


/*EXTL_DOC
 * Is \var{split} a vertical split?
 */
EXTL_EXPORT_MEMBER
bool split_is_vertical(WSplit *split)
{
    return (split->type==SPLIT_VERTICAL);
}


/*EXTL_DOC
 * Is \var{split} a horizontal split?
 */
EXTL_EXPORT_MEMBER
bool split_is_horizontal(WSplit *split)
{
    return (split->type==SPLIT_HORIZONTAL);
}


/*EXTL_DOC
 * Returns the area of workspace used by the regions under \var{split}.
 */
EXTL_EXPORT_MEMBER
ExtlTab split_geom(WSplit *split)
{
    return extl_table_from_rectangle(&(split->geom));
}


/*}}}*/


/*{{{ Misc. */


WRegion *split_region_at(WSplit *node, int x, int y)
{
    WRegion *ret;
    
    CHKNODE(node);
    
    if(node->type==SPLIT_UNUSED)
        return NULL;

    if(node->type==SPLIT_REGNODE){
        if(!rectangle_contains(&REGION_GEOM(node->u.reg), x, y))
            return NULL;
        return node->u.reg;
    }
    
    if(!rectangle_contains(&(node->geom), x, y))
        return NULL;
    
    ret=split_region_at(node->u.s.tl, x, y);
    if(ret==NULL)
        ret=split_region_at(node->u.s.br, x, y);
    return ret;
}


void split_mark_current(WSplit *node)
{
    WSplit *split=node->parent;
    
    while(split!=NULL){
        split->u.s.current=(split->u.s.tl==node ? 0 : 1);
        node=split;
        split=split->parent;
    }
}


/*}}}*/


/*{{{ Transpose */


void split_transpose_to(WSplit *node, const WRectangle *geom)
{
    split_update_bounds(node, TRUE);
    split_do_resize(node, geom,  PRIMN_ANY, PRIMN_ANY, TRUE);
}


EXTL_EXPORT_MEMBER
void split_transpose(WSplit *node)
{
    WRectangle g=node->geom;
    
    split_transpose_to(node, &g);
}


/*}}}*/
