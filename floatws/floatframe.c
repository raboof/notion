/*
 * ion/floatws/floatframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <X11/extensions/shape.h>
#include <string.h>

#include <ioncore/genframe.h>
#include <ioncore/genframep.h>
#include <ioncore/genframe-pointer.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/objp.h>
#include <ioncore/objp.h>
#include <ioncore/drawp.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>
#include <ioncore/resize.h>
#include <ioncore/sizehint.h>
#include "floatframe.h"
#include "main.h"


	
/*{{{ Destroy/create frame */


static bool floatframe_init(WFloatFrame *frame, WWindow *parent,
							WRectangle geom)
{
	if(!genframe_init((WGenFrame*)frame, parent, geom))
		return FALSE;

	frame->bar_w=geom.w;
	frame->genframe.tab_spacing=0;
	
	genframe_recalc_bar((WGenFrame*)frame);
	
	region_add_bindmap((WRegion*)frame, &(floatframe_bindmap));
	
	return TRUE;
}


WFloatFrame *create_floatframe(WWindow *parent, WRectangle geom)
{
	CREATEOBJ_IMPL(WFloatFrame, floatframe, (p, parent, geom));
}


static void deinit_floatframe(WFloatFrame *frame)
{
	genframe_deinit((WGenFrame*)frame);
}


/*}}}*/
						  

/*{{{ Geometry */


static WRectangle floatframe_to_managed_geom(WGRData *grdata, WRectangle geom)
{
	geom.x=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad;
	geom.w-=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad);
	geom.y=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad
		+grdata->bar_h;
	geom.h-=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad)
		+grdata->bar_h;
	
	return geom;
}


WRectangle managed_to_floatframe_geom(WGRData *grdata, WRectangle geom)
{
	return initial_to_floatframe_geom(grdata, geom, StaticGravity);
	/*
	geom.x-=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad;
	geom.y-=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad
		+grdata->bar_h;
	geom.w+=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad);
	geom.h+=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad)
		+grdata->bar_h;
	return geom;*/
}


static void floatframe_border_geom(const WFloatFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);

	geom->x=0;
	geom->y=grdata->bar_h;
	geom->w=REGION_GEOM(frame).w;
	geom->h=REGION_GEOM(frame).h-grdata->bar_h;
}


static void floatframe_bar_geom(const WFloatFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	geom->x=0;
	geom->y=0;
	geom->w=frame->bar_w;
	geom->h=grdata->bar_h;
}


static void floatframe_managed_geom(const WFloatFrame *frame, WRectangle *geom)
{
	*geom=floatframe_to_managed_geom(GRDATA_OF(frame), REGION_GEOM(frame));
}


static void floatframe_border_inner_geom(const WFloatFrame *frame,
										 WRectangle *geom)
{
	*geom=floatframe_to_managed_geom(GRDATA_OF(frame), REGION_GEOM(frame));
}


WRectangle initial_to_floatframe_geom(WGRData *grdata, WRectangle geom,
									  int gravity)
{
	int top, bottom, left, right;

	if(geom.w<1)
		geom.w=1;
	if(geom.h<1)
		geom.h=1;
	
	top=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad
		+grdata->bar_h;
	bottom=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad;
	left=bottom;
	right=bottom;
	
	account_gravity(&geom, gravity, top, bottom, left, right);
	
	return geom;
}


static void floatframe_request_clientwin_geom(WFloatFrame *frame, 
											  WClientWin *cwin,
											  int flags, WRectangle geom)
{
	int gravity=NorthWestGravity;
	WRegion *parent;
	XSizeHints hints;
	
	if(cwin->size_hints.flags&PWinGravity)
		gravity=cwin->size_hints.win_gravity;
	
	genframe_resize_hints((WGenFrame*)frame, &hints, NULL, NULL);
	correct_size(&(geom.w), &(geom.h), &hints, TRUE);
	geom=initial_to_floatframe_geom(GRDATA_OF(frame), geom, gravity);
	{
		int rx, ry;
		WRegion *par=(WRegion*)frame;
		
		do{
			rx=REGION_GEOM(par).x;
			ry=REGION_GEOM(par).y;
			par=region_parent((WRegion*)par);
		}while(par!=NULL && ROOT_OF(par)!=region_x_window(par));
		
		/* Add the difference between client requested coordinates and 
		 * the coordinates of the outermost WM window to frame coordinates.
		 */
		geom.x+=REGION_GEOM(frame).x-rx;
		geom.y+=REGION_GEOM(frame).y-ry;
	}
	
	region_request_geom((WRegion*)frame, REGION_RQGEOM_NORMAL, geom, NULL);
}


