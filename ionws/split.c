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

#include <ioncore/common.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/objp.h>
#include <ioncore/resize.h>
#include <ioncore/attach.h>
#include <ioncore/defer.h>
#include <ioncore/reginfo.h>
#include <ioncore/extlconv.h>
#include <ioncore/manage.h>
#include <ioncore/minmax.h>
#include "ionws.h"
#include "ionframe.h"
#include "split.h"
#include "splitframe.h"
#include "bindmaps.h"

	
IMPLOBJ(WWsSplit, WObj, NULL, NULL);


/*{{{ Misc helper functions */


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


static WRectangle split_tree_geom(WObj *obj)
{
	if(WOBJ_IS(obj, WRegion))
		return REGION_GEOM(obj);
	
	return ((WWsSplit*)obj)->geom;
}


int split_tree_size(WObj *obj, int dir)
{
	if(WOBJ_IS(obj, WRegion))
		return reg_size((WRegion*)obj, dir);
	
	if(dir==HORIZONTAL)
		return ((WWsSplit*)obj)->geom.w;
	return ((WWsSplit*)obj)->geom.h;
}


int split_tree_pos(WObj *obj, int dir)
{
	if(WOBJ_IS(obj, WRegion))
		return reg_pos((WRegion*)obj, dir);
	
	if(dir==HORIZONTAL)
		return ((WWsSplit*)obj)->geom.x;
	return ((WWsSplit*)obj)->geom.y;
}


int split_tree_other_size(WObj *obj, int dir)
{
	if(WOBJ_IS(obj, WRegion))
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
	
	region_fit(reg, &geom);
	
	return nsize;
}


static WWsSplit *split_of_reg(WRegion *reg)
{
	WWsSplit *split=NULL;
	WIonWS *ws=REGION_MANAGER_CHK(reg, WIonWS);
	assert(ws!=NULL);
	extl_table_get(ws->managed_splits, 'o', 'o', reg, &split);
	return split;
}

	
WWsSplit *split_of(WObj *obj)
{
	if(WOBJ_IS(obj, WWsSplit)){
		return ((WWsSplit*)obj)->parent;
	}else{
		assert(WOBJ_IS(obj, WRegion));
		return split_of_reg((WRegion*)obj);
	}
}


void set_split_of_reg(WRegion *reg, WWsSplit *split)
{
	WIonWS *ws=REGION_MANAGER_CHK(reg, WIonWS);
	assert(ws!=NULL);
	extl_table_set(ws->managed_splits, 'o', 'o', reg, split);
}


