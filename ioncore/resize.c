/* 
 * ion/ioncore/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "resize.h"
#include "signal.h"
#include "draw.h"
#include "sizehint.h"
#include "event.h"
#include "cursor.h"
#include "objp.h"
#include "grab.h"


#define XOR_RESIZE (!wglobal.opaque_resize)

static bool resize_mode=FALSE;

static XSizeHints tmphints;
static uint tmprelw, tmprelh;
static int tmpdx1, tmpdx2, tmpdy1, tmpdy2;
static WRectangle tmporiggeom;
static WRectangle tmpgeom;
static WRegion *tmpreg=NULL;
static WDrawRubberbandFn *tmprubfn=NULL;
static void (*resize_atexit)();
static int parent_rx, parent_ry;
static enum {MOVERES_SIZE, MOVERES_POS} moveres_mode=MOVERES_SIZE;
static bool resize_cumulative=FALSE;


/*{{{ Dynfuns */


void region_resize_hints(WRegion *reg, XSizeHints *hints_ret,
						 uint *relw_ret, uint *relh_ret)
{
	hints_ret->flags=0;
	hints_ret->min_width=1;
	hints_ret->min_height=1;
	*relw_ret=REGION_GEOM(reg).w;
	*relh_ret=REGION_GEOM(reg).h;
	{
		CALL_DYN(region_resize_hints, reg, (reg, hints_ret, relw_ret, relh_ret));
	}
}


/*}}}*/


/*{{{ Resize timer */


static void tmr_end_resize(WTimer *unused)
{
	if(!resize_mode)
		return;
	
	assert(tmpreg!=NULL);
	
	end_resize(tmpreg);
}


static WTimer resize_timer=INIT_TIMER(tmr_end_resize);


