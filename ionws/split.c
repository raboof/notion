/*
 * ion/ionws/split.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <X11/Xmd.h>

#include <ioncore/common.h>
#include <ioncore/screen.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/objp.h>
#include <ioncore/resize.h>
#include <ioncore/attach.h>
#include <ioncore/defer.h>
#include <ioncore/reginfo.h>
#include <ioncore/extlconv.h>
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
	
	region_fit(reg, geom);
	
	return nsize;
}


static WWsSplit *split_of(WObj *obj)
{
	if(WOBJ_IS(obj, WRegion))
		return SPLIT_OF((WRegion*)obj);
	
	assert(WOBJ_IS(obj, WWsSplit));
	
	return ((WWsSplit*)obj)->parent;
}


void set_split_of(WObj *obj, WWsSplit *split)
{
	if(WOBJ_IS(obj, WRegion)){
		SPLIT_OF((WRegion*)obj)=split;
		return;
	}
	
	assert(WOBJ_IS(obj, WWsSplit));
	
	((WWsSplit*)obj)->parent=split;
}


/*}}}*/


/*{{{ Low-level resize code */


static int split_tree_calcresize(WObj *node_, int dir, int primn,
						   int nsize)
{
	WWsSplit *node;
	WObj *o1, *o2;
	int s1, s2, ns1, ns2;
	
	assert(node_!=NULL);
	
	/* Reached a window? */
	if(!WOBJ_IS(node_, WWsSplit)){
		assert(WOBJ_IS(node_, WRegion));
		return reg_calcresize((WRegion*)node_, dir, nsize);
	}
	
	node=(WWsSplit*)node_;
	
	if(node->dir!=dir){
		/* Found a split in the other direction than the resize */
		s1=split_tree_calcresize(node->tl, dir, primn, nsize);
		s2=split_tree_calcresize(node->br, dir, primn, nsize);
		
		if(s1>s2){
			/*if(nsize>split_tree_size(node->tl, dir)){
				split_tree_calcresize(node->tl, dir, primn, s2);
				s1=s2;
			}else*/{
				split_tree_calcresize(node->br, dir, primn, s1);
			}
		}else if(s2>s1){
			/*if(nsize>split_tree_size(node->br, dir)){
				split_tree_calcresize(node->br, dir, primn, s1);
			}else*/{
				split_tree_calcresize(node->tl, dir, primn, s2);
				s1=s2;
			}
		}
		node->res=ANY;
		node->knowsize=ANY;
		return (node->tmpsize=s1);
	}else{
		if(primn==TOP_OR_LEFT){
			/* Resize top or left node first */
			o1=node->tl;
			o2=node->br;
		}else{
			/* Resize bottom or right node first */
			o1=node->br;
			o2=node->tl;
			primn=BOTTOM_OR_RIGHT;
		}
		
		s2=split_tree_size(o2, dir);
		ns1=nsize-s2;
		s1=split_tree_calcresize(o1, dir, primn, ns1);
		
		/*if(s1!=ns1){*/
			ns2=nsize-s1;
			s2=split_tree_calcresize(o2, dir, primn, ns2);
			node->res=ANY;
		/*}else{
			node->res=primn;
		}*/
		
		node->knowsize=primn;
		node->tmpsize=s1;
		return s1+s2;
	}
}


