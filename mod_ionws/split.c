/*
 * ion/ionws/split.c
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
#include "ionws.h"
#include "split.h"


IMPLCLASS(WSplit, Obj, split_deinit, NULL);

#define CHKNODE(NODE)                                           \
    assert((NODE)->type!=SPLIT_REGNODE || (NODE)->u.reg!=NULL); \
    assert((NODE)->type==SPLIT_REGNODE ||                       \
           ((NODE)->u.s.tl!=NULL && (NODE)->u.s.br!=NULL));

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


static void get_minmax(WSplit *node, int dir, int *min, int *max)
{
    if(dir==SPLIT_VERTICAL){
        *min=node->min_h;
        *max=node->max_h;
    }else{
        *min=node->min_h;
        *max=node->max_h;
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


static bool set_node_of_reg(WRegion *reg, WSplit *split)
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
    }else{
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
    
    node->used_w=REGION_GEOM(reg).w;
    node->min_w=((hints.flags&PMinSize ? hints.min_width : 1)
                 +REGION_GEOM(reg).w-relw);
    node->max_w=INT_MAX;

    node->used_h=REGION_GEOM(reg).h;
    node->min_h=((hints.flags&PMinSize ? hints.min_height : 1)
                 +REGION_GEOM(reg).h-relh);
    node->max_h=INT_MAX;
}


static void split_update_bounds(WSplit *node, bool recursive)
{
    int tlmax, tlmin, brmax, brmin;
    WSplit *tl, *br;
    
    CHKNODE(node);
    
    if(node->type==SPLIT_REGNODE){
        split_update_region_bounds(node, node->u.reg);
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
        node->used_h=maxof(tl->used_w, br->used_w);
        node->max_w=maxof(tl->max_w, br->max_w);
        node->min_w=maxof(tl->min_w, br->min_w);
    }else{
        node->used_h=infadd(tl->used_h, br->used_h);
        node->max_h=infadd(tl->max_h, br->max_h);
        node->min_h=infadd(tl->min_h, br->min_h);
        node->used_h=maxof(tl->used_h, br->used_h);
        node->max_h=maxof(tl->max_h, br->max_h);
        node->min_h=maxof(tl->min_h, br->min_h);
    }
}



/* Get resize bounds for <from> due to <split> and all nodes towards the
 * root. <from> must be a child node of <split> (->tl/br). The <*free>
 * variables indicate the free space in that direction while the <*shrink>
 * variables indicate the amount the object in that direction can grow
 * (INT_MAX means no limit has been set). <minsize> and <maxsize> are
 * size limits set by siblings in splits perpendicular to <dir>.
 */
static void get_bounds(WSplit *split, int dir, WSplit *from,
                       int *tlfree, int *brfree, int *maxsize,
                       int *tlshrink, int *brshrink, int *minsize)
{
    WSplit *other=(from==split->u.s.tl ? split->u.s.br : split->u.s.tl);
    WSplit *parent=split->parent;
    int s=split_size(split, dir);
    int omin, omax;
    
    if(parent==NULL){
        *tlfree=0;
        *brfree=0;
        *maxsize=s;
        *tlshrink=0;
        *brshrink=0;
        *minsize=s;
    }else{
        get_bounds(parent, dir, split, 
                   tlfree, brfree, maxsize,
                   tlshrink, brshrink, minsize);
    }
    
    split_update_bounds(other, TRUE);
    get_minmax(other, dir, &omin, &omax);
    
    if(split->type!=dir){
        if(parent!=NULL){
            if(*maxsize>omax)
                *maxsize=omax;
            if(*minsize<omin)
                *minsize=omin;
        }
    }else{
        int os=split_size(other, dir);
        
        *maxsize-=omin;
        if(*minsize>infsub(s, omax))
            *minsize=infsub(s, omax);
        
        if(other==split->u.s.tl){
            *tlfree+=os-omin;
            *tlshrink=infadd(*tlshrink, infsub(omax, os));
        }else{
            *brfree+=os-omin;
            *brshrink=infadd(*brshrink, infsub(omax, os));
        }
    }
}


static void get_bounds_for(WSplit *node, int dir,
                           int *tlfree, int *brfree, int *maxsize,
                           int *tlshrink, int *brshrink, int *minsize)
{
    if(node==NULL || node->parent==NULL){
        *tlfree=0;
        *brfree=0;
        *tlshrink=0;
        *brshrink=0;
        *maxsize=split_size(node, dir);
        *minsize=*maxsize;
        return;
    }
    
    get_bounds(node->parent, dir, node, tlfree, brfree, maxsize,
               tlshrink, brshrink, minsize);
}