void set_resize_timer(WRegion *reg, uint timeout)
{
	assert(tmpreg==reg && tmpreg!=NULL);
	set_timer(&resize_timer, timeout);
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
		draw_rubberbox((WWindow*)rootwin, rgeom);
	else
		tmprubfn((WWindow*)rootwin, rgeom);
	
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
	tmpreg=reg;
	tmprubfn=rubfn;
	resize_cumulative=cumulative;
	
	resize_mode=TRUE;
	
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


bool begin_resize_atexit(WRegion *reg, WDrawRubberbandFn *rubfn, 
						 bool cumulative, void (*exitfn)())
{
	if(begin_resize(reg, rubfn, cumulative)){
		resize_atexit=exitfn;
		return TRUE;
	}
	return FALSE;
}


static void delta_moveres(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
						  WRectangle *rret)
{
	WRootWin *rootwin=ROOTWIN_OF(reg);
	WRectangle geom;
	int w, h;
	
	assert(tmpreg==reg);
	
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
	
	region_request_geom(reg, geom, &geom, XOR_RESIZE);

	if(XOR_RESIZE)
		res_draw_rubberband(rootwin);
	
	res_draw_moveres(rootwin);
	
	tmpgeom=geom;
	
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


void end_resize(WRegion *reg)
{
	WRootWin *rootwin=ROOTWIN_OF(reg);
	
	assert(tmpreg==reg);

	resize_mode=FALSE;
	tmpreg=NULL;
	
	reset_timer(&resize_timer);
	
	if(XOR_RESIZE){
		res_draw_rubberband(rootwin);
		region_request_geom(reg, tmpgeom, &tmpgeom, FALSE);
		XUngrabServer(wglobal.dpy);
	}
	
	set_saved(reg);
	
	XUnmapWindow(wglobal.dpy, rootwin->grdata.moveres_win);

	if(resize_atexit){
		resize_atexit();
		resize_atexit=NULL;
	}
}


void cancel_resize(WRegion *reg)
{
	WRootWin *rootwin=ROOTWIN_OF(reg);

	assert(tmpreg==reg);
	
	resize_mode=FALSE;
	tmpreg=NULL;
	
	reset_timer(&resize_timer);
	
	if(XOR_RESIZE){
		res_draw_rubberband(rootwin);
		XUngrabServer(wglobal.dpy);
	}else{
		set_saved(reg);
	}
	
	XUnmapWindow(wglobal.dpy, rootwin->grdata.moveres_win);

	if(resize_atexit){
		resize_atexit();
		resize_atexit=NULL;
	}
}


/*}}}*/


/*{{{ Request */


void region_request_geom(WRegion *reg,
						 WRectangle geom, WRectangle *geomret,
						 bool tryonly)
{
	if(REGION_MANAGER(reg)!=NULL){
		region_request_managed_geom(REGION_MANAGER(reg), reg, geom, geomret, 
									tryonly);
	}else{
		if(geomret!=NULL)
			*geomret=REGION_GEOM(reg);
		if(!tryonly)
			region_fit(reg, geom);
	}

	if(!tryonly && geomret!=NULL)
		*geomret=REGION_GEOM(reg);
}


/*}}}*/


/*{{{ set_width etc. */


/*EXTL_DOC
 * Attempt to set the width of \var{reg} to \var{w} pixels.
 */
EXTL_EXPORT
void region_set_w(WRegion *reg, int w)
{
	WRectangle geom=REGION_GEOM(reg);
	if(w>0){
		geom.w=w;
		region_request_geom(reg, geom, &geom, FALSE);
	}
}


/*EXTL_DOC
 * Attempt to set the height of \var{reg} to \var{h} pixels.
 */
EXTL_EXPORT
void region_set_h(WRegion *reg, int h)
{
	WRectangle geom=REGION_GEOM(reg);
	if(h>0){
		geom.h=h;
		region_request_geom(reg, geom, &geom, FALSE);
	}
}


/*}}}*/


/*{{{ Restore size, maximize, shade */

#if 0
void save_vert(WGenFrame *frame, bool override)
{
	if(override || !(frame->flags&WGENFRAME_SAVED_VERT)){
		frame->flags|=WGENFRAME_SAVED_VERT;
		frame->saved_y=REGION_GEOM(frame).y;
		frame->saved_h=REGION_GEOM(frame).h;
	}
}


static void save_horiz(WGenFrame *frame, bool override)
{
	if(override || !(frame->flags&WGENFRAME_SAVED_HORIZ)){
		frame->flags|=WGENFRAME_SAVED_HORIZ;
		frame->saved_x=REGION_GEOM(frame).x;
		frame->saved_w=REGION_GEOM(frame).w;
	}
}
#endif


void genframe_restore_size(WGenFrame *frame, bool horiz, bool vert)
{
	WRectangle geom;
	
	if(!(frame->flags&WGENFRAME_SAVED_VERT))
		vert=FALSE;
	if(!(frame->flags&WGENFRAME_SAVED_HORIZ))
		horiz=FALSE;
	
	if(!vert && !horiz)
		return;
	
	geom=REGION_GEOM(frame);
	
	if(vert){
		geom.h=frame->saved_h;
		geom.y=frame->saved_y;
	}
	
	if(horiz){
		geom.w=frame->saved_w;
		geom.x=frame->saved_x;
	}
	
	region_request_geom((WRegion*)frame, geom, NULL, FALSE);
}


static bool trymaxv(WGenFrame *frame, WRegion *mgr, bool tryonly)
{
	WRectangle geom=REGION_GEOM(frame);
	geom.y=0;
	geom.h=REGION_GEOM(mgr).h;
	region_request_geom((WRegion*)frame, geom, &geom, tryonly);
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

	if(!trymaxv(frame, mgr, TRUE)){
		/* Could not maximize further, restore */
		genframe_restore_size(frame, FALSE, TRUE);
		return;
	}

	trymaxv(frame, mgr, FALSE);
}


static bool trymaxh(WGenFrame *frame, WRegion *mgr, bool tryonly)
{
	WRectangle geom=REGION_GEOM(frame);
	geom.x=0;
	geom.w=REGION_GEOM(mgr).w;
	region_request_geom((WRegion*)frame, geom, &geom, tryonly);
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

	if(!trymaxh(frame, mgr, TRUE)){
		/* Could not maximize further, restore */
		genframe_restore_size(frame, TRUE, FALSE);
		return;
	}
	
	trymaxh(frame, mgr, FALSE);
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
	/* TODO: region_request_geom should support specifying only size 
	 * so this gets handled correctly on WIonWS:s.
	 */
	
	region_request_geom((WRegion*)frame, geom, NULL, FALSE);
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
	WRootWin *rootwin=ROOTWIN_OF(genframe);
	*wret=rootwin->w_unit;
	*hret=rootwin->h_unit;
	
	if(genframe->current_sub!=NULL){
		uint dummyrelw,  dummyrelh;
		XSizeHints hints;
		
		region_resize_hints(genframe->current_sub, &hints,
							&dummyrelw, &dummyrelh);
		
		if(hints.flags&PResizeInc &&
		   (hints.width_inc>1 || hints.height_inc>1)){
			*wret=hints.width_inc;
			*hret=hints.height_inc;
		}
	}
}


/*}}}*/