int split_tree_do_resize(WObj *node_, int dir, int npos, int nsize)
{
	WWsSplit *node;
	int tls, brs;
	int s=0;
	int pos=npos;
	
	if(node_==NULL){
		return 0;
	}
	assert(node_!=NULL);
	
	/* Reached a window? */
	if(!WOBJ_IS(node_, WWsSplit)){
		assert(WOBJ_IS(node_, WRegion));
		return reg_resize((WRegion*)node_, dir, npos, nsize);
	}
	
	node=(WWsSplit*)node_;
	
	if(node->dir!=dir){
		split_tree_do_resize(node->tl, dir, npos, nsize);
		s=split_tree_do_resize(node->br, dir, npos, nsize);
	}else{
		if(node->knowsize==TOP_OR_LEFT){
			tls=node->tmpsize;
			brs=nsize-tls;
		}else if(node->knowsize==BOTTOM_OR_RIGHT){
			brs=node->tmpsize;
			tls=nsize-brs;
		}else{
			/*fprintf(stderr, "Resize is broken!");*/
			brs=nsize/2;
			tls=nsize-brs;
		}
		
		
		/*if(node->res!=BOTTOM_OR_RIGHT)*/
			s+=split_tree_do_resize(node->tl, dir, npos, tls);
		/*else
			s+=split_tree_size(node->tl, dir);*/
		
		npos+=s;
		
		/*if(node->res!=TOP_OR_LEFT)*/
			s+=split_tree_do_resize(node->br, dir, npos, brs);
		/*else
			s+=split_tree_size(node->br, dir);*/
	}
	
	if(dir==VERTICAL){
		node->geom.y=pos;
		node->geom.h=s;
	}else{
		node->geom.x=pos;
		node->geom.w=s;
	}
	
	return s;
}


/* Calculate parameters for resizing <split> and possibly anything
 * above it so that the <dir>-dimension of <from> becomes <nsize>
 * (if possible).
 */
static void wcalcres(WWsSplit *split, int dir, int primn,
					 int nsize, int from, WResizeTmp *ret)
{
	int s, s2, ds, rs, rp;
	WObj *other=(from==TOP_OR_LEFT ? split->br : split->tl);
	WWsSplit *p;
	
	s=split_tree_size((WObj*)split, dir);
	
	if(dir!=split->dir){
		/* It might not be possible to shrink the other as much */
		ds=split_tree_calcresize(other, dir, primn, nsize);
		nsize=ds;
		s2=0;
	}else{
		if(primn!=from)
			s2=split_tree_calcresize(other, dir, from, s-nsize);
		else
			s2=split_tree_calcresize(other, dir, from, split_tree_size(other, dir));
	}
	ds=nsize+s2;

	p=split->parent;
	
	if(p==NULL || ds==s){
		rs=s;
		rp=split_tree_pos((WObj*)split, dir);
		ret->postmp=rp;
		ret->sizetmp=rs;
		/* Don't have to resize the other branch if the split is not
		 * in the wanted direction
		 */
		if(split->dir!=dir)
			ret->startnode=(from==TOP_OR_LEFT ? split->tl : split->br);
		else
			ret->startnode=(WObj*)split;
	}else{
		wcalcres(p, dir, primn, ds,
				 (p->tl==(WObj*)split ? TOP_OR_LEFT : BOTTOM_OR_RIGHT),
				 ret);
		rp=ret->winpostmp;
		rs=ret->winsizetmp;
		
		if(rs!=ds && dir!=split->dir)
			split_tree_calcresize(other, dir, primn, rs);
	}
	
	nsize=rs-s2;
	
	split->knowsize=from;
	split->tmpsize=nsize;
	split->res=ANY;
	
	if(from==TOP_OR_LEFT)
		ret->winpostmp=rp;
	else
		ret->winpostmp=rp+s2;
	ret->winsizetmp=nsize;
}


static int calcresize_obj(WObj *obj, int dir, int primn, int nsize,
						  WResizeTmp *ret)
{
	WWsSplit *split=split_of(obj);
	
	nsize=split_tree_calcresize(obj, dir, primn, nsize);
	
	ret->dir=dir;
	
	if(split==NULL){
		ret->winsizetmp=ret->sizetmp=split_tree_size(obj, dir);
		ret->winpostmp=ret->postmp=split_tree_pos(obj, dir);
		ret->startnode=NULL;
		ret->dir=dir;
	}else{
		wcalcres(split, dir, primn, nsize, 
				 (split->tl==obj ? TOP_OR_LEFT : BOTTOM_OR_RIGHT),
				 ret);
	}
	return ret->winsizetmp;
}


/*}}}*/


/*{{{ Resize interface */


static int calcresize_reg(WRegion *reg, int dir, int primn, int nsize,
						  WResizeTmp *ret)
{
	return calcresize_obj((WObj*)reg, dir, primn, nsize, ret);
}


static void resize_tmp(const WResizeTmp *tmp)
{
	split_tree_do_resize(tmp->startnode, tmp->dir, tmp->postmp, tmp->sizetmp);
}


