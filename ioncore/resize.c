/* 
 * ion/ioncore/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>

#include "common.h"
#include "global.h"
#include "resize.h"
#include "gr.h"
#include "sizehint.h"
#include "event.h"
#include "cursor.h"
#include "objp.h"
#include "extl.h"
#include "extlconv.h"
#include "grab.h"
#include "framep.h"
#include "infowin.h"
#include "defer.h"


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
static bool snap_enabled=FALSE;
static WRectangle tmp_snapgeom;
static int tmprqflags=0;
static WInfoWin *moveres_infowin=NULL;


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


/*{{{ Size/position display and rubberband */


static void draw_rubberbox(WRootWin *rw, const WRectangle *rect)
{
	XPoint fpts[5];
	
	fpts[0].x=rect->x;
	fpts[0].y=rect->y;
	fpts[1].x=rect->x+rect->w;
	fpts[1].y=rect->y;
	fpts[2].x=rect->x+rect->w;
	fpts[2].y=rect->y+rect->h;
	fpts[3].x=rect->x;
	fpts[3].y=rect->y+rect->h;
	fpts[4].x=rect->x;
	fpts[4].y=rect->y;
	
	XDrawLines(wglobal.dpy, WROOTWIN_ROOT(rw), rw->xor_gc, fpts, 5, 
			   CoordModeOrigin);
}


static int max_width(GrBrush *brush, const char *str)
{
	int maxw=0, w;
	
	while(str && *str!='\0'){
		w=grbrush_get_text_width(brush, str, 1);
		if(w>maxw)
			maxw=w;
		str++;
	}
	
	return maxw;
}


static int chars_for_num(int d)
{
	int n=0;
	
	do{
		n++;
		d/=10;
	}while(d);
	
	return n;
}


static void setup_moveres_display(WWindow *parent, int cx, int cy)
{
	GrBorderWidths bdw;
	GrFontExtents fnte;
	WRectangle g;
	
	g.x=0;
	g.y=0;
	g.w=1;
	g.h=1;
	
	assert(moveres_infowin==NULL);
	
	moveres_infowin=create_infowin(parent, &g, "moveres_display");
	
	if(moveres_infowin==NULL)
		return;

	grbrush_get_border_widths(WINFOWIN_BRUSH(moveres_infowin), &bdw);
	grbrush_get_font_extents(WINFOWIN_BRUSH(moveres_infowin), &fnte);
	
	/* Create move/resize position/size display window */
	g.w=3;
	g.w+=chars_for_num(REGION_GEOM(parent).w);
	g.w+=chars_for_num(REGION_GEOM(parent).h);
	g.w*=max_width(WINFOWIN_BRUSH(moveres_infowin), "0123456789x+"); 	
	g.w+=bdw.left+bdw.right;
	g.h=fnte.max_height+bdw.top+bdw.bottom;;

	g.x=cx-g.w/2;
	g.y=cy-g.h/2;
	
	region_fit((WRegion*)moveres_infowin, &g);
	region_map((WRegion*)moveres_infowin);
}


static void res_draw_moveres()
{
	WRectangle geom;
	char *buf;
		
	if(moveres_infowin==NULL)
		return;
	
	buf=WINFOWIN_BUFFER(moveres_infowin);
	
	if(buf==NULL)
		return;
	
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
		
		snprintf(buf, WINFOWIN_BUFFER_LEN, "%dx%d", w, h);
	}else{
		snprintf(buf, WINFOWIN_BUFFER_LEN, "%+d %+d", tmpgeom.x, tmpgeom.y);
	}
	
	window_draw((WWindow*)moveres_infowin, TRUE);
}


static void res_draw_rubberband(WRootWin *rootwin)
{
	WRectangle rgeom=tmpgeom;
	int rx, ry;
	
	rgeom.x+=parent_rx;
	rgeom.y+=parent_ry;

	if(tmprubfn==NULL)
		draw_rubberbox(rootwin, &rgeom);
	else
		tmprubfn(rootwin, &rgeom);
}


/*}}}*/


/*{{{ Keyboard resize accelerator */


