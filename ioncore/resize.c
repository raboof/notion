/* 
 * wmcore/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
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


static void res_draw_moveres(WScreen *scr)
{
	int w, h;
	
	w=tmpgeom.w-(tmporiggeom.w-tmprelw);
	h=tmpgeom.h-(tmporiggeom.h-tmprelh);

	if(tmphints.flags&PBaseSize){
		w-=tmphints.base_width;
		h-=tmphints.base_height;
	}
	
	if(tmphints.flags&PResizeInc &&
	   tmphints.width_inc>0 && tmphints.height_inc>0){
		w/=tmphints.width_inc;
		h/=tmphints.height_inc;
	}
	
	set_moveres_size(scr, w, h);
}


static void res_draw_rubberband(WScreen *scr)
{
	if(tmprubfn==NULL)
		draw_rubberbox(scr, tmpgeom);
	else
		tmprubfn(scr, tmpgeom);
	
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


bool begin_resize(WRegion *reg, WDrawRubberbandFn *rubfn)
{
	WScreen *scr=SCREEN_OF(reg);
	
	if(tmpreg!=NULL)
		return FALSE;
	
	region_resize_hints(reg, &tmphints, &tmprelw, &tmprelh);

	tmpgeom=REGION_GEOM(reg);
	tmporiggeom=REGION_GEOM(reg);
	tmpdx1=0;
	tmpdx2=0;
	tmpdy1=0;
	tmpdy2=0;
	tmpreg=reg;
	tmprubfn=rubfn;
	
	resize_mode=TRUE;
	
	if(!tmphints.flags&PMinSize || tmphints.min_width<1)
		tmphints.min_width=1;
	if(!tmphints.flags&PMinSize || tmphints.min_height<1)
		tmphints.min_height=1;
	
	if(XOR_RESIZE)
		XGrabServer(wglobal.dpy);
	
	XMoveWindow(wglobal.dpy, scr->grdata.moveres_win,
				tmpgeom.x+(tmpgeom.w-scr->grdata.moveres_geom.w)/2,
				tmpgeom.y+(tmpgeom.h-scr->grdata.moveres_geom.h)/2);
	XMapRaised(wglobal.dpy, scr->grdata.moveres_win);
	
	res_draw_moveres(scr);
	
	if(XOR_RESIZE)
		res_draw_rubberband(scr);

	change_grab_cursor(CURSOR_RESIZE);
	
	return TRUE;
}

bool begin_resize_atexit(WRegion *reg, WDrawRubberbandFn *rubfn, void (*exitfn)())
{
	if(begin_resize(reg, rubfn)){
		resize_atexit=exitfn;
		return TRUE;
	}
	return FALSE;
}

void delta_resize(WRegion *reg, int dx1, int dx2, int dy1, int dy2,
				  WRectangle *rret)
{
	WScreen *scr=SCREEN_OF(reg);
	WRectangle geom;
	int w, h;
	
	assert(tmpreg==reg);
	
	tmpdx1+=dx1;
	tmpdx2+=dx2;
	tmpdy1+=dy1;
	tmpdy2+=dy2;
	
	geom=tmporiggeom;
	w=tmprelw-tmpdx1+tmpdx2;
	h=tmprelh-tmpdy1+tmpdy2;
	correct_size(&w, &h, &tmphints, TRUE);
	geom.w=geom.w-tmprelw+w;
	geom.h=geom.h-tmprelh+h;
	
	/* If top/left delta is not zero, don't move bottom/right side
	 * if the respective delta is zero
	 */
	
	if(tmpdx1!=0){
		if(tmpdx2==0)
			geom.x+=tmporiggeom.w-geom.w;
		else
			geom.x+=tmpdx1;
	}

	if(tmpdy1!=0){
		if(tmpdy2==0)
			geom.y+=tmporiggeom.h-geom.h;
		else
			geom.y+=tmpdy1;
	}
	
	region_request_geom(reg, geom, &geom, XOR_RESIZE);

	if(XOR_RESIZE)
		res_draw_rubberband(scr);
	
	res_draw_moveres(scr);
	
	tmpgeom=geom;
	
	if(XOR_RESIZE)
		res_draw_rubberband(scr);
	
	if(rret!=NULL)
		*rret=tmpgeom;
}


void end_resize(WRegion *reg)
{
	WScreen *scr=SCREEN_OF(reg);
	
	assert(tmpreg==reg);

	resize_mode=FALSE;
	tmpreg=NULL;
	
	reset_timer(&resize_timer);
	
	if(XOR_RESIZE){
		res_draw_rubberband(scr);
		region_request_geom(reg, tmpgeom, &tmpgeom, FALSE);
		XUngrabServer(wglobal.dpy);
	}
	
	XUnmapWindow(wglobal.dpy, scr->grdata.moveres_win);

	if(resize_atexit){
		resize_atexit();
		resize_atexit=NULL;
	}
}


void cancel_resize(WRegion *reg)
{
	WScreen *scr=SCREEN_OF(reg);

	assert(tmpreg==reg);
	
	resize_mode=FALSE;
	tmpreg=NULL;
	
	reset_timer(&resize_timer);
	
	if(XOR_RESIZE){
		res_draw_rubberband(scr);
		XUngrabServer(wglobal.dpy);
	}
	
	XUnmapWindow(wglobal.dpy, scr->grdata.moveres_win);

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
	WRegion *par=FIND_PARENT1(reg, WRegion);
	
	if(par==NULL){
		if(geomret!=NULL)
			*geomret=REGION_GEOM(reg);
		return;
	}
		
	region_request_sub_geom(par, reg, geom, geomret, tryonly);
	
	if(!tryonly && geomret!=NULL)
		*geomret=REGION_GEOM(reg);
}


/*}}}*/


/*{{{ Misc. */


#define REG_MIN_WH(REG, WHL)                            \
	XSizeHints hints;                                   \
	uint relwidth, relheight;                                    \
	region_resize_hints(reg, &hints, &relwidth, &relheight);     \
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


/*}}}*/



/*{{{ set_width etc. */


void set_width(WRegion *reg, uint w)
{
	WRectangle geom=REGION_GEOM(reg);
	geom.w=w;
	region_request_geom(reg, geom, &geom, FALSE);
}


void set_height(WRegion *reg, uint h)
{
	WRectangle geom=REGION_GEOM(reg);
	geom.h=h;
	region_request_geom(reg, geom, &geom, FALSE);
}


void set_widthq(WRegion *reg, double q)
{
	WScreen *scr=SCREEN_OF(reg);
	set_width(reg, q*(double)REGION_GEOM(scr).w);
}


void set_heightq(WRegion *reg, double q)
{
	WScreen *scr=SCREEN_OF(reg);
	set_height(reg, q*(double)REGION_GEOM(scr).h);
}


/*}}}*/