/*}}}*/


/*{{{ Low-level resize code */


/* Resize (sub-)split tree with root <node> . <npos> and <nsize> indicate
 * the new geometry of <node> in direction <dir> (vertical/horizontal). 
 * If <primn> is PRIMN_ANY, the difference between old and new sizes is split
 * proportionally between tl/br nodes modulo minimum maximum size constraints. 
 * Otherwise the node indicated by <primn> is resized first and what is left 
 * after size constraints is applied to the other node. The size bounds 
 * must've been updated split_tree_updated_bounds before using this function.
 */
void split_do_resize(WSplit *node, int dir, int primn, int npos, int nsize)
{
    CHKNODE(node);
    
    if(node->type==SPLIT_REGNODE){
        WRectangle geom=node->geom;
        
        if(dir==SPLIT_VERTICAL){
            geom.y=npos;
            geom.h=nsize;
        }else{
            geom.x=npos;
            geom.w=nsize;
        }
        
        region_fit(node->u.reg, &geom, REGION_FIT_BOUNDS);
    }else{
        WSplit *tl=node->u.s.tl, *br=node->u.s.br;
        int tls, brs, tlpos, brpos;
        
        if(node->type!=dir){
            tls=nsize; brs=nsize;
            tlpos=npos; brpos=npos;
        }else{
            /* Handle used_* better? */
            int tlmin, tlmax, brmin, brmax, sz;
            
            sz=split_size(node, dir);
            tls=split_size(tl, dir);
            brs=split_size(br, dir);
            
            get_minmax(tl, dir, &tlmin, &tlmax);
            get_minmax(br, dir, &brmin, &brmax);
            
            if(primn==PRIMN_TL){
                tls=tls+nsize-sz;
                bound(&tls, tlmin, tlmax);
                brs=nsize-tls;
            }else if(primn==PRIMN_BR){
                brs=brs+nsize-sz;
                bound(&brs, brmin, brmax);
                tls=nsize-brs;
            }else{
                if(sz==0)
                    tls=nsize/2;
                else
                    tls=tls*nsize/sz;
                bound(&tls, tlmin, tlmax);
                brs=nsize-tls;
            }
            
            tlpos=npos;
            brpos=npos+tls;
        }
        
        split_do_resize(tl, dir, primn, tlpos, tls);
        split_do_resize(br, dir, primn, brpos, brs);
    }
    
    if(dir==SPLIT_VERTICAL){
        node->geom.y=npos;
        node->geom.h=nsize;
    }else{
        node->geom.x=npos;
        node->geom.w=nsize;
    }
    split_update_bounds(node, FALSE);
}


void split_resize(WSplit *node, int dir, int primn, int npos, int nsize)
{
    split_update_bounds(node, TRUE);
    split_do_resize(node, dir, PRIMN_ANY, npos, nsize);
}


/* Resize splits starting from <s> and going back to root in <dir> without
 * touching <from>. Because this function may be called multiple times 
 * without anything being done to <from> in between, the expected old size
 * is also passed as a parameter and used insteaed of split_tree_size(from, 
 * dir). If <primn> is PRIMN_ANY, any split on the path will be resized by what 
 * can be done given minimum and maximum size bounds. If <primn> is 
 * PRIMN_TL/PRIMN_BR, only splits where <from> is not the node
 * corresponding <primn> are resized.
 */
static int split_do_resize_rootward(WSplit *split, int dir, WSplit *from, 
                                    int primn, int amount, int fromoldsize)
{
    WSplit *other=(from==split->u.s.tl ? split->u.s.br : split->u.s.tl);
    int os=split_size(other, dir);
    int pos, fpos;
    WSplit *p=split->parent;
    
    if(split->type!=dir){
        if(p==NULL){
            pos=split_pos(split, dir);
        }else{
            pos=split_do_resize_rootward(p, dir, split, primn, amount,
                                         fromoldsize);
            split_do_resize(other, dir, PRIMN_ANY, pos, os+amount);
        }
        fpos=pos;
    }else{
        bool res=(primn==PRIMN_ANY ||
                  (primn==PRIMN_TL && other==split->u.s.tl) ||
                  (primn==PRIMN_BR && other==split->u.s.br));
        int osn=os, fsn=amount+fromoldsize;
        
        if(res){
            int min, max;
            osn-=amount;
            split_update_bounds(other, TRUE);
            get_minmax(other, dir, &min,&max);
            bound(&osn, min, max);
            amount=osn-(os-amount);
        }
        
        if(amount!=0 && p!=NULL){
            pos=split_do_resize_rootward(p, dir, split, primn, amount, 
                                         fromoldsize+os);
        }else{
            if(amount!=0){
                warn("Split tree size calculation bug: resize amount %d!=0 "
                     "and at root node.", amount);
            }
            pos=split_pos(split, dir);
        }
        
        if(other==split->u.s.tl){
            split_do_resize(other, dir, PRIMN_BR, pos, osn);
            fpos=pos+osn;
        }else{
            split_do_resize(other, dir, PRIMN_TL, pos+fsn, osn);
            fpos=pos;
        }
    }
    
    if(dir==SPLIT_VERTICAL){
        split->geom.y=pos;
        split->geom.h=split_size(split, dir)+amount;
    }else{
        split->geom.x=pos;
        split->geom.w=split_size(split, dir)+amount;
    }
    
    return fpos;
}


