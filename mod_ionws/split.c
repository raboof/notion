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


IMPLCLASS(WWsSplit, Obj, NULL, NULL);


static Rb_node split_of_map=NULL;


/*{{{ Geometry helper functions */


static int reg_size(WRegion *reg, int dir)
{
    if(dir==HORIZONTAL)
        return REGION_GEOM(reg).w;
    return REGION_GEOM(reg).h;
}


static int reg_other_size(WRegion *reg, int dir)
{
    if(dir==HORIZONTAL)
        return REGION_GEOM(reg).h;
    return REGION_GEOM(reg).w;
}


static int reg_pos(WRegion *reg, int dir)
{
    if(dir==HORIZONTAL)
        return REGION_GEOM(reg).x;
    return REGION_GEOM(reg).y;
}


static WRectangle split_tree_geom(Obj *obj)
{
    if(OBJ_IS(obj, WRegion))
        return REGION_GEOM(obj);
    
    return ((WWsSplit*)obj)->geom;
}


int split_tree_size(Obj *obj, int dir)
{
    if(OBJ_IS(obj, WRegion))
        return reg_size((WRegion*)obj, dir);
    
    if(dir==HORIZONTAL)
        return ((WWsSplit*)obj)->geom.w;
    return ((WWsSplit*)obj)->geom.h;
}


int split_tree_pos(Obj *obj, int dir)
{
    if(OBJ_IS(obj, WRegion))
        return reg_pos((WRegion*)obj, dir);
    
    if(dir==HORIZONTAL)
        return ((WWsSplit*)obj)->geom.x;
    return ((WWsSplit*)obj)->geom.y;
}


int split_tree_other_size(Obj *obj, int dir)
{
    if(OBJ_IS(obj, WRegion))
        return reg_other_size((WRegion*)obj, dir);
    
    if(dir==HORIZONTAL)
        return ((WWsSplit*)obj)->geom.h;
    return ((WWsSplit*)obj)->geom.w;
}


static int reg_calcresize(WRegion *reg, int dir, int nsize)
{
    int tmp;
    
    if(dir==HORIZONTAL)
        tmp=region_min_w(reg);
    else
        tmp=region_min_h(reg);

    return (nsize<tmp ? tmp : nsize);
}