static struct timeval last_action_tv={-1, 0};
static struct timeval last_update_tv={-1, 0};
static int last_mode=0;
static double accel=1, accelinc=30, accelmax=100*100;
static long actmax=200, uptmin=50;

static void accel_reset()
{
	last_mode=0;
	accel=1.0;
	last_action_tv.tv_sec=-1;
	last_action_tv.tv_usec=-1;
}


/*EXTL_DOC
 * Set keyboard resize acceleration parameters. When a keyboard resize
 * function is called, and at most \var{t_max} milliseconds has passed
 * from a previous call, acceleration factor is reset to 1.0. Otherwise,
 * if at least \var{t_min} milliseconds have passed from the from previous
 * acceleration update or reset the squere root of the acceleration factor
 * is incremented by \var{step}. The maximum acceleration factor (pixels/call 
 * modulo size hints) is given by \var{maxacc}. The default values are 
 * (200, 50, 30, 100). 
 * 
 * Notice the interplay with X keyboard acceleration parameters.
 * (Maybe insteed of \var{t_min} we should use a minimum number of
 * calls to the function/key presses between updated? Or maybe the
 * resize should be completely time-based with key presses triggering
 * the changes?)
 */
EXTL_EXPORT
void set_resize_accel_params(int t_max, int t_min, double step, double maxacc)
{
	actmax=(t_max>0 ? t_max : INT_MAX);
	uptmin=(t_min>0 ? t_min : INT_MAX);
	accelinc=(step>0 ? step : 1);
	accelmax=(maxacc>0 ? maxacc*maxacc : 1);
}


static int sign(int x)
{
	return (x>0 ? 1 : (x<0 ? -1 : 0));
}


static long tvdiffmsec(struct timeval *tv1, struct timeval *tv2)
{
	double t1=1000*(double)tv1->tv_sec+(double)tv1->tv_usec/1000;
	double t2=1000*(double)tv2->tv_sec+(double)tv2->tv_usec/1000;
	
	return (int)(t1-t2);
}

#define SIGN_NZ(X) ((X) < 0 ? -1 : 1)

static double max(double a, double b)
{
	return (a<b ? b : a);
}