void set_split_of(WObj *obj, WWsSplit *split)
{
	if(WOBJ_IS(obj, WWsSplit)){
		((WWsSplit*)obj)->parent=split;
	}else{
		assert(WOBJ_IS(obj, WRegion));
		set_split_of_reg((WRegion*)obj, split);
	}
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
static void split_tree_update_bounds(WObj *node, int dir, int *min, int *max)
{
	int tlmax, tlmin, brmax, brmin;
	WWsSplit *split;
	
	if(!WOBJ_IS(node, WWsSplit)){
		assert(WOBJ_IS(node, WRegion));
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
static void split_tree_get_bounds(WObj *node, int dir, int *min, int *max)
{
	WWsSplit *split;
	
	if(!WOBJ_IS(node, WWsSplit)){
		assert(WOBJ_IS(node, WRegion));
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
static void get_bounds(WWsSplit *split, int dir, WObj *from,
					   int *tlfree, int *brfree, int *maxsize,
					   int *tlshrink, int *brshrink, int *minsize)
{
	WObj *other=(from==split->tl ? split->br : split->tl);
	WWsSplit *p=split->parent;
	int s=split_tree_size((WObj*)split, dir);
	int omin, omax;
	
	if(p==NULL){
		*tlfree=0;
		*brfree=0;
		*maxsize=s;
		*tlshrink=0;
		*brshrink=0;
		*minsize=s;
	}else{
		get_bounds(p, dir, (WObj*)split, 
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
		int os=split_tree_size((WObj*)other, dir);
		
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


static void get_bounds_for(WObj *obj, int dir,
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
void split_tree_do_resize(WObj *node, int dir, int primn, int npos, int nsize)
{
	WWsSplit *split;
	
	if(!WOBJ_IS(node, WWsSplit)){
		assert(WOBJ_IS(node, WRegion));
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


void split_tree_resize(WObj *node, int dir, int primn,
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
static int do_resize_split(WWsSplit *split, int dir, WObj *from, int primn, 
						   int amount, int fromoldsize)
{
	WObj *other=(from==split->tl ? split->br : split->tl);
	int os=split_tree_size(other, dir);
	int pos, fpos;
	WWsSplit *p=split->parent;
	
	if(split->dir!=dir){
		if(p==NULL){
			pos=split_tree_pos((WObj*)split, dir);
		}else{
			pos=do_resize_split(p, dir, (WObj*)split, primn, amount, 
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
			pos=do_resize_split(p, dir, (WObj*)split, primn, amount,
								fromoldsize+os);
		}else{
			if(amount!=0){
				warn("Split tree size calculation bug: resize amount %d!=0 "
					 "and at root node.", amount);
			}
			pos=split_tree_pos((WObj*)split, dir);
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
		split->geom.h=split_tree_size((WObj*)split, dir)+amount;
	}else{
		split->geom.x=pos;
		split->geom.w=split_tree_size((WObj*)split, dir)+amount;
	}
	
	return fpos;
}


static void do_resize_node(WObj *obj, int dir, int primn, int amount)
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


static void ionws_do_request_geom_dir(WIonWS *ws, WObj *sub, 
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


static void ionws_do_request_geom(WIonWS *ws, WObj *obj,
								  int flags, const WRectangle *geom,
								  WRectangle *geomret)
{
	ionws_do_request_geom_dir(ws, obj, flags, geom, geomret, HORIZONTAL);
	ionws_do_request_geom_dir(ws, obj, flags, geom, geomret, VERTICAL);
}


void ionws_request_managed_geom(WIonWS *ws, WRegion *mgd, 
								int flags, const WRectangle *geom,
								WRectangle *geomret)
{
	ionws_do_request_geom(ws, (WObj*)mgd, flags, geom, geomret);
}


/*}}}*/


/*{{{ Split */


WWsSplit *create_split(int dir, WObj *tl, WObj *br, const WRectangle *geom)
{
	WWsSplit *split=ALLOC(WWsSplit);
	
	if(split==NULL){
		warn_err();
		return NULL;
	}
	
	WOBJ_INIT(split, WWsSplit);
	
	split->dir=dir;
	split->tl=tl;
	split->br=br;
	split->geom=*geom;
	split->parent=NULL;
	split->current=0;
	
	return split;
}


static WRegion *do_split_at(WIonWS *ws, WObj *obj, int dir, int primn,
                            int minsize, int oprimn,
                            WRegionSimpleCreateFn *fn)
{
	int tlfree, brfree, tlshrink, brshrink, minsizebytree, maxsizebytree;
	int objmin, objmax;
	int s, sn, so, pos;
	WWsSplit *split, *nsplit;
	WRectangle geom;
	WRegion *nreg;
	WWindow *par;
	
	assert(obj!=NULL);
	
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
	geom=split_tree_geom(obj);
	
	nsplit=create_split(dir, NULL, NULL, &geom);
	
	if(nsplit==NULL)
		return NULL;
	
	if(dir==VERTICAL){
		if(primn==BOTTOM_OR_RIGHT)
			geom.y+=so;
		geom.h=sn;
	}else{
		if(primn==BOTTOM_OR_RIGHT)
			geom.x+=so;
		geom.w=sn;
	}
	
	par=REGION_PARENT_CHK(ws, WWindow);
	assert(par!=NULL);
	
	nreg=fn(par, &geom);
	
	if(nreg==NULL){
		free(nsplit);
		return NULL;
	}
	
	ionws_add_managed(ws, nreg);
	
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
		nsplit->br=(WObj*)nreg;
	}else{
		nsplit->tl=(WObj*)nreg;
		nsplit->br=obj;
	}
	
	if(split!=NULL){
		if(obj==split->tl)
			split->tl=(WObj*)nsplit;
		else
			split->br=(WObj*)nsplit;
		nsplit->parent=split;
	}else{
		ws->split_tree=(WObj*)nsplit;
	}
	
	return nreg;
}


WRegion *split_reg(WRegion *reg, int dir, int primn, int minsize,
				   WRegionSimpleCreateFn *fn)
{
	WRegion *mgr=REGION_MANAGER(reg);
	assert(mgr!=NULL && WOBJ_IS(mgr, WIonWS));
	
	return do_split_at((WIonWS*)mgr, (WObj*)reg, dir, primn, minsize, primn,
                       fn);
}


WRegion *split_toplevel(WIonWS *ws, int dir, int primn, int minsize,
						WRegionSimpleCreateFn *fn)
{
	if(ws->split_tree==NULL)
		return NULL;
	
	return do_split_at(ws, ws->split_tree, dir, primn, minsize, ANY, fn);
}


/*}}}*/


/*{{{ Navigation */


static WRegion *left_or_topmost_current(WObj *obj, int dir)
{
	WWsSplit *split;
	
	if(obj==NULL)
		return NULL;
	
	while(1){
		if(WOBJ_IS(obj, WRegion))
			return (WRegion*)obj;
		
		assert(WOBJ_IS(obj, WWsSplit));
		
		split=(WWsSplit*)obj;
		
		if(split->dir==dir){
			obj=split->tl;
			continue;
		}
		
		obj=(split->current==0 ? split->tl : split->br);
	}
	
	assert(0);
}


static WRegion *right_or_lowest_current(WObj *obj, int dir)
{
	WWsSplit *split;
	
	if(obj==NULL)
		return NULL;
	
	while(1){
		if(WOBJ_IS(obj, WRegion))
			return (WRegion*)obj;
		
		assert(WOBJ_IS(obj, WWsSplit));
		
		split=(WWsSplit*)obj;
		
		if(split->dir==dir){
			obj=split->br;
			continue;
		}
		
		obj=(split->current==0 ? split->tl : split->br);
	}
	
	assert(0);
}


/*EXTL_DOC
 * Returns most recently active region on \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_current(WIonWS *ws)
{
	return left_or_topmost_current(ws->split_tree, -1);
}


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT_MEMBER
ExtlTab ionws_managed_list(WIonWS *ws)
{
	return managed_list_to_table(ws->managed_list, NULL);
}


static WWsSplit *find_split(WObj *obj, int dir, int *from)
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
		
		obj=(WObj*)split;
		split=split->parent;
	}
	
	return split;
}


static WRegion *down_or_right(WRegion *reg, int dir)
{
	WObj *prev=(WObj*)reg;
	WIonWS *ws;
	WWsSplit *split;
	int from;
	
	if(reg==NULL)
		return NULL;
	
	while(1){
		split=find_split(prev, dir, &from);
		
		if(split==NULL)
			break;
		
		if(from==TOP_OR_LEFT)
			return left_or_topmost_current(split->br, dir);
		
		prev=(WObj*)split;
	}
	
	
	return NULL;
}


static WRegion *up_or_left(WRegion *reg, int dir)
{
	WObj *prev=(WObj*)reg;
	WIonWS *ws;
	WWsSplit *split;
	int from;
	
	if(reg==NULL)
		return NULL;
	
	while(1){
		split=find_split(prev, dir, &from);
		
		if(split==NULL)
			break;
		
		if(from==BOTTOM_OR_RIGHT)
			return right_or_lowest_current(split->tl, dir);
		
		prev=(WObj*)split;
	}
	
	return NULL;
}


bool get_split_dir_primn(const char *str, int *dir, int *primn)
{
	if(str==NULL)
		return FALSE;
	
	if(!strcmp(str, "left")){
		*primn=TOP_OR_LEFT;
		*dir=HORIZONTAL;
	}else if(!strcmp(str, "right")){
		*primn=BOTTOM_OR_RIGHT;
		*dir=HORIZONTAL;
	}else if(!strcmp(str, "top") || !strcmp(str, "up")){
		*primn=TOP_OR_LEFT;
		*dir=VERTICAL;
	}else if(!strcmp(str, "bottom") || !strcmp(str, "down")){
		*primn=BOTTOM_OR_RIGHT;
		*dir=VERTICAL;
	}else{
		return FALSE;
	}
	
	return TRUE;
}


static WRegion *do_get_next_to(WIonWS *ws, WRegion *reg, int dir, int primn)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws)
		return NULL;
	
	if(primn==TOP_OR_LEFT)
		return up_or_left(reg, dir);
	else
		return down_or_right(reg, dir);
}


/*EXTL_DOC
 * Return the most previously active region next to \var{reg} in
 * direction \var{dirstr} (left/right/up/down). The region \var{reg}
 * must be managed by \var{ws}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_next_to(WIonWS *ws, WRegion *reg, const char *dirstr)
{
	int dir=0, primn=0;
	
	if(!get_split_dir_primn(dirstr, &dir, &primn))
		return NULL;
	
	return do_get_next_to(ws, reg, dir, primn);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.next_to}\code{(ws, reg, "up")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_above(WIonWS *ws, WRegion *reg)
{
	return do_get_next_to(ws, reg, VERTICAL, TOP_OR_LEFT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.nextto}\code{(ws, reg, "down")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_below(WIonWS *ws, WRegion *reg)
{
	return do_get_next_to(ws, reg, VERTICAL, BOTTOM_OR_RIGHT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.nextto}\code{(ws, reg, "left")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_left_of(WIonWS *ws, WRegion *reg)
{
	return do_get_next_to(ws, reg, HORIZONTAL, TOP_OR_LEFT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.nextto}\code{(ws, reg, "right")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_right_of(WIonWS *ws, WRegion *reg)
{
	return do_get_next_to(ws, reg, HORIZONTAL, BOTTOM_OR_RIGHT);
}


static WRegion *do_get_farthest(WIonWS *ws, int dir, int primn)
{
	if(primn==TOP_OR_LEFT)
		return left_or_topmost_current(ws->split_tree, dir);
	else
		return right_or_lowest_current(ws->split_tree, dir);
}


/*EXTL_DOC
 * Return the most previously active region on \var{ws} with no
 * other regions next to it in  direction \var{dirstr} 
 * (left/right/up/down). 
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_farthest(WIonWS *ws, const char *dirstr)
{
	int dir=0, primn=0;

	if(!get_split_dir_primn(dirstr, &dir, &primn))
		return NULL;
	
	return do_get_farthest(ws, dir, primn);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.farthest}\code{(ws, "up")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_topmost(WIonWS *ws)
{
	return do_get_farthest(ws, VERTICAL, TOP_OR_LEFT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.farthest}\code{(ws, "down")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_lowest(WIonWS *ws)
{
	return do_get_farthest(ws, VERTICAL, BOTTOM_OR_RIGHT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.farthest}\code{(ws, "left")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_leftmost(WIonWS *ws)
{
	return do_get_farthest(ws, HORIZONTAL, TOP_OR_LEFT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.farthest}\code{(ws, "right")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_rightmost(WIonWS *ws)
{
	return do_get_farthest(ws, HORIZONTAL, BOTTOM_OR_RIGHT);
}


static WRegion *do_goto_dir(WIonWS *ws, int dir, int primn)
{
	int primn2=(primn==TOP_OR_LEFT ? BOTTOM_OR_RIGHT : TOP_OR_LEFT);
	WRegion *reg=NULL, *curr=ionws_current(ws);
	if(curr!=NULL)
		reg=do_get_next_to(ws, curr, dir, primn);
	if(reg==NULL)
		reg=do_get_farthest(ws, dir, primn2);
	if(reg!=NULL)
		region_goto(reg);
	return reg;
}


/*EXTL_DOC
 * Go to the most previously active region on \var{ws} next to \var{reg} in
 * direction \var{dirstr} (up/down/left/right), wrapping around to a most 
 * recently active farthest region in the opposite direction if \var{reg} 
 * is already the further region in the given direction.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_dir(WIonWS *ws, const char *dirstr)
{
	int dir=0, primn=0;

	if(!get_split_dir_primn(dirstr, &dir, &primn))
		return NULL;
	
	return do_goto_dir(ws, dir, primn);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.goto_dir}\code{(ws, "up")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_above(WIonWS *ws)
{
	return do_goto_dir(ws, VERTICAL, TOP_OR_LEFT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.goto_dir}\code{(ws, "down")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_below(WIonWS *ws)
{
	return do_goto_dir(ws, VERTICAL, BOTTOM_OR_RIGHT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.goto_dir}\code{(ws, "left")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_left(WIonWS *ws)
{
	return do_goto_dir(ws, HORIZONTAL, TOP_OR_LEFT);
}


/*EXTL_DOC
 * Same as \fnref{WIonWS.goto_dir}\code{(ws, "right")}.
 */
EXTL_EXPORT_MEMBER
WRegion *ionws_goto_right(WIonWS *ws)
{
	return do_goto_dir(ws, HORIZONTAL, BOTTOM_OR_RIGHT);
}


/*}}}*/


/*{{{ Remove/add */


void ionws_add_managed(WIonWS *ws, WRegion *reg)
{
	region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
	
	region_add_bindmap_owned(reg, &ionws_bindmap, (WRegion*)ws);
	
	if(REGION_IS_MAPPED(ws))
		region_map(reg);
}


static bool ionws_remove_split(WIonWS *ws, WWsSplit *split)
{
	WWsSplit *split2;
	WObj *other;
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
		if((WObj*)split==split2->tl)
			split2->tl=other;
		else
			split2->br=other;
	}else{
		ws->split_tree=other;
	}
	
	if(other==NULL)
		return FALSE;
	
	set_split_of(other, split2);
	
	if(!WOBJ_IS_BEING_DESTROYED(ws)){
		nsize=split_tree_size((WObj*)split, split->dir);
		npos=split_tree_pos((WObj*)split, split->dir);
		split_tree_resize(other, split->dir, ANY, npos, nsize);
	}

	destroy_obj((WObj*)split);
	
	return TRUE;
}


void ionws_remove_managed(WIonWS *ws, WRegion *reg)
{
	WWsSplit *split;
	
	split=split_of_reg(reg);
	
	if(split!=NULL){
		WRegion *other;
		if(split->tl==(WObj*)reg){
			split->tl=NULL;
			other=left_or_topmost_current(split->br, split->dir);
		}else{
			split->br=NULL;
			other=right_or_lowest_current(split->tl, split->dir);
		}
		
		set_split_of_reg(reg, NULL);
		
		ionws_remove_split(ws, split);
		
		if(region_may_control_focus((WRegion*)ws))
			set_focus(other!=NULL ? other : (WRegion*)ws);
	}else{
		ws->split_tree=NULL;
	}

	region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));
	region_remove_bindmap_owned(reg, &ionws_bindmap, (WRegion*)ws);
	
	if(!WOBJ_IS_BEING_DESTROYED(ws) && ws->split_tree==NULL)
		defer_destroy((WObj*)ws);
}


/*}}}*/


/*{{{ managed_activated */


void ionws_managed_activated(WIonWS *ws, WRegion *reg)
{
	WWsSplit *split=split_of_reg(reg);
	WObj *prev=(WObj*)reg;
	
	while(split!=NULL){
		split->current=(split->tl==prev ? 0 : 1);
		prev=(WObj*)split;
		split=split->parent;
	}
}


/*}}}*/


/*{{{ iowns_find_rescue_manager_for */


static WRegion *do_find_nmgr(WObj *ptr, int primn)
{
	WRegion *reg;
	WWsSplit *split;
	
	do{
		if(WOBJ_IS(ptr, WRegion)){
			return (region_has_manage_clientwin((WRegion*)ptr)
					? (WRegion*)ptr : NULL);
		}
		
		if(!WOBJ_IS(ptr, WWsSplit))
			return NULL;
		
		split=(WWsSplit*)ptr;
		
		if(primn==TOP_OR_LEFT)
			reg=do_find_nmgr(split->tl, primn);
		else
			reg=do_find_nmgr(split->br, primn);
		
		if(reg!=NULL)
			return reg;
		
		if(primn==TOP_OR_LEFT)
			ptr=split->br;
		else
			ptr=split->tl;
	}while(1);
}
					  

WRegion *ionws_find_rescue_manager_for(WIonWS *ws, WRegion *reg)
{
	WWsSplit *split;
	WRegion *nmgr;
	WObj *obj;
	
	if(REGION_MANAGER(reg)!=(WRegion*)ws)
		return NULL;

	split=split_of_reg(reg);
	
	obj=(WObj*)reg;
	while(split!=NULL){
		if(split->tl==obj)
			nmgr=do_find_nmgr(split->br, TOP_OR_LEFT);
		else
			nmgr=do_find_nmgr(split->tl, BOTTOM_OR_RIGHT);
		
		if(nmgr!=NULL)
			return nmgr;
		
		obj=(WObj*)split;
		split=split->parent;
	}
	
	return NULL;
}


/*}}}*/


/*{{{ Exports */


/*EXTL_DOC
 * For region \var{reg} managed by \var{ws} return the \type{WWsSplit}
 * a leaf of which \var{reg} is.
 */
EXTL_EXPORT_MEMBER
WWsSplit *ionws_split_of(WIonWS *ws, WRegion *reg)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws){
		warn_obj("ionws_split_of", "Manager doesn't match");
		return NULL;
	}
	
	return split_of_reg(reg);
}


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
WObj *split_tl(WWsSplit *split)
{
	return split->tl;
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to bottom or right
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT_MEMBER
WObj *split_br(WWsSplit *split)
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
	return geom_to_extltab(&(split->geom));
}


/*EXTL_DOC
 * Attempt to resize and/or move the split tree starting at \var{node}
 * (\type{WWsSplit} or \type{WRegion}). Behaviour and the \var{g} 
 * parameter are as for \fnref{WRegion.request_geom} operating on
 * \var{node} (if it were a \type{WRegion}).
 */
EXTL_EXPORT_MEMBER
ExtlTab ionws_resize_tree(WIonWS *ws, WObj *node, ExtlTab g)
{
	WRectangle geom, ogeom;
	int flags=REGION_RQGEOM_WEAK_ALL;
	
	if(WOBJ_IS(node, WRegion)){
		geom=REGION_GEOM((WRegion*)node);
	}else if(WOBJ_IS(node, WWsSplit)){
		geom=((WWsSplit*)node)->geom;
	}else{
		warn("Invalid node.");
		return extl_table_none();
	}
	
	ogeom=geom;

	if(extl_table_gets_i(g, "x", &(geom.x)))
		flags&=~REGION_RQGEOM_WEAK_X;
	if(extl_table_gets_i(g, "y", &(geom.y)))
		flags&=~REGION_RQGEOM_WEAK_Y;
	if(extl_table_gets_i(g, "w", &(geom.w)))
		flags&=~REGION_RQGEOM_WEAK_W;
	if(extl_table_gets_i(g, "h", &(geom.h)))
		flags&=~REGION_RQGEOM_WEAK_H;
	
	ionws_do_request_geom(ws, node, flags, &geom, &ogeom);
	
	return geom_to_extltab(&ogeom);
}


/*}}}*/