/*}}}*/


/*{{{ Drawing routines and such */


static void floatframe_set_shape(WFloatFrame *frame)
{
	WRectangle g={0, 0, 0, 0};
	XRectangle r[2];
	int n=0;
	
	if(!(frame->genframe.flags&WGENFRAME_TAB_HIDE)){
		floatframe_bar_geom(frame, &g);
		r[n].x=g.x;
		r[n].y=g.y;
		r[n].width=g.w;
		r[n].height=g.h;
		n++;
	}
	if(!(frame->genframe.flags&WGENFRAME_SHADED)){
		r[n].width=REGION_GEOM(frame).w;
		r[n].height=REGION_GEOM(frame).h-g.h;
		r[n].x=0;
		r[n].y=g.h;
		n++;
	}
	XShapeCombineRectangles(wglobal.dpy, WGENFRAME_WIN(frame), ShapeBounding,
							0, 0, r, n, ShapeSet, YXBanded);
}


#define CF_TAB_MAX_TEXT_X_OFF 10

static void floatframe_recalc_bar(WFloatFrame *frame)
{
	WGRData *grdata=GRDATA_OF(frame);
	int bar_w=0, maxw=0, tmaxw=0;
	int tab_w, textw, tmp;
	WRegion *sub;
	const char *p;
	
	if(WGENFRAME_MCOUNT(frame)!=0){
		FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
			p=region_name(sub);
			if(p!=NULL){
				tab_w=(text_width(grdata->tab_font, p, strlen(p))
					   +BORDER_TL_TOTAL(&(grdata->tab_border))
					   +BORDER_BR_TOTAL(&(grdata->tab_border)));
				if(tab_w>tmaxw)
					tmaxw=tab_w;
			}
		}
		
		maxw=grdata->pwm_bar_max_width_q*REGION_GEOM(frame).w;
		if(maxw<grdata->pwm_tab_min_width &&
		   REGION_GEOM(frame).w>grdata->pwm_tab_min_width)
			maxw=grdata->pwm_tab_min_width;
		
		tmp=maxw-tmaxw*WGENFRAME_MCOUNT(frame);
		if(tmp>0){
			/* No label truncation needed. good. See how much can be padded */
			tmp/=WGENFRAME_MCOUNT(frame)*2;
			if(tmp>CF_TAB_MAX_TEXT_X_OFF)
				tmp=CF_TAB_MAX_TEXT_X_OFF;
			tab_w=tmaxw+tmp*2;
			bar_w=tab_w*WGENFRAME_MCOUNT(frame);
		}else{
			/* Some labels must be truncated */
			bar_w=maxw;
		}
	}else{
		bar_w=grdata->pwm_tab_min_width;
		if(bar_w>grdata->pwm_bar_max_width_q*REGION_GEOM(frame).w)
			bar_w=grdata->pwm_bar_max_width_q*REGION_GEOM(frame).w;
	}

	if(frame->bar_w!=bar_w){
		frame->bar_w=bar_w;
		floatframe_set_shape(frame);
	}

	if(WGENFRAME_MCOUNT(frame)!=0){
		int n=0;
		FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
			tab_w=genframe_nth_tab_w((WGenFrame*)frame, n++);
			textw=BORDER_IW(&(grdata->tab_border), tab_w);
			REGION_LABEL(sub)=region_make_label(sub, textw, grdata->tab_font);
		}
	}
}


static void floatframe_size_changed(WFloatFrame *frame, bool wchg, bool hchg)
{
	int bar_w=frame->bar_w;
	
	if(wchg)
		floatframe_recalc_bar(frame);
	if(hchg || (wchg && bar_w==frame->bar_w))
		floatframe_set_shape(frame);
}