void ionws_request_managed_geom(WIonWS *ws, WRegion *sub, WRectangle geom,
								WRectangle *geomret, bool tryonly)
{
	int hprimn=ANY, vprimn=ANY;
	WResizeTmp tmp;

	if(geomret!=NULL)
		*geomret=REGION_GEOM(sub);
	
	if(REGION_GEOM(sub).w!=geom.w){
		if(REGION_GEOM(sub).x==geom.x)
			hprimn=BOTTOM_OR_RIGHT;
		else if(REGION_GEOM(sub).x+REGION_GEOM(sub).w==geom.x+geom.w)
			hprimn=TOP_OR_LEFT;
		
		calcresize_reg(sub, HORIZONTAL, hprimn, geom.w, &tmp);
		if(geomret!=NULL){
			geomret->x=tmp.winpostmp;
			geomret->w=tmp.winsizetmp;
		}
		if(!tryonly)
			resize_tmp(&tmp);
	}
	
	if(REGION_GEOM(sub).h!=geom.h){
		if(REGION_GEOM(sub).y==geom.y)
			vprimn=BOTTOM_OR_RIGHT;
		else if(REGION_GEOM(sub).y+REGION_GEOM(sub).h==geom.y+geom.h)
			vprimn=TOP_OR_LEFT;

		calcresize_reg(sub, VERTICAL, vprimn, geom.h, &tmp);
		if(geomret!=NULL){
			geomret->y=tmp.winpostmp;
			geomret->h=tmp.winsizetmp;
		}
		if(!tryonly)
			resize_tmp(&tmp);
	}
}


/*}}}*/


/*{{{ Split */


WWsSplit *create_split(int dir, WObj *tl, WObj *br, WRectangle geom)
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
	split->geom=geom;
	split->parent=NULL;
	split->current=0;
	
	return split;
}


static WRegion *do_split_at(WIonWS *ws, WObj *obj, int dir, int primn,
							int minsize, WRegionSimpleCreateFn *fn)
{
	int s, sn, gs, pos;
	WWsSplit *split, *nsplit;
	WRectangle geom;
	WRegion *nreg;
	WResizeTmp rtmp;
	WWindow *par;
	
	assert(obj!=NULL);
	
	if(primn!=TOP_OR_LEFT && primn!=BOTTOM_OR_RIGHT)
		primn=BOTTOM_OR_RIGHT;
	if(dir!=HORIZONTAL && dir!=VERTICAL)
		dir=VERTICAL;
	
	/* First possibly resize <obj> so that the area allocated by it
	 * is big enough to fit the new object and <obj> itself without
	 * them becoming too small.
	 */

	s=split_tree_size(obj, dir);
	sn=s/2;
	
	if(sn<minsize)
		sn=minsize;
	
	gs=split_tree_calcresize(obj, dir, primn, s-sn);

	if(gs+sn>s){
		s=calcresize_obj(obj, dir, ANY, gs+sn, &rtmp);
		if(gs+sn>s){
			warn("Cannot resize: minimum size reached");
			return NULL;
		}
		resize_tmp(&rtmp);
	}
	
	/* Create split and new window
	 */
	geom=split_tree_geom(obj);
	
	nsplit=create_split(dir, NULL, NULL, geom);
	
	if(nsplit==NULL)
		return NULL;
	
	if(dir==VERTICAL){
		if(primn==BOTTOM_OR_RIGHT)
			geom.y+=gs;
		geom.h=sn;
	}else{
		if(primn==BOTTOM_OR_RIGHT)
			geom.x+=gs;
		geom.w=sn;
	}
	
	par=REGION_PARENT_CHK(ws, WWindow);
	assert(par!=NULL);
	
	nreg=fn(par, geom);
	
	if(nreg==NULL){
		free(nsplit);
		return NULL;
	}
	
	ionws_add_managed(ws, nreg);
	
	/* Now that everything's ok, resize (and move) the original
	 * obj.
	 */
	pos=split_tree_pos(obj, dir);
	if(primn!=BOTTOM_OR_RIGHT)
		pos+=sn;
	s=split_tree_calcresize(obj, dir, primn, gs);
	split_tree_do_resize(obj, dir, pos, s);

	/* Set up split structure
	 */
	split=split_of(obj);
	
	set_split_of(obj, nsplit);
	SPLIT_OF(nreg)=nsplit;
	
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
	
	return do_split_at((WIonWS*)mgr, (WObj*)reg, dir, primn, minsize, fn);
}