static int reg_resize(WRegion *reg, int dir, int npos, int nsize)
{
    WRectangle geom=REGION_GEOM(reg);
    
    if(dir==VERTICAL){
        geom.y=npos;
        geom.h=nsize;
        /*wwin->flags&=~WWINDOW_HFORCED;*/
    }else{
        geom.x=npos;
        geom.w=nsize;
        /*wwin->flags&=~WWINDOW_WFORCED;*/
    }
    
    region_fit(reg, &geom, REGION_FIT_EXACT);
    
    return nsize;
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


/*}}}*/


/*{{{ Functions to find the parent split */


static WWsSplit *split_of_reg(WRegion *reg)
{
    Rb_node node=NULL;
    int found=0;
    
    /*assert(REGION_MANAGER_CHK(reg, WIonWS)!=NULL);*/
    
    if(split_of_map!=NULL){
        node=rb_find_pkey_n(split_of_map, reg, &found);
        if(found)
            return (WWsSplit*)(node->v.val);
    }
    
    return NULL;
}


#define split_of split_tree_split_of


WWsSplit *split_tree_split_of(Obj *obj)
{
    if(OBJ_IS(obj, WWsSplit)){
        return ((WWsSplit*)obj)->parent;
    }else{
        assert(OBJ_IS(obj, WRegion));
        return split_of_reg((WRegion*)obj);
    }
}


bool set_split_of_reg(WRegion *reg, WWsSplit *split)
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


void set_split_of(Obj *obj, WWsSplit *split)
{
    if(OBJ_IS(obj, WWsSplit)){
        ((WWsSplit*)obj)->parent=split;
    }else{
        assert(OBJ_IS(obj, WRegion));
        set_split_of_reg((WRegion*)obj, split);
    }
}


/*}}}*/


/*{{{ Low-level resize code */


static void get_region_bounds(WRegion *reg, int dir, int *min, int *max)
{
    XSizeHints hints;
    uint relw, relh;
    region_resize_hints(reg, &hints, &relw, &relh);
    
    if(dir==HORIZONTAL){
        *min=(hints.flags&PMinSize ? hints.min_width : 1)
            +REGION_GEOM(reg).w-relw;
        /*if(hints.flags&PMaxSize)
            *max=hints.max_width + REGION_GEOM(reg).w-relw;
        else*/
            *max=INT_MAX;
    }else{
        *min=(hints.flags&PMinSize ? hints.min_height : 1)
            +REGION_GEOM(reg).h-relh;
        /*if(hints.flags&PMaxSize)
            *max=hints.max_height + REGION_GEOM(reg).h-relh;
        else*/
            *max=INT_MAX;
    }
}



/* Update size bounds for the split with root <node>. */
static void split_tree_update_bounds(Obj *node, int dir, int *min, int *max)
{
    int tlmax, tlmin, brmax, brmin;
    WWsSplit *split;
    
    if(!OBJ_IS(node, WWsSplit)){
        assert(OBJ_IS(node, WRegion));
        get_region_bounds((WRegion*)node, dir, min, max);
        return;
    }
    
    split=(WWsSplit*)node;
    
    split_tree_update_bounds(split->tl, dir, &tlmin, &tlmax);
    split_tree_update_bounds(split->br, dir, &brmin, &brmax);
    
    if(split->dir!=dir){
        *min=maxof(tlmin, brmin);
        *max=minof(tlmax, brmax);
    }else{
        *min=infadd(tlmin, brmin);
        *max=infadd(tlmax, brmax);
    }
    
    if(dir==VERTICAL){
        split->max_h=*max;
        split->min_h=*min;
    }else{
        split->max_w=*max;
        split->min_w=*min;
    }
}


/* Get the bounds calculated by the above function. */
static void split_tree_get_bounds(Obj *node, int dir, int *min, int *max)
{
    WWsSplit *split;
    
    if(!OBJ_IS(node, WWsSplit)){
        assert(OBJ_IS(node, WRegion));
        get_region_bounds((WRegion*)node, dir, min, max);
        return;
    }

    split=(WWsSplit*)node;

    if(dir==VERTICAL){
        *max=split->max_h;
        *min=split->min_h;
    }else{
        *max=split->max_w;
        *min=split->min_w;
    }
}


/* Get resize bounds for <from> due to <split> and all nodes towards the
 * root. <from> must be a child node of <split> (->tl/br). The <*free>
 * variables indicate the free space in that direction while the <*shrink>
 * variables indicate the amount the object in that direction can grow
 * (INT_MAX means no limit has been set). <minsize> and <maxsize> are
 * size limits set by siblings in splits perpendicular to <dir>.
 */
static void get_bounds(WWsSplit *split, int dir, Obj *from,
                       int *tlfree, int *brfree, int *maxsize,
                       int *tlshrink, int *brshrink, int *minsize)
{
    Obj *other=(from==split->tl ? split->br : split->tl);
    WWsSplit *p=split->parent;
    int s=split_tree_size((Obj*)split, dir);
    int omin, omax;
    
    if(p==NULL){
        *tlfree=0;
        *brfree=0;
        *maxsize=s;
        *tlshrink=0;
        *brshrink=0;
        *minsize=s;
    }else{
        get_bounds(p, dir, (Obj*)split, 
                   tlfree, brfree, maxsize,
                   tlshrink, brshrink, minsize);
    }
    
    split_tree_update_bounds(other, dir, &omin, &omax);
    
    if(split->dir!=dir){
        if(p!=NULL){
            if(*maxsize>omax)
                *maxsize=omax;
            if(*minsize<omin)
                *minsize=omin;
        }
    }else{
        int os=split_tree_size((Obj*)other, dir);
        
        *maxsize-=omin;
        if(*minsize>infsub(s, omax))
            *minsize=infsub(s, omax);
        
        if(other==split->tl){
            *tlfree+=os-omin;
            *tlshrink=infadd(*tlshrink, infsub(omax, os));
        }else{
            *brfree+=os-omin;
            *brshrink=infadd(*brshrink, infsub(omax, os));
        }
    }
}


static void get_bounds_for(Obj *obj, int dir,
                           int *tlfree, int *brfree, int *maxsize,
                           int *tlshrink, int *brshrink, int *minsize)
{
    WWsSplit *split=split_of(obj);
    
    if(split==NULL){
        *tlfree=0;
        *brfree=0;
        *tlshrink=0;
        *brshrink=0;
        *maxsize=split_tree_size(obj, dir);
        *minsize=*maxsize;
        return;
    }
    
    get_bounds(split, dir, obj, tlfree, brfree, maxsize,
               tlshrink, brshrink, minsize);
}


/* Resize (sub-)split tree with root <node> . <npos> and <nsize> indicate
 * the new geometry of <node> in direction <dir> (vertical/horizontal). 
 * If <primn> is ANY, the difference between old and new sizes is split
 * proportionally between tl/br nodes modulo minimum maximum size constraints. 
 * Otherwise the node indicated by <primn> is resized first and what is left 
 * after size constraints is applied to the other node. The size bounds 
 * must've been updated split_tree_updated_bounds before using this function.
 */
void split_tree_do_resize(Obj *node, int dir, int primn, int npos, int nsize)
{
    WWsSplit *split;
    
    if(!OBJ_IS(node, WWsSplit)){
        assert(OBJ_IS(node, WRegion));
        reg_resize((WRegion*)node, dir, npos, nsize);
        return;
    }
    
    split=(WWsSplit*)node;
    
    if(split->dir!=dir){
        split_tree_do_resize(split->tl, dir, primn, npos, nsize);
        split_tree_do_resize(split->br, dir, primn, npos, nsize);
    }else{
        int tlmin, tlmax, brmin, brmax, tls, brs, sz;
        
        sz=split_tree_size(node, dir);
        tls=split_tree_size(split->tl, dir);
        brs=split_tree_size(split->br, dir);

        split_tree_get_bounds(split->tl, dir, &tlmin, &tlmax);
        split_tree_get_bounds(split->br, dir, &brmin, &brmax);
        
        if(primn==TOP_OR_LEFT){
            tls=tls+nsize-sz;
            bound(&tls, tlmin, tlmax);
            brs=nsize-tls;
        }else if(primn==BOTTOM_OR_RIGHT){
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
        
        split_tree_do_resize(split->tl, dir, primn, npos, tls);
        split_tree_do_resize(split->br, dir, primn, npos+tls, brs);
    }
    
    if(dir==VERTICAL){
        split->geom.y=npos;
        split->geom.h=nsize;
    }else{
        split->geom.x=npos;
        split->geom.w=nsize;
    }
}


void split_tree_resize(Obj *node, int dir, int primn,
                       int npos, int nsize)
{
    int dummymax, dummymin;
    split_tree_update_bounds(node, dir, &dummymax, &dummymin);
    split_tree_do_resize(node, dir, ANY, npos, nsize);
}


/* Resize splits starting from <s> and going back to root in <dir> without
 * touching <from>. Because this function may be called multiple times 
 * without anything being done to <from> in between, the expected old size
 * is also passed as a parameter and used insteaed of split_tree_size(from, 
 * dir). If <primn> is ANY, any split on the path will be resized by what 
 * can be done given minimum and maximum size bounds. If <primn> is 
 * TOP_OR_LEFT/BOTTOM_OR_RIGHT, only splits where <from> is not the node
 * corresponding <primn> are resized.
 */
static int do_resize_split(WWsSplit *split, int dir, Obj *from, int primn, 
                           int amount, int fromoldsize)
{
    Obj *other=(from==split->tl ? split->br : split->tl);
    int os=split_tree_size(other, dir);
    int pos, fpos;
    WWsSplit *p=split->parent;
    
    if(split->dir!=dir){
        if(p==NULL){
            pos=split_tree_pos((Obj*)split, dir);
        }else{
            pos=do_resize_split(p, dir, (Obj*)split, primn, amount, 
                                fromoldsize);
            split_tree_do_resize(other, dir, ANY, pos, os+amount);
        }
        fpos=pos;
    }else{
        
        bool res=(primn==ANY ||
                  (primn==TOP_OR_LEFT && other==split->tl) ||
                  (primn==BOTTOM_OR_RIGHT && other==split->br));
        int osn=os, fsn=amount+fromoldsize;/*split_tree_size(from, dir);*/
        
        if(res){
            int min, max;
            osn-=amount;
            split_tree_get_bounds(other, dir, &min, &max);
            bound(&osn, min, max);
            amount=osn-(os-amount);
        }
        
        if(amount!=0 && p!=NULL){
            pos=do_resize_split(p, dir, (Obj*)split, primn, amount,
                                fromoldsize+os);
        }else{
            if(amount!=0){
                warn("Split tree size calculation bug: resize amount %d!=0 "
                     "and at root node.", amount);
            }
            pos=split_tree_pos((Obj*)split, dir);
        }
        
        if(other==split->tl){
            split_tree_do_resize(other, dir, BOTTOM_OR_RIGHT, pos, osn);
            fpos=pos+osn;
        }else{
            split_tree_do_resize(other, dir, TOP_OR_LEFT, pos+fsn, osn);
            fpos=pos;
        }
    }

    if(dir==VERTICAL){
        split->geom.y=pos;
        split->geom.h=split_tree_size((Obj*)split, dir)+amount;
    }else{
        split->geom.x=pos;
        split->geom.w=split_tree_size((Obj*)split, dir)+amount;
    }
    
    return fpos;
}


static void do_resize_node(Obj *obj, int dir, int primn, int amount)
{
    WWsSplit *p=split_of(obj);
    int s=split_tree_size(obj, dir);
    int pos;
    
    if(p==NULL){
        pos=split_tree_pos(obj, dir);
        if(primn==TOP_OR_LEFT)
            pos-=amount;
    }else{
        pos=do_resize_split(p, dir, obj, primn, amount, s);
    }
    
    split_tree_do_resize(obj, dir, ANY, pos, s+amount);
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


static void split_tree_do_rqgeom_dir(Obj *root, Obj *sub, 
                                     int flags, const WRectangle *geom,
                                     WRectangle *geomret, int dir)
{
    bool horiz=(dir==HORIZONTAL);
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
        ox=split_tree_pos(sub, dir);
        ow=split_tree_size(sub, dir);
        
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

    pos=split_tree_pos(sub, dir);
    size=split_tree_size(sub, dir);

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
        WWsSplit *split=split_of(sub);

        if(x1d!=0 && split!=NULL)
            do_resize_split(split, dir, sub, TOP_OR_LEFT, -x1d, size);
        
        if(x2d!=0 && split!=NULL)
            do_resize_split(split, dir, sub, BOTTOM_OR_RIGHT, x2d, size-x1d);
        
        split_tree_do_resize(sub, dir, ANY, pos+x1d, size-x1d+x2d);
    }
}


void split_tree_rqgeom(Obj *root, Obj *obj, int flags, 
                       const WRectangle *geom,
                       WRectangle *geomret)
{
    split_tree_do_rqgeom_dir(root, obj, flags, geom, geomret, HORIZONTAL);
    split_tree_do_rqgeom_dir(root, obj, flags, geom, geomret, VERTICAL);
}


/*}}}*/


/*{{{ Split */


WWsSplit *create_split(int dir, Obj *tl, Obj *br, const WRectangle *geom)
{
    WWsSplit *split=ALLOC(WWsSplit);
    
    if(split==NULL){
        warn_err();
        return NULL;
    }
    
    OBJ_INIT(split, WWsSplit);
    
    split->dir=dir;
    split->tl=tl;
    split->br=br;
    split->geom=*geom;
    split->parent=NULL;
    split->current=0;
    
    return split;
}


WRegion *split_tree_split(Obj **root, Obj *obj, int dir, int primn, 
                          int minsize, int oprimn, 
                          WRegionSimpleCreateFn *fn, WWindow *parent)
{
    int tlfree, brfree, tlshrink, brshrink, minsizebytree, maxsizebytree;
    int objmin, objmax;
    int s, sn, so, pos;
    WWsSplit *split, *nsplit;
    WRegion *nreg;
    WFitParams fp;
    
    assert(root!=NULL && *root!=NULL && obj!=NULL && parent!=NULL);
    
    if(primn!=TOP_OR_LEFT && primn!=BOTTOM_OR_RIGHT)
        primn=BOTTOM_OR_RIGHT;
    if(dir!=HORIZONTAL && dir!=VERTICAL)
        dir=VERTICAL;
    
    get_bounds_for(obj, dir, &tlfree, &brfree, &maxsizebytree,
                   &tlshrink, &brshrink, &minsizebytree);
    split_tree_update_bounds(obj, dir, &objmin, &objmax);
    
    s=split_tree_size(obj, dir);
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
        do_resize_node(obj, dir, ANY, (sn+so)-s);
    }

    /* Create split and new window
     */
    fp.mode=REGION_FIT_EXACT;
    fp.g=split_tree_geom(obj);
    
    nsplit=create_split(dir, NULL, NULL, &(fp.g));
    
    if(nsplit==NULL)
        return NULL;
    
    if(dir==VERTICAL){
        if(primn==BOTTOM_OR_RIGHT)
            fp.g.y+=so;
        fp.g.h=sn;
    }else{
        if(primn==BOTTOM_OR_RIGHT)
            fp.g.x+=so;
        fp.g.w=sn;
    }
    
    nreg=fn(parent, &fp);
    
    if(nreg==NULL){
        free(nsplit);
        return NULL;
    }
    
    /* Now that everything's ok, resize and move everything.
     */

    pos=split_tree_pos(obj, dir);
    if(primn!=BOTTOM_OR_RIGHT)
        pos+=sn;
    split_tree_do_resize(obj, dir, oprimn, pos, so);
    
    /* Set up split structure
     */
    split=split_of(obj);
    
    set_split_of(obj, nsplit);
    set_split_of_reg(nreg, nsplit);
    
    if(primn==BOTTOM_OR_RIGHT){
        nsplit->tl=obj;
        nsplit->br=(Obj*)nreg;
    }else{
        nsplit->tl=(Obj*)nreg;
        nsplit->br=obj;
    }
    
    if(split!=NULL){
        if(obj==split->tl)
            split->tl=(Obj*)nsplit;
        else
            split->br=(Obj*)nsplit;
        nsplit->parent=split;
    }else{
        *root=(Obj*)nsplit;
    }
    
    return nreg;
}


/*}}}*/


/*{{{ Remove */


static bool split_tree_remove_split(Obj **root, WWsSplit *split, 
                                    bool reclaim_space)
{
    WWsSplit *split2;
    Obj *other;
    int osize, nsize, npos;
    int primn;
    
    if(split->tl==NULL){
        other=split->br;
        primn=TOP_OR_LEFT;
    }else{
        other=split->tl;
        primn=BOTTOM_OR_RIGHT;
    }
    
    split2=split->parent;
    
    if(split2!=NULL){
        if((Obj*)split==split2->tl)
            split2->tl=other;
        else
            split2->br=other;
    }else{
        *root=other;
    }
    
    if(other==NULL)
        return FALSE;
    
    set_split_of(other, split2);
    
    if(reclaim_space){
        nsize=split_tree_size((Obj*)split, split->dir);
        npos=split_tree_pos((Obj*)split, split->dir);
        split_tree_resize(other, split->dir, ANY, npos, nsize);
    }

    destroy_obj((Obj*)split);
    
    return TRUE;
}

    
WRegion *split_tree_remove(Obj **root, WRegion *reg, bool reclaim_space)
{
    WWsSplit *split;
    
    split=split_of_reg(reg);

    if(split!=NULL){
        WRegion *other;
        if(split->tl==(Obj*)reg){
            split->tl=NULL;
            other=split_tree_current_tl(split->br, split->dir);
        }else{
            split->br=NULL;
            other=split_tree_current_br(split->tl, split->dir);
        }
        
        set_split_of_reg(reg, NULL);
        
        split_tree_remove_split(root, split, reclaim_space);
        
        return other;
    }else{
        *root=NULL;
        return NULL;
    }
}


/*}}}*/


/*{{{ iowns_manage_rescue */


static WMPlex *find_mplex_descend(Obj *ptr, int primn)
{
    WMPlex *reg;
    WWsSplit *split;
    
    do{
        if(OBJ_IS(ptr, WMPlex))
            return (WMPlex*)ptr;
        
        if(!OBJ_IS(ptr, WWsSplit))
            return NULL;
        
        split=(WWsSplit*)ptr;
        
        if(primn==TOP_OR_LEFT)
            reg=find_mplex_descend(split->tl, primn);
        else
            reg=find_mplex_descend(split->br, primn);
        
        if(reg!=NULL)
            return reg;
        
        if(primn==TOP_OR_LEFT)
            ptr=split->br;
        else
            ptr=split->tl;
    }while(1);
}


WMPlex *split_tree_find_mplex(WRegion *from)
{
    WMPlex *nmgr=NULL;
    Obj *obj=(Obj*)from;
    WWsSplit *split=split_of_reg(from);
    
    while(split!=NULL){
        if(split->tl==obj)
            nmgr=find_mplex_descend(split->br, TOP_OR_LEFT);
        else
            nmgr=find_mplex_descend(split->tl, BOTTOM_OR_RIGHT);
        if(nmgr!=NULL)
            break;
        obj=(Obj*)split;
        split=split->parent;
    }
    
    return nmgr;
}


/*}}}*/


/*{{{ Tree traversal */


WRegion *split_tree_current_tl(Obj *obj, int dir)
{
    WWsSplit *split;
    
    if(obj==NULL)
        return NULL;
    
    while(1){
        if(OBJ_IS(obj, WRegion))
            return (WRegion*)obj;
        
        assert(OBJ_IS(obj, WWsSplit));
        
        split=(WWsSplit*)obj;
        
        if(split->dir==dir){
            obj=split->tl;
            continue;
        }
        
        obj=(split->current==0 ? split->tl : split->br);
    }
    
    assert(0);
}


WRegion *split_tree_current_br(Obj *obj, int dir)
{
    WWsSplit *split;
    
    if(obj==NULL)
        return NULL;
    
    while(1){
        if(OBJ_IS(obj, WRegion))
            return (WRegion*)obj;
        
        assert(OBJ_IS(obj, WWsSplit));
        
        split=(WWsSplit*)obj;
        
        if(split->dir==dir){
            obj=split->br;
            continue;
        }
        
        obj=(split->current==0 ? split->tl : split->br);
    }
    
    assert(0);
}


static WWsSplit *find_split(Obj *obj, int dir, int *from)
{
    WWsSplit *split;
    
    split=split_of(obj);
    
    while(split!=NULL){
        if(split->dir==dir){
            if(obj==split->tl)
                *from=TOP_OR_LEFT;
            else
                *from=BOTTOM_OR_RIGHT;
            break;
        }
        
        obj=(Obj*)split;
        split=split->parent;
    }
    
    return split;
}


WRegion *split_tree_to_br(WRegion *reg, int dir)
{
    Obj *prev=(Obj*)reg;
    WWsSplit *split;
    int from;
    
    if(reg==NULL)
        return NULL;
    
    while(1){
        split=find_split(prev, dir, &from);
        
        if(split==NULL)
            break;
        
        if(from==TOP_OR_LEFT)
            return split_tree_current_tl(split->br, dir);
        
        prev=(Obj*)split;
    }
    
    
    return NULL;
}


WRegion *split_tree_to_tl(WRegion *reg, int dir)
{
    Obj *prev=(Obj*)reg;
    WWsSplit *split;
    int from;
    
    if(reg==NULL)
        return NULL;
    
    while(1){
        split=find_split(prev, dir, &from);
        
        if(split==NULL)
            break;
        
        if(from==BOTTOM_OR_RIGHT)
            return split_tree_current_br(split->tl, dir);
        
        prev=(Obj*)split;
    }
    
    return NULL;
}


/*}}}*/


/*{{{ Misc. exports */


/*EXTL_DOC
 * Return parent split for \var{split}.
 */
EXTL_EXPORT_MEMBER
WWsSplit *split_parent(WWsSplit *split)
{
    return split->parent;
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to top or left
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT_MEMBER
Obj *split_tl(WWsSplit *split)
{
    return split->tl;
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to bottom or right
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT_MEMBER
Obj *split_br(WWsSplit *split)
{
    return split->br;
}


/*EXTL_DOC
 * Is \var{split} a vertical split?
 */
EXTL_EXPORT_MEMBER
bool split_is_vertical(WWsSplit *split)
{
    return (split->dir==VERTICAL);
}


/*EXTL_DOC
 * Is \var{split} a horizontal split?
 */
EXTL_EXPORT_MEMBER
bool split_is_horizontal(WWsSplit *split)
{
    return (split->dir==VERTICAL);
}


/*EXTL_DOC
 * Returns the area of workspace used by the regions under \var{split}.
 */
EXTL_EXPORT_MEMBER
ExtlTab split_geom(WWsSplit *split)
{
    return extl_table_from_rectangle(&(split->geom));
}


/*}}}*/


/*{{{ Misc. */


WRegion *split_tree_region_at(Obj *obj, int x, int y)
{
    WWsSplit *split;
    WRegion *ret;
    
    if(!OBJ_IS(obj, WWsSplit)){
        if(!OBJ_IS(obj, WRegion))
            return NULL;
        if(!rectangle_contains(&REGION_GEOM((WRegion*)obj), x, y))
            return NULL;
        return (WRegion*)obj;
    }
    
    split=(WWsSplit*)obj;
    
    if(!rectangle_contains(&(split->geom), x, y))
        return NULL;
    
    ret=split_tree_region_at(split->tl, x, y);
    if(ret==NULL)
        ret=split_tree_region_at(split->br, x, y);
    return ret;
}


void split_tree_mark_current(WRegion *reg)
{
    WWsSplit *split=split_of_reg(reg);
    Obj *prev=(Obj*)reg;
    
    while(split!=NULL){
        split->current=(split->tl==prev ? 0 : 1);
        prev=(Obj*)split;
        split=split->parent;
    }
}


/*}}}*/

