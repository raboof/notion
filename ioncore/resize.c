/* 
 * ion/ioncore/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "resize.h"
#include "draw.h"
#include "sizehint.h"
#include "event.h"
#include "cursor.h"
#include "objp.h"
#include "extl.h"
#include "extlconv.h"
#include "grab.h"
#include "genframep.h"


#define XOR_RESIZE (!wglobal.opaque_resize)

static bool resize_mode=FALSE;

static XSizeHints tmphints;
static uint tmprelw, tmprelh;
static int tmpdx1, tmpdx2, tmpdy1, tmpdy2;
static WRectangle tmporiggeom;
static WRectangle tmpgeom;
static WRegion *tmpreg;
static WDrawRubberbandFn *tmprubfn=NULL;
static int parent_rx, parent_ry;
static enum {MOVERES_SIZE, MOVERES_POS} moveres_mode=MOVERES_SIZE;
static bool resize_cumulative=FALSE;
static int tmprqflags=0;

/*static WWatch tmpregwatch=WWATCH_INIT;
#define tmpreg ((WRegion*)(tmpregwatch.obj))
*/

/*{{{ Dynfuns */


void region_resize_hints(WRegion *reg, XSizeHints *hints_ret,
						 uint *relw_ret, uint *relh_ret)
{
	hints_ret->flags=0;
	hints_ret->min_width=1;
	hints_ret->min_height=1;
	if(relw_ret!=NULL)
		*relw_ret=REGION_GEOM(reg).w;
	if(relh_ret!=NULL)
		*relh_ret=REGION_GEOM(reg).h;
	{
		CALL_DYN(region_resize_hints, reg, (reg, hints_ret, relw_ret, relh_ret));
	}
}


/*}}}*/


/*{{{ Size display and rubberband */


static void res_draw_moveres(WRootWin *rootwin)
{
	if(moveres_mode==MOVERES_SIZE){
		int w, h;
		
		w=(tmpgeom.w-tmporiggeom.w)+tmprelw;
		h=(tmpgeom.h-tmporiggeom.h)+tmprelh;

		if((tmphints.flags&PResizeInc) &&
		   (tmphints.width_inc>1 || tmphints.height_inc>1)){
			if(tmphints.flags&PBaseSize){
				w-=tmphints.base_width;
				h-=tmphints.base_height;
			}
			w/=tmphints.width_inc;
			h/=tmphints.height_inc;
		}
	
		set_moveres_size(rootwin, w, h);
	}else{
		set_moveres_pos(rootwin, tmpgeom.x, tmpgeom.y);
	}
}


static void res_draw_rubberband(WRootWin *rootwin)
{
	WRectangle rgeom=tmpgeom;
	int rx, ry;
	
	rgeom.x+=parent_rx;
	rgeom.y+=parent_ry;

	if(tmprubfn==NULL)
		draw_rubberbox(rootwin, rgeom);
	else
		tmprubfn(rootwin, rgeom);
}


/*}}}*/


/*{{{ Resize */


bool is_resizing()
{
	return resize_mode;
}


bool may_resize(WRegion *reg)
{
	return (tmpreg==reg);
}


/* It is ugly to do this here, but it will have to do for now... */
static void set_saved(WRegion *reg)
{
	WGenFrame *frame;
	
	if(!WOBJ_IS(reg, WGenFrame))
		return;
	
	frame=(WGenFrame*)reg;
	
	/* Restore saved sizes from the beginning of the resize action */
	if(tmporiggeom.w!=tmpgeom.w){
		frame->saved_x=tmporiggeom.x;
		frame->saved_w=tmporiggeom.w;
	}
	
	if(tmporiggeom.h!=tmpgeom.h){
		frame->saved_y=tmporiggeom.y;
		frame->saved_h=tmporiggeom.h;
	}
}


static void do_end_resize(WRegion *reg, bool dors)
{
	WRootWin *rootwin;
	
	resize_mode=FALSE;
	rootwin=ROOTWIN_OF(reg);
	
	if(XOR_RESIZE){
		res_draw_rubberband(rootwin);
		if(dors){
			region_request_geom(reg, tmprqflags&~REGION_RQGEOM_TRYONLY,
								tmpgeom, &tmpgeom);
		}
		XUngrabServer(wglobal.dpy);
	}
	if(dors)
		set_saved(reg);
	
	XUnmapWindow(wglobal.dpy, rootwin->grdata.moveres_win);
}


/*static void tmpreg_watch_handler(WWatch *watch, WRegion *reg)
{
	D(warn("Object destroyed while resizing."));
	do_end_resize(reg, FALSE);
}*/