WRegion *split_toplevel(WIonWS *ws, int dir, int primn, int minsize,
						WRegionSimpleCreateFn *fn)
{
	if(ws->split_tree==NULL)
		return NULL;
	
	return do_split_at(ws, ws->split_tree, dir, primn, minsize, fn);
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
EXTL_EXPORT
WRegion *ionws_current(WIonWS *ws)
{
	return left_or_topmost_current(ws->split_tree, -1);
}


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT
ExtlTab ionws_managed_list(WIonWS *ws)
{
	return managed_list_to_table(ws->managed_list, NULL);
}


static WWsSplit *find_split(WObj *obj, int dir, int *from)
{
	WWsSplit *split;

	if(WOBJ_IS(obj, WRegion))
		split=SPLIT_OF((WRegion*)obj);
	else
		split=((WWsSplit*)obj)->parent;
	
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


/*EXTL_DOC
 * Returns the most recently active region above \var{reg} on
 * \var{ws} or NULL.
 */
EXTL_EXPORT
WRegion *ionws_above(WIonWS *ws, WRegion *reg)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws)
		return NULL;
	return up_or_left(reg, VERTICAL);
}


/*EXTL_DOC
 * Returns the most recently active region below \var{reg} on
 * \var{ws} or NULL.
 */
EXTL_EXPORT
WRegion *ionws_below(WIonWS *ws, WRegion *reg)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws)
		return NULL;
	return down_or_right(reg, VERTICAL);
}


/*EXTL_DOC
 * Returns the most recently active region left of \var{reg} on
 * \var{ws} or NULL.
 */
EXTL_EXPORT
WRegion *ionws_left_of(WIonWS *ws, WRegion *reg)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws)
		return NULL;
	return up_or_left(reg, HORIZONTAL);
}


/*EXTL_DOC
 * Returns the most recently active region right of \var{reg} on
 * \var{ws} or NULL.
 */
EXTL_EXPORT
WRegion *ionws_right_of(WIonWS *ws, WRegion *reg)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws)
		return NULL;
	return down_or_right(reg, HORIZONTAL);
}

/*EXTL_DOC
 * Returns the most recently active region on \var{ws} with
 * no other regions above it.
 */
EXTL_EXPORT
WRegion *ionws_topmost(WIonWS *ws)
{
	return left_or_topmost_current(ws->split_tree, VERTICAL);
}


/*EXTL_DOC
 * Returns the most recently active region on \var{ws} with
 * no other regions below it.
 */
EXTL_EXPORT
WRegion *ionws_lowest(WIonWS *ws)
{
	return right_or_lowest_current(ws->split_tree, VERTICAL);
}


/*EXTL_DOC
 * Returns the most recently active region on \var{ws} with
 * no other regions left of it.
 */
EXTL_EXPORT
WRegion *ionws_leftmost(WIonWS *ws)
{
	return left_or_topmost_current(ws->split_tree, HORIZONTAL);
}


/*EXTL_DOC
 * Returns the most recently active region on \var{ws} with
 * no other regions right of it.
 */
EXTL_EXPORT
WRegion *ionws_rightmost(WIonWS *ws)
{
	return right_or_lowest_current(ws->split_tree, HORIZONTAL);
}


static bool goto_reg(WRegion *reg)
{
	if(reg==NULL)
		return FALSE;
	region_goto(reg);
	return TRUE;
}


/*EXTL_DOC
 * Switch input the most the most recently active region above
 * the current object on \var{ws} wrapping around to the
 * lowest most recently active region if there is nothing above
 * the current object.
 */
EXTL_EXPORT
void ionws_goto_above(WIonWS *ws)
{
	if(!goto_reg(ionws_above(ws, ionws_current(ws))))
		goto_reg(ionws_lowest(ws));
	   
}