void resize_accel(int *wu, int *hu, int mode)
{
	struct timeval tv;
	long adiff, udiff;
	
	gettimeofday(&tv, NULL);
	
	adiff=tvdiffmsec(&tv, &last_action_tv);
	udiff=tvdiffmsec(&tv, &last_update_tv);
	
	if(last_mode==mode && adiff<actmax){
		if(udiff>uptmin){
			accel+=accelinc;
			if(accel>accelmax)
				accel=accelmax;
			last_update_tv=tv;
		}
	}else{
		accel=1.0;
		last_update_tv=tv;
	}
	
	last_mode=mode;
	last_action_tv=tv;
	
	/*
	if(*wu!=0)
		*wu=SIGN_NZ(*wu)*ceil(max(sqrt(accel), abs(*wu)));
	if(*hu!=0)
		*hu=SIGN_NZ(*hu)*ceil(max(sqrt(accel), abs(*hu)));
	 */
	if(*wu!=0)
		*wu=(*wu)*ceil(sqrt(accel)/abs(*wu));
	if(*hu!=0)
		*hu=(*hu)*ceil(sqrt(accel)/abs(*hu));
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
	WFrame *frame;
	
	if(!WOBJ_IS(reg, WFrame))
		return;
	
	frame=(WFrame*)reg;
	
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
			WRectangle g2=tmpgeom;
			region_request_geom(reg, tmprqflags&~REGION_RQGEOM_TRYONLY,
								&g2, &tmpgeom);
		}
		XUngrabServer(wglobal.dpy);
	}
	if(dors)
		set_saved(reg);
	
	if(moveres_infowin!=NULL){
		defer_destroy((WObj*)moveres_infowin);
		moveres_infowin=NULL;
	}
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
	WWindow *parent;
	WRegion *mgr;
	
	if(tmpreg!=NULL)
		return FALSE;

	snap_enabled=FALSE;
	
	region_resize_hints(reg, &tmphints, &tmprelw, &tmprelh);

	parent=REGION_PARENT_CHK(reg, WWindow);
	
	if(parent==NULL)
		return FALSE;
	
	accel_reset();
	
	region_rootpos((WRegion*)parent, &parent_rx, &parent_ry);
	
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

	/* Get snapping geometry */
	mgr=REGION_MANAGER(reg);
		
	if(mgr!=NULL){
		tmp_snapgeom=REGION_GEOM(mgr);
		
		if(mgr==(WRegion*)parent){
			tmp_snapgeom.x=0;
			tmp_snapgeom.y=0;	
			snap_enabled=FALSE;
		}else if(REGION_PARENT(mgr)==(WRegion*)parent){
			snap_enabled=TRUE;
		}
	}
	
	if(!tmphints.flags&PMinSize || tmphints.min_width<1)
		tmphints.min_width=1;
	if(!tmphints.flags&PMinSize || tmphints.min_height<1)
		tmphints.min_height=1;
	
	if(XOR_RESIZE)
		XGrabServer(wglobal.dpy);
	
	setup_moveres_display(parent, 
						  parent_rx+tmpgeom.x+tmpgeom.w/2,
						  parent_ry+tmpgeom.y+tmpgeom.h/2);
	
	res_draw_moveres();
	
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
	int realdx1, realdx2, realdy1, realdy2;
	
	if(tmpreg!=reg || reg==NULL)
		return;
	
	moveres_mode=MOVERES_SIZE;
	
	realdx1=(tmpdx1+=dx1);
	realdx2=(tmpdx2+=dx2);
	realdy1=(tmpdy1+=dy1);
	realdy2=(tmpdy2+=dy2);
	geom=tmporiggeom;

	/* snap */
	if(snap_enabled){
		WRectangle *sg=&tmp_snapgeom;
		int er=CF_EDGE_RESISTANCE;

		if(tmpdx1!=0 && geom.x+tmpdx1<sg->x && geom.x+tmpdx1>sg->x-er)
			realdx1=sg->x-geom.x;
		if(tmpdx2!=0 && geom.x+geom.w+tmpdx2>sg->x+sg->w && geom.x+geom.w+tmpdx2<sg->x+sg->w+er)
			realdx2=sg->x+sg->w-geom.x-geom.w;
		if(tmpdy1!=0 && geom.y+tmpdy1<sg->y && geom.y+tmpdy1>sg->y-er)
			realdy1=sg->y-geom.y;
		if(tmpdy2!=0 && geom.y+geom.h+tmpdy2>sg->y+sg->h && geom.y+geom.h+tmpdy2<sg->y+sg->h+er)
			realdy2=sg->y+sg->h-geom.y-geom.h;
	}
	
	w=tmprelw-realdx1+realdx2;
	h=tmprelh-realdy1+realdy2;
	
	if(w<=0)
		w=tmphints.min_width;
	if(h<=0)
		h=tmphints.min_height;
	
	correct_size(&w, &h, &tmphints, TRUE);
	
	/* Do not modify coordinates and sizes that were not requested to be
	 * changed. 
	 */
	
	if(tmpdx1==tmpdx2){
		if(tmpdx1==0 || realdx1!=tmpdx1)
			geom.x+=realdx1;
		else
			geom.x+=realdx2;
	}else{
		geom.w=geom.w-tmprelw+w;
		if(tmpdx1==0 || realdx1!=tmpdx1)
			geom.x+=realdx1;
		else
			geom.x+=tmporiggeom.w-geom.w;
	}

	
	if(tmpdy1==tmpdy2){
		if(tmpdy1==0 || realdy1!=tmpdy1)
			geom.y+=realdy1;
		else
			geom.y+=realdy2;
	}else{
		geom.h=geom.h-tmprelh+h;
		if(tmpdy1==0 || realdy1!=tmpdy1)
			geom.y+=realdy1;
		else
			geom.y+=tmporiggeom.h-geom.h;
	}
	
	if(XOR_RESIZE)
		res_draw_rubberband(rootwin);
	
	region_request_geom(reg, tmprqflags, &geom, &tmpgeom);
	
	if(!resize_cumulative){
		tmpdx1=0;
		tmpdx2=0;
		tmpdy1=0;
		tmpdy2=0;
		tmprelw=(tmpgeom.w-tmporiggeom.w)+tmprelw;
		tmprelh=(tmpgeom.h-tmporiggeom.h)+tmprelh;
		tmporiggeom=tmpgeom;
	}
	
	res_draw_moveres();
	
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