static void split_resize_rootward(WSplit *node, int dir, int primn, int amount)
{
    WSplit *p=node->parent;
    int s=split_size(node, dir);
    int pos;
    
    if(p==NULL){
        pos=split_pos(node, dir);
        if(primn==PRIMN_TL)
            pos-=amount;
    }else{
        pos=split_do_resize_rootward(p, dir, node, primn, amount, s);
    }
    
    split_do_resize(node, dir, PRIMN_ANY, pos, s+amount);
}



/*}}}*/


/*{{{ Resize interface */


static void adjust_d(int *d, int negmax, int posmax)
{
    if(*d<0 && -*d>negmax)
        *d=-negmax;
    else if(*d>0 && *d>posmax)
        *d=posmax;
}


static void split_tree_do_rqgeom_dir(WSplit *root, WSplit *sub, 
                                     int flags, const WRectangle *geom,
                                     WRectangle *geomret, int dir)
{
    bool horiz=(dir==SPLIT_HORIZONTAL);
    int x1d, x2d;
    int tlfree=0, brfree=0, tlshrink=0, brshrink=0, minsize, maxsize;
    int pos, size;
    
    get_bounds_for(sub, dir, &tlfree, &brfree, &maxsize,
                   &tlshrink, &brshrink, &minsize);
    
    {
        int x, w, w2, ox, ow;
        if(horiz){
            x=geom->x; w=geom->w;
        }else{
            x=geom->y; w=geom->h;
        }
        ox=split_pos(sub, dir);
        ow=split_size(sub, dir);
        
        x1d=x-ox;
        x2d=x+w-(ox+ow);
        
        /* Bound size so e.g. other objects in the same split don't
         * get too small. The change in size difference is proprtionally
         * divided between x1 and x2 (ignoring WEAK settings).
         */
        w2=w;
        bound(&w2, minsize, maxsize);
        if(w2!=w){
            int ax1d=abs(x1d), ax2d=abs(x2d);
            if(ax1d+ax2d!=0){
                x1d+=(w-w2)*ax1d/(ax1d+ax2d);
                x2d+=(w2-w)*ax2d/(ax1d+ax2d);
            }
        }
    }
    
#if 1
    if(flags&(horiz ? REGION_RQGEOM_WEAK_X : REGION_RQGEOM_WEAK_Y)){
        int wd=-x1d+x2d;
        int x2d2;
        /* Adjust width/height change to what is possible */
        adjust_d(&wd, infadd(tlshrink, brshrink), tlfree+brfree);
        
        /* Adjust x initially */
        adjust_d(&x1d, tlfree, tlshrink);
        
        /* Adjust x2 to grow or shrink the frame */
        x2d2=wd+x1d;
        x2d=x2d2;
        adjust_d(&x2d, brshrink, brfree);
        /* Readjust x to if the frame could not be grown/shrink enough
         * keeping it fixed */
        x1d+=x2d-x2d2;
        adjust_d(&x1d, tlfree, tlshrink);
    }else
#endif    
    {
        /* Just adjust both x:s independently */
        adjust_d(&x1d, tlfree, tlshrink);
        adjust_d(&x2d, brshrink, brfree);
        
    }
    
    pos=split_pos(sub, dir);
    size=split_size(sub, dir);
    
    if(geomret!=NULL){
        if(horiz){
            geomret->x=pos+x1d;
            geomret->w=size-x1d+x2d;
        }else{
            geomret->y=pos+x1d;
            geomret->h=size-x1d+x2d;
        }
    }
    
    if(!(flags&REGION_RQGEOM_TRYONLY)){
        WSplit *split=sub->parent;
        
        if(x1d!=0 && split!=NULL)
            split_do_resize_rootward(split, dir, sub, PRIMN_TL, -x1d, size);
        
        if(x2d!=0 && split!=NULL)
            split_do_resize_rootward(split, dir, sub, PRIMN_BR, x2d, size-x1d);
        
        split_do_resize(sub, dir, PRIMN_ANY, pos+x1d, size-x1d+x2d);
    }
}