static void floatframe_draw(const WFloatFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(frame);
	int a;

	dinfo->win=WGENFRAME_WIN(frame);
	dinfo->draw=WGENFRAME_DRAW(frame);

	dinfo->grdata=grdata;
	dinfo->gc=grdata->gc;
	dinfo->border=&(grdata->frame_border);
	floatframe_border_geom(frame, &(dinfo->geom));
	
	if(REGION_IS_ACTIVE(frame))
		dinfo->colors=&(grdata->act_frame_colors);
	else
		dinfo->colors=&(grdata->frame_colors);
	
	/*if(complete)
		XClearWindow(wglobal.dpy, WGENFRAME_WIN(frame));*/
	
	draw_border(dinfo);
	
	dinfo->geom.x+=BORDER->tl;
	dinfo->geom.y+=BORDER->tl;
	dinfo->geom.w-=BORDER->tl+BORDER->br;
	dinfo->geom.h-=BORDER->tl+BORDER->br;
	
	draw_border_inverted(dinfo, TRUE);

	genframe_draw_bar((WGenFrame*)frame, FALSE);
}


/*}}}*/


/*{{{ Add/remove */


void floatframe_remove_managed(WFloatFrame *frame, WRegion *reg)
{
	mplex_remove_managed((WMPlex*)frame, reg);
	if(WGENFRAME_MCOUNT(frame)==0)
		defer_destroy((WObj*)frame);
}


/*}}}*/


/*{{{ Actions */


/*EXTL_DOC
 * Start moving \var{frame} with the mouse (or other pointing device).
 * this should only be used by binding to the \emph{mpress} or
 * \emph{mdrag} actions.
 */
EXTL_EXPORT
void floatframe_p_move(WFloatFrame *frame)
{
	genframe_p_move((WGenFrame*)frame);
}


/*EXTL_DOC
 * Toggle shade (only titlebar visible) mode.
 */
EXTL_EXPORT
void floatframe_toggle_shade(WFloatFrame *frame)
{
	WRectangle geom;
	floatframe_bar_geom(frame, &geom);
	genframe_do_toggle_shade((WGenFrame*)frame, geom.h);
}


/*}}}*/


/*{{{ Save/load */


static bool floatframe_save_to_file(WFloatFrame *frame, FILE *file, int lvl)
{
	WRegion *sub;
	
	begin_saved_region((WRegion*)frame, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "flags = %d,\n", frame->genframe.flags);
	save_indent_line(file, lvl);
	fprintf(file, "subs = {\n");
	FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
		save_indent_line(file, lvl+1);
		fprintf(file, "{\n");
		region_save_to_file((WRegion*)sub, file, lvl+2);
		if(sub==WGENFRAME_CURRENT(frame)){
			save_indent_line(file, lvl+2);
			fprintf(file, "selected = true,\n");
		}
		save_indent_line(file, lvl+1);
		fprintf(file, "},\n");
	}
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
	
	return TRUE;
}


WRegion *floatframe_load(WWindow *par, WRectangle geom, ExtlTab tab)
{
	int flags=0;
	ExtlTab substab, subtab;
	WFloatFrame *frame;
	int n, i;
	
	/*extl_table_gets_i(tab, "flags", &flags);
	flags&=WGENFRAME_TAB_HIDE;*/
	
	if(!extl_table_gets_t(tab, "subs", &substab))
		return NULL;

	frame=create_floatframe(par, geom);
	
	if(frame!=NULL){
		n=extl_table_get_n(substab);
		for(i=1; i<=n; i++){
			if(extl_table_geti_t(substab, i, &subtab)){
				region_add_managed_load((WRegion*)frame, subtab);
				extl_unref_table(subtab);
			}
		}
	}
	
	extl_unref_table(substab);
	
	if(WGENFRAME_MCOUNT(frame)==0){
		/* Nothing to manage, destroy */
		destroy_obj((WObj*)frame);
		frame=NULL;
	}
	
	return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynfuntab and class implementation */


static DynFunTab floatframe_dynfuntab[]={
	{draw_window, floatframe_draw},
	{mplex_size_changed, floatframe_size_changed},
	{genframe_recalc_bar, floatframe_recalc_bar},

	{mplex_managed_geom, floatframe_managed_geom},
	{genframe_bar_geom, floatframe_bar_geom},
	{genframe_border_inner_geom, floatframe_border_inner_geom},
	{region_remove_managed, floatframe_remove_managed},
	
	/*{region_request_managed_geom, floatframe_request_managed_geom},*/

	{region_request_clientwin_geom, floatframe_request_clientwin_geom},
	
	{(DynFun*)region_save_to_file, (DynFun*)floatframe_save_to_file},
	
	END_DYNFUNTAB
};


IMPLOBJ(WFloatFrame, WGenFrame, deinit_floatframe, floatframe_dynfuntab);

		
/*}}}*/