/*EXTL_DOC
 * Similar to \fnref{ionws_goto_above}.
 */
EXTL_EXPORT
void ionws_goto_below(WIonWS *ws)
{
	if(!goto_reg(ionws_below(ws, ionws_current(ws))))
		goto_reg(ionws_topmost(ws));
}


/*EXTL_DOC
 * Similar to \fnref{ionws_goto_above}.
 */
EXTL_EXPORT
void ionws_goto_left(WIonWS *ws)
{
	if(!goto_reg(ionws_left_of(ws, ionws_current(ws))))
		goto_reg(ionws_rightmost(ws));
}


/*EXTL_DOC
 * Similar to \fnref{ionws_goto_above}.
 */
EXTL_EXPORT
void ionws_goto_right(WIonWS *ws)
{
	if(!goto_reg(ionws_right_of(ws, ionws_current(ws))))
		goto_reg(ionws_leftmost(ws));
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
		
	if(WOBJ_IS(other, WRegion))
		SPLIT_OF((WRegion*)other)=split2;
	else
		((WWsSplit*)other)->parent=split2;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		nsize=split_tree_size((WObj*)split, split->dir);
		npos=split_tree_pos((WObj*)split, split->dir);
		nsize=split_tree_calcresize(other, split->dir, primn, nsize);
		split_tree_do_resize(other, split->dir, npos, nsize);
	}

	free(split);
	
	return TRUE;
}


void ionws_remove_managed(WIonWS *ws, WRegion *reg)
{
	WWsSplit *split;
	
	region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));

	region_remove_bindmap_owned(reg, &ionws_bindmap, (WRegion*)ws);

	split=SPLIT_OF(reg);
	
	if(split!=NULL){
		WRegion *other;
		if(split->tl==(WObj*)reg){
			split->tl=NULL;
			other=left_or_topmost_current(split->br, split->dir);
		}else{
			split->br=NULL;
			other=right_or_lowest_current(split->tl, split->dir);
		}
		
		SPLIT_OF(reg)=NULL;
		
		ionws_remove_split(ws, split);
		
		if(region_may_control_focus((WRegion*)ws))
			set_focus(other!=NULL ? other : (WRegion*)ws);
	}else{
		ws->split_tree=NULL;
	}
	
	if(ws->split_tree==NULL)
		defer_destroy((WObj*)ws);
}


/*}}}*/


/*{{{ managed_activated */


void ionws_managed_activated(WIonWS *ws, WRegion *reg)
{
	WWsSplit *split=SPLIT_OF(reg);
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
			return (region_can_manage_clientwins((WRegion*)ptr)
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
		return FALSE;

	split=SPLIT_OF(reg);
	
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
EXTL_EXPORT
WWsSplit *ionws_split_of(WIonWS *ws, WRegion *reg)
{
	if(REGION_MANAGER(reg)!=(WRegion*)ws){
		warn_obj("ionws_split_of", "Manager doesn't match");
		return NULL;
	}
	
	return split_of((WObj*)reg);
}


/*EXTL_DOC
 * Return parent split for \var{split}.
 */
EXTL_EXPORT
WWsSplit *split_parent(WWsSplit *split)
{
	return split->parent;
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to top or left
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT
WObj *split_tl(WWsSplit *split)
{
	return split->tl;
}


/*EXTL_DOC
 * Return the object (region or split) corresponding to bottom or right
 * sibling of \var{split} depending on the split's direction.
 */
EXTL_EXPORT
WObj *split_br(WWsSplit *split)
{
	return split->br;
}


/*EXTL_DOC
 * Is \var{split} a vertical split?
 */
EXTL_EXPORT
bool split_is_vertical(WWsSplit *split)
{
	return (split->dir==VERTICAL);
}


/*EXTL_DOC
 * Is \var{split} a horizontal split?
 */
EXTL_EXPORT
bool split_is_horizontal(WWsSplit *split)
{
	return (split->dir==VERTICAL);
}


/*EXTL_DOC
 * Returns the area of workspace used by the regions under \var{split}.
 */
EXTL_EXPORT
ExtlTab split_geom(WWsSplit *split)
{
	return geom_to_extltab(split->geom);
}


/*}}}*/