void split_tree_rqgeom(WSplit *root, WSplit *node, int flags, 
                       const WRectangle *geom,
                       WRectangle *geomret)
{
    split_tree_do_rqgeom_dir(root, node, flags, geom, geomret, SPLIT_HORIZONTAL);
    split_tree_do_rqgeom_dir(root, node, flags, geom, geomret, SPLIT_VERTICAL);
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
        split->max_w=0;
        split->max_h=0;
        split->used_w=0;
        split->used_h=0;
    }
    return split;
}
        

WSplit *create_split(int dir, WSplit *tl, WSplit *br, const WRectangle *geom)
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


WSplit *create_split_regnode(WRegion *reg, const WRectangle *geom)
{
    WSplit *split=do_create_split(geom);
    
    if(split!=NULL){
        split->type=SPLIT_REGNODE;
        split->u.reg=reg;
        set_node_of_reg(reg, split);
    }
    
    return split;
}


WSplit *split_tree_split(WSplit **root, WSplit *node, int dir, int primn,
                         int minsize, int oprimn, 
                         WRegionSimpleCreateFn *fn, WWindow *parent)
{
    int tlfree, brfree, tlshrink, brshrink, minsizebytree, maxsizebytree;
    int objmin, objmax;
    int s, sn, so, pos;
    WSplit *psplit, *nsplit, *nnode;
    WRegion *nreg;
    WFitParams fp;
    
    assert(root!=NULL && *root!=NULL && node!=NULL && parent!=NULL);
    
    if(primn!=PRIMN_TL && primn!=PRIMN_BR)
        primn=PRIMN_BR;
    if(dir!=SPLIT_HORIZONTAL && dir!=SPLIT_VERTICAL)
        dir=SPLIT_VERTICAL;
    
    get_bounds_for(node, dir, &tlfree, &brfree, &maxsizebytree,
                   &tlshrink, &brshrink, &minsizebytree);
    split_update_bounds(node, TRUE);
    get_minmax(node, dir, &objmin, &objmax);
    
    s=split_size(node, dir);
    sn=s/2;
    so=s-sn;
    
    if(sn<minsize)
        sn=minsize;
    if(so<objmin)
        so=objmin;
    
    if(sn+so!=s){
        if(tlfree+brfree<(sn+so)-s){
            warn("Unable to split: not enough free space.");
            return NULL;
        }
        /* Get more space */
        split_resize_rootward(node, dir, PRIMN_ANY, (sn+so)-s);
    }
    
    /* Create split and new window
     */
    fp.mode=REGION_FIT_EXACT;
    fp.g=node->geom;
    
    nsplit=create_split(dir, NULL, NULL, &(fp.g));
    
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

    nnode=create_split_regnode(nreg, &(fp.g));
    if(nnode==NULL){
        warn_err();
        destroy_obj((Obj*)nreg);
        free(nsplit);
        return NULL;
    }
    
    /* Now that everything's ok, resize and move everything.
     */
    
    pos=split_pos(node, dir);
    if(primn!=PRIMN_BR)
        pos+=sn;
    split_do_resize(node, dir, oprimn, pos, so);
    
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
    
    if(reclaim_space){
        nsize=split_size(split, split->type);
        npos=split_pos(split, split->type);
        split_resize(other, split->type, PRIMN_ANY, npos, nsize);
    }
    
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
    WSplit *split;
    
    if(node==NULL)
        return NULL;
    
    while(1){
        CHKNODE(node);
        
        if(node->type==SPLIT_REGNODE)
            return node;
        
        if(node->type==dir)
            node=node->u.s.tl;
        else
            node=(node->u.s.current==0 ? node->u.s.tl : node->u.s.br);
    }
    
    assert(0);
}


WSplit *split_current_br(WSplit *node, int dir)
{
    if(node==NULL)
        return NULL;
    
    while(1){
        CHKNODE(node);
        
        if(node->type==SPLIT_REGNODE)
            return node;
        
        if(node->type==dir)
            node=node->u.s.br;
        else
            node=(node->u.s.current==0 ? node->u.s.tl : node->u.s.br);
    }
    
    assert(0);
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

    
static Obj *hoist(WSplit *split)
{
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
    if(split->type==SPLIT_REGNODE)
        return NULL;
    return hoist(split->u.s.tl);
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to bottom or right
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT_MEMBER
Obj *split_br(WSplit *split)
{
    if(split->type==SPLIT_REGNODE)
        return NULL;
    return hoist(split->u.s.br);
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