static bool begin_moveres(WRegion *reg, WDrawRubberbandFn *rubfn,
						  bool cumulative, int cursor)
{
	WRootWin *rootwin=ROOTWIN_OF(reg);
	WRegion *parent;
	
	if(tmpreg!=NULL)
		return FALSE;
	
	region_resize_hints(reg, &tmphints, &tmprelw, &tmprelh);

	parent=REGION_PARENT_CHK(reg, WRegion);
	if(parent==NULL){
		parent_rx=0;
		parent_ry=0;
	}else{
		region_rootpos(parent, &parent_rx, &parent_ry);
	}
	
	tmpgeom=REGION_GEOM(reg);
	tmporiggeom=REGION_GEOM(reg);
	tmpdx1=0;
	tmpdx2=0;
	tmpdy1=0;
	tmpdy2=0;
	tmprubfn=rubfn;
	resize_cumulative=cumulative;
	resize_mode=TRUE;
	tmprqflags=(XOR_RESIZE ? REGION_RQGEOM_TRYONLY : 0);

	/*setup_watch(&tmpregwatch, (WObj*)reg,
				(WWatchHandler*)tmpreg_watch_handler);*/
	tmpreg=reg;
	
	if(!tmphints.flags&PMinSize || tmphints.min_width<1)
		tmphints.min_width=1;
	if(!tmphints.flags&PMinSize || tmphints.min_height<1)
		tmphints.min_height=1;
	
	if(XOR_RESIZE)
		XGrabServer(wglobal.dpy);
	
	XMoveWindow(wglobal.dpy, rootwin->grdata.moveres_win,
				parent_rx+tmpgeom.x+(tmpgeom.w-rootwin->grdata.moveres_geom.w)/2,
				parent_ry+tmpgeom.y+(tmpgeom.h-rootwin->grdata.moveres_geom.h)/2);
	XMapRaised(wglobal.dpy, rootwin->grdata.moveres_win);
	
	res_draw_moveres(rootwin);
	
	if(XOR_RESIZE)
		res_draw_rubberband(rootwin);

	change_grab_cursor(cursor);
	
	return TRUE;
}


bool begin_resize(WRegion *reg, WDrawRubberbandFn *rubfn, bool cumulative)
{
	moveres_mode=MOVERES_SIZE;
	return begin_moveres(reg, rubfn, cumulative, CURSOR_RESIZE);
}


bool begin_move(WRegion *reg, WDrawRubberbandFn *rubfn, bool cumulative)
{
	moveres_mode=MOVERES_POS;
	return begin_moveres(reg, rubfn, cumulative, CURSOR_MOVE);
}



static void delta_moveres(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
						  WRectangle *rret)
{
	WRootWin *rootwin=ROOTWIN_OF(reg);
	WRectangle geom;
	int w, h;
	
	if(tmpreg!=reg || reg==NULL)
		return;
	
	if(!resize_cumulative){
		tmpdx1=dx1;
		tmpdx2=dx2;
		tmpdy1=dy1;
		tmpdy2=dy2;
		geom=tmpgeom;
	}else{
		tmpdx1+=dx1;
		tmpdx2+=dx2;
		tmpdy1+=dy1;
		tmpdy2+=dy2;
		geom=tmporiggeom;
	}
	
	w=tmprelw-tmpdx1+tmpdx2;
	h=tmprelh-tmpdy1+tmpdy2;
	
	if(w<=0)
		w=tmphints.min_width;
	if(h<=0)
		h=tmphints.min_height;
	
	correct_size(&w, &h, &tmphints, TRUE);
	/* Do not alter sizes that were not changed */
	if(tmpdx1==tmpdx2)
		w=tmprelw;
	if(tmpdy1==tmpdy2)
		h=tmprelh;
	
	geom.w=geom.w-tmprelw+w;
	geom.h=geom.h-tmprelh+h;

	if(!resize_cumulative){
		tmprelw=w;
		tmprelh=h;
	}

	/* If top/left delta is not zero, don't move bottom/right side
	 * if the respective delta is zero
	 */
	
	if(tmpdx1!=0){
		if(tmpdx2==0)
			geom.x+=(resize_cumulative ? tmporiggeom.w : tmpgeom.w)-geom.w;
		else
			geom.x+=tmpdx1;
	}

	if(tmpdy1!=0){
		if(tmpdy2==0)
			geom.y+=(resize_cumulative ? tmporiggeom.h : tmpgeom.h)-geom.h;
		else
			geom.y+=tmpdy1;
	}
	
	region_request_geom(reg, tmprqflags, geom, &geom);

	if(XOR_RESIZE)
		res_draw_rubberband(rootwin);

	tmpgeom=geom;
	
	res_draw_moveres(rootwin);
	
	if(XOR_RESIZE)
		res_draw_rubberband(rootwin);
	
	if(rret!=NULL)
		*rret=tmpgeom;
}