void region_request_geom(WRegion *reg, int flags, const WRectangle *geom,
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
EXTL_EXPORT_MEMBER_AS(WRegion, request_geom)
ExtlTab region_request_geom_extl(WRegion *reg, ExtlTab g)
{
	WRectangle geom=REGION_GEOM(reg);
	WRectangle ogeom=REGION_GEOM(reg);
	int flags=REGION_RQGEOM_WEAK_ALL;
	
	if(extl_table_gets_i(g, "x", &(geom.x)))
	   flags&=~REGION_RQGEOM_WEAK_X;
	if(extl_table_gets_i(g, "y", &(geom.y)))
	   flags&=~REGION_RQGEOM_WEAK_Y;
	if(extl_table_gets_i(g, "w", &(geom.w)))
	   flags&=~REGION_RQGEOM_WEAK_W;
	if(extl_table_gets_i(g, "h", &(geom.h)))
	   flags&=~REGION_RQGEOM_WEAK_H;
	
	region_request_geom(reg, flags, &geom, &ogeom);
	
	return geom_to_extltab(&ogeom);
}


void region_request_managed_geom(WRegion *mgr, WRegion *reg,
								 int flags, const WRectangle *geom,
								 WRectangle *geomret)
{
	CALL_DYN(region_request_managed_geom, mgr,
			 (mgr, reg, flags, geom, geomret));
}


void region_request_clientwin_geom(WRegion *mgr, WClientWin *cwin,
								   int flags, const WRectangle *geom)
{
	CALL_DYN(region_request_clientwin_geom, mgr, (mgr, cwin, flags, geom));
}


void region_request_managed_geom_allow(WRegion *mgr, WRegion *reg,
									   int flags, const WRectangle *geom,
									   WRectangle *geomret)
{
	if(geomret!=NULL)
		*geomret=*geom;
	
	if(!(flags&REGION_RQGEOM_TRYONLY))
		region_fit(reg, geom);
}


void region_request_managed_geom_unallow(WRegion *mgr, WRegion *reg,
										 int flags, const WRectangle *geom,
										 WRectangle *geomret)
{
	if(geomret!=NULL)
		*geomret=REGION_GEOM(reg);
}


/*}}}*/


/*{{{ Restore size, maximize, shade */


void frame_restore_size(WFrame *frame, bool horiz, bool vert)
{
	WRectangle geom;
	int rqf=REGION_RQGEOM_WEAK_ALL;

	geom=REGION_GEOM(frame);
	
	if(vert && frame->flags&WFRAME_SAVED_VERT){
		geom.y=frame->saved_y;
		geom.h=frame->saved_h;
		rqf&=~(REGION_RQGEOM_WEAK_Y|REGION_RQGEOM_WEAK_H);
	}

	if(horiz && frame->flags&WFRAME_SAVED_HORIZ){
		geom.x=frame->saved_x;
		geom.w=frame->saved_w;
		rqf&=~(REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_W);
	}
	
	if((rqf&REGION_RQGEOM_WEAK_ALL)!=REGION_RQGEOM_WEAK_ALL)
		region_request_geom((WRegion*)frame, rqf, &geom, NULL);
}


static void correct_frame_size(WFrame *frame, int *w, int *h)
{
	XSizeHints hints;
	uint relw, relh;
	int wdiff, hdiff;
	
	region_resize_hints((WRegion*)frame, &hints, &relw, &relh);
	
	wdiff=REGION_GEOM(frame).w-relw;
	hdiff=REGION_GEOM(frame).h-relh;
	*w-=wdiff;
	*h-=hdiff;
	correct_size(w, h, &hints, TRUE);
	*w+=wdiff;
	*h+=hdiff;
}


static bool trymaxv(WFrame *frame, WRegion *mgr, int tryonlyflag)
{
	WRectangle geom=REGION_GEOM(frame), rgeom;
	geom.y=0;
	geom.h=REGION_GEOM(mgr).h;
	
	{
		int dummy_w=geom.w;
		correct_frame_size(frame, &dummy_w, &(geom.h));
	}
	
	region_request_geom((WRegion*)frame, 
						tryonlyflag|REGION_RQGEOM_VERT_ONLY, 
						&geom, &rgeom);
	return (abs(rgeom.y-REGION_GEOM(frame).y)>1 ||
			abs(rgeom.h-REGION_GEOM(frame).h)>1);
}


/*EXTL_DOC
 * Attempt to maximize \var{frame} vertically.
 */
EXTL_EXPORT_MEMBER
void frame_maximize_vert(WFrame *frame)
{
	WRegion *mgr=REGION_MANAGER(frame);
	
	if(frame->flags&WFRAME_SHADED){
		frame_do_toggle_shade(frame, 0 /* not used */);
		return;
	}
		
	if(mgr==NULL)
		return;

	if(!trymaxv(frame, mgr, REGION_RQGEOM_TRYONLY)){
		/* Could not maximize further, restore */
		frame_restore_size(frame, FALSE, TRUE);
		return;
	}

	trymaxv(frame, mgr, 0);
}


static bool trymaxh(WFrame *frame, WRegion *mgr, int tryonlyflag)
{
	WRectangle geom=REGION_GEOM(frame), rgeom;
	geom.x=0;
	geom.w=REGION_GEOM(mgr).w;
	
	{
		int dummy_h=geom.h;
		correct_frame_size(frame, &(geom.w), &dummy_h);
	}
	
	region_request_geom((WRegion*)frame, 
						tryonlyflag|REGION_RQGEOM_HORIZ_ONLY, 
						&geom, &rgeom);
	return (abs(rgeom.x-REGION_GEOM(frame).x)>1 ||
			abs(rgeom.w-REGION_GEOM(frame).w)>1);
}
				   
/*EXTL_DOC
 * Attempt to maximize \var{frame} horizontally.
 */
EXTL_EXPORT_MEMBER
void frame_maximize_horiz(WFrame *frame)
{
	WRegion *mgr=REGION_MANAGER(frame);
	
	if(mgr==NULL)
		return;

	if(!trymaxh(frame, mgr, REGION_RQGEOM_TRYONLY)){
		/* Could not maximize further, restore */
		frame_restore_size(frame, TRUE, FALSE);
		return;
	}
	
	trymaxh(frame, mgr, 0);
}


void frame_do_toggle_shade(WFrame *frame, int shaded_h)
{
	WRectangle geom=REGION_GEOM(frame);

	if(frame->flags&WFRAME_SHADED){
		if(!(frame->flags&WFRAME_SAVED_VERT))
			return;
		geom.h=frame->saved_h;
	}else{
		if(frame->flags&WFRAME_TAB_HIDE)
			return;
		geom.h=shaded_h;
	}
	
	region_request_geom((WRegion*)frame, REGION_RQGEOM_H_ONLY,
						&geom, NULL);
}


/*EXTL_DOC
 * Is \var{frame} shaded?
 */
EXTL_EXPORT_MEMBER
bool frame_is_shaded(WFrame *frame)
{
	return ((frame->flags&WFRAME_SHADED)!=0);
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


void frame_resize_units(WFrame *frame, int *wret, int *hret)
{
	/*WGRData *grdata=GRDATA_OF(frame);
	*wret=grdata->w_unit;
	*hret=grdata->h_unit;*/
	*wret=1;
	*hret=1;
	
	if(WFRAME_CURRENT(frame)!=NULL){
		XSizeHints hints;
		
		region_resize_hints(WFRAME_CURRENT(frame), &hints, NULL, NULL);
		
		if(hints.flags&PResizeInc &&
		   (hints.width_inc>1 || hints.height_inc>1)){
			*wret=hints.width_inc;
			*hret=hints.height_inc;
		}
	}
}


void region_convert_root_geom(WRegion *reg, WRectangle *geom)
{
	int rx, ry;
	if(reg!=NULL){
		region_rootpos(reg, &rx, &ry);
		geom->x-=rx;
		geom->y-=ry;
	}
}

/*}}}*/