void delta_resize(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
				  WRectangle *rret)
{
	moveres_mode=MOVERES_SIZE;
	delta_moveres(reg, dx1, dx2, dy1, dy2, rret);
}


void delta_move(WRegion *reg, int dx, int dy, WRectangle *rret)
{
	moveres_mode=MOVERES_POS;
	delta_moveres(reg, dx, dx, dy, dy, rret);
}


bool end_resize()
{
	WRegion *reg=tmpreg;
	if(reg!=NULL){
		/*reset_watch(&tmpregwatch);*/
		tmpreg=NULL;
		do_end_resize(reg, TRUE);
		return TRUE;
	}
	return FALSE;
}


bool cancel_resize()
{
	WRegion *reg=tmpreg;
	if(reg!=NULL){
		/*reset_watch(&tmpregwatch);*/
		tmpreg=NULL;
		do_end_resize(reg, FALSE);
		return TRUE;
	}
	return FALSE;
}


/*}}}*/


/*{{{ Request */


void region_request_geom(WRegion *reg, int flags, WRectangle geom,
						 WRectangle *geomret)
{
	bool tryonly=(flags&REGION_RQGEOM_TRYONLY);
	
	if(REGION_MANAGER(reg)!=NULL){
		region_request_managed_geom(REGION_MANAGER(reg), reg, flags, geom,
									geomret);
	}else{
		if(geomret!=NULL)
			*geomret=REGION_GEOM(reg);
		if(!tryonly)
			region_fit(reg, geom);
	}

	/*if(!tryonly && geomret!=NULL)
		*geomret=REGION_GEOM(reg);*/
}


/*EXTL_DOC
 * Attempt to resize and/or move \var{reg}. The table \var{g} is a usual
 * geometry specification (fields \var{x}, \var{y}, \var{w} and \var{h}),
 * but may contain missing fields, in which case, \var{reg}'s manager may
 * attempt to leave that attribute unchanged.
 */
EXTL_EXPORT_AS(region_request_geom)
ExtlTab region_request_geom_extl(WRegion *reg, ExtlTab g)
{
	WRectangle geom=REGION_GEOM(reg);
	int flags=REGION_RQGEOM_WEAK_ALL;
	
	if(extl_table_gets_i(g, "x", &(geom.x)))
	   flags&=~REGION_RQGEOM_WEAK_X;
	if(extl_table_gets_i(g, "y", &(geom.y)))
	   flags&=~REGION_RQGEOM_WEAK_Y;
	if(extl_table_gets_i(g, "w", &(geom.w)))
	   flags&=~REGION_RQGEOM_WEAK_W;
	if(extl_table_gets_i(g, "h", &(geom.h)))
	   flags&=~REGION_RQGEOM_WEAK_H;
	
	region_request_geom(reg, flags, geom, &geom);
	
	return geom_to_extltab(geom);
}


void region_request_managed_geom(WRegion *mgr, WRegion *reg,
								 int flags, WRectangle geom,
								 WRectangle *geomret)
{
	CALL_DYN(region_request_managed_geom, mgr,
			 (mgr, reg, flags, geom, geomret));
}


void region_request_clientwin_geom(WRegion *mgr, WClientWin *cwin,
								   int flags, WRectangle geom)
{
	CALL_DYN(region_request_clientwin_geom, mgr, (mgr, cwin, flags, geom));
}


void region_request_managed_geom_allow(WRegion *mgr, WRegion *reg,
									   int flags, WRectangle geom,
									   WRectangle *geomret)
{
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!(flags&REGION_RQGEOM_TRYONLY))
		region_fit(reg, geom);
}


void region_request_managed_geom_unallow(WRegion *mgr, WRegion *reg,
										 int flags, WRectangle geom,
										 WRectangle *geomret)
{
	if(geomret!=NULL)
		*geomret=REGION_GEOM(reg);
}


/*}}}*/


/*{{{ Restore size, maximize, shade */


void genframe_restore_size(WGenFrame *frame, bool horiz, bool vert)
{
	WRectangle geom;
	int rqf=REGION_RQGEOM_WEAK_ALL;

	geom=REGION_GEOM(frame);
	
	if(vert && frame->flags&WGENFRAME_SAVED_VERT){
		geom.y=frame->saved_y;
		geom.h=frame->saved_h;
		rqf&=~(REGION_RQGEOM_WEAK_Y|REGION_RQGEOM_WEAK_H);
	}

	if(horiz && frame->flags&WGENFRAME_SAVED_HORIZ){
		geom.x=frame->saved_x;
		geom.w=frame->saved_w;
		rqf&=~(REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_W);
	}
	
	if((rqf&REGION_RQGEOM_WEAK_ALL)!=REGION_RQGEOM_WEAK_ALL)
		region_request_geom((WRegion*)frame, rqf, geom, NULL);
}


static bool trymaxv(WGenFrame *frame, WRegion *mgr, int tryonlyflag)
{
	WRectangle geom=REGION_GEOM(frame);
	geom.y=0;
	geom.h=REGION_GEOM(mgr).h;
	region_request_geom((WRegion*)frame, 
						tryonlyflag|REGION_RQGEOM_VERT_ONLY, 
						geom, &geom);
	return (abs(geom.y-REGION_GEOM(frame).y)>1 ||
			abs(geom.h-REGION_GEOM(frame).h)>1);
}


/*EXTL_DOC
 * Attempt to maximize \var{frame} vertically.
 */
EXTL_EXPORT
void genframe_maximize_vert(WGenFrame *frame)
{
	WRegion *mgr=REGION_MANAGER(frame);
	
	if(frame->flags&WGENFRAME_SHADED){
		genframe_do_toggle_shade(frame, 0 /* not used */);
		return;
	}
		
	if(mgr==NULL)
		return;

	if(!trymaxv(frame, mgr, REGION_RQGEOM_TRYONLY)){
		/* Could not maximize further, restore */
		genframe_restore_size(frame, FALSE, TRUE);
		return;
	}

	trymaxv(frame, mgr, 0);
}


static bool trymaxh(WGenFrame *frame, WRegion *mgr, int tryonlyflag)
{
	WRectangle geom=REGION_GEOM(frame);
	geom.x=0;
	geom.w=REGION_GEOM(mgr).w;
	region_request_geom((WRegion*)frame, 
						tryonlyflag|REGION_RQGEOM_HORIZ_ONLY, 
						geom, &geom);
	return (abs(geom.x-REGION_GEOM(frame).x)>1 ||
			abs(geom.w-REGION_GEOM(frame).w)>1);
}
				   
/*EXTL_DOC
 * Attempt to maximize \var{frame} horizontally.
 */
EXTL_EXPORT
void genframe_maximize_horiz(WGenFrame *frame)
{
	WRegion *mgr=REGION_MANAGER(frame);
	
	if(mgr==NULL)
		return;

	if(!trymaxh(frame, mgr, REGION_RQGEOM_TRYONLY)){
		/* Could not maximize further, restore */
		genframe_restore_size(frame, TRUE, FALSE);
		return;
	}
	
	trymaxh(frame, mgr, 0);
}


void genframe_do_toggle_shade(WGenFrame *frame, int shaded_h)
{
	WRectangle geom=REGION_GEOM(frame);

	if(frame->flags&WGENFRAME_SHADED){
		if(!(frame->flags&WGENFRAME_SAVED_VERT))
			return;
		geom.h=frame->saved_h;
	}else{
		if(frame->flags&WGENFRAME_TAB_HIDE)
			return;
		geom.h=shaded_h;
	}
	
	region_request_geom((WRegion*)frame, REGION_RQGEOM_H_ONLY,
						geom, NULL);
}


/*EXTL_DOC
 * Is \var{frame} shaded?
 */
EXTL_EXPORT
bool genframe_is_shaded(WGenFrame *frame)
{
	return ((frame->flags&WGENFRAME_SHADED)!=0);
}


/*}}}*/


/*{{{ Misc. */


#define REG_MIN_WH(REG, WHL)                                 \
	XSizeHints hints;                                        \
	uint relwidth, relheight;                                \
	region_resize_hints(reg, &hints, &relwidth, &relheight); \
	return (hints.flags&PMinSize ? hints.min_##WHL : 1);
	
	
uint region_min_h(WRegion *reg)
{
	XSizeHints hints;
	uint relw, relh;
	region_resize_hints(reg, &hints, &relw, &relh);
	return (hints.flags&PMinSize ? hints.min_height : 1)
		+REGION_GEOM(reg).h-relh;
}


uint region_min_w(WRegion *reg)
{
	XSizeHints hints;
	uint relw, relh;
	region_resize_hints(reg, &hints, &relw, &relh);
	return (hints.flags&PMinSize ? hints.min_width : 1)
		+REGION_GEOM(reg).w-relw;
}


void genframe_resize_units(WGenFrame *genframe, int *wret, int *hret)
{
	WGRData *grdata=GRDATA_OF(genframe);
	*wret=grdata->w_unit;
	*hret=grdata->h_unit;
	
	if(WGENFRAME_CURRENT(genframe)!=NULL){
		XSizeHints hints;
		
		region_resize_hints(WGENFRAME_CURRENT(genframe), &hints, NULL, NULL);
		
		if(hints.flags&PResizeInc &&
		   (hints.width_inc>1 || hints.height_inc>1)){
			*wret=hints.width_inc;
			*hret=hints.height_inc;
		}
	}
}


/*}}}*/

