/*
 * ion/ionws/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/genframep.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/objp.h>
#include <ioncore/drawp.h>
#include <ioncore/resize.h>
#include <ioncore/extl.h>

#include "ionframe.h"
#include "bindmaps.h"
#include "splitframe.h"


static void set_tab_spacing(WIonFrame *frame);

	
/*{{{ Destroy/create frame */


static bool ionframe_init(WIonFrame *frame, WWindow *parent, WRectangle geom,
						  int flags)
{
	if(!genframe_init((WGenFrame*)frame, parent, geom))
		return FALSE;
	
	flags&=~WGENFRAME_TAB_HIDE;
	frame->genframe.flags|=flags;
	set_tab_spacing(frame);
	
	region_add_bindmap((WRegion*)frame, &(ionframe_bindmap));
	
	return TRUE;
}


WIonFrame *create_ionframe(WWindow *parent, WRectangle geom, int flags)
{
	CREATEOBJ_IMPL(WIonFrame, ionframe, (p, parent, geom, flags));
}


WIonFrame* create_ionframe_simple(WWindow *parent, WRectangle geom)
{
	return create_ionframe(parent, geom, 0);
}


static void ionframe_deinit(WIonFrame *frame)
{
	genframe_deinit((WGenFrame*)frame);
}


/*}}}*/
						  

/*{{{ Geometry */


static WBorder frame_border(WGRData *grdata)
{
	WBorder bd=grdata->frame_border;
	bd.ipad/=2;
	return bd;
}


static void ionframe_border_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);

	geom->x=0;
	geom->y=0;
	geom->w=REGION_GEOM(frame).w;
	geom->h=REGION_GEOM(frame).h;
	
	if(!grdata->bar_inside_frame && !(frame->genframe.flags&WGENFRAME_TAB_HIDE)){
		WBorder bd=frame_border(grdata);
		geom->y+=grdata->bar_h+bd.ipad;
		geom->h-=grdata->bar_h+bd.ipad;
	}
}


static void ionframe_bar_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	if(grdata->bar_inside_frame){
		ionframe_border_geom(frame, geom);
		geom->x+=BORDER_TL_TOTAL(&bd)+grdata->spacing;
		geom->y+=BORDER_TL_TOTAL(&bd)+grdata->spacing;
		geom->w-=BORDER_TOTAL(&bd)+2*grdata->spacing;
	}else{
		geom->x=bd.ipad;
		geom->y=bd.ipad;
		geom->w=REGION_GEOM(frame).w-2*bd.ipad;
	}

	geom->h=(frame->genframe.flags&WGENFRAME_TAB_HIDE ? 0 : grdata->bar_h);
}


static void ionframe_border_inner_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	ionframe_border_geom(frame, geom);

	geom->x+=BORDER_TL_TOTAL(&bd);
	geom->y+=BORDER_TL_TOTAL(&bd);
	geom->w-=BORDER_TOTAL(&bd);
	geom->h-=BORDER_TOTAL(&bd);
}


static void ionframe_managed_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	ionframe_border_inner_geom(frame, geom);
	geom->x+=grdata->spacing;
	geom->y+=grdata->spacing;
	geom->w-=2*grdata->spacing;
	geom->h-=2*grdata->spacing;
	
	if(grdata->bar_inside_frame && !(frame->genframe.flags&WGENFRAME_TAB_HIDE)){
		geom->y+=grdata->bar_h+grdata->spacing;
		geom->h-=grdata->bar_h+grdata->spacing;
	}
}


static void set_tab_spacing(WIonFrame *frame)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	if(!grdata->bar_inside_frame)
		frame->genframe.tab_spacing=bd.ipad;
	else
		frame->genframe.tab_spacing=grdata->spacing;
}


static void ionframe_resize_hints(WIonFrame *frame, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	genframe_resize_hints((WGenFrame*)frame, hints_ret, relw_ret, relh_ret);
	
	if(!(hints_ret->flags&PResizeInc)){
		hints_ret->flags|=PResizeInc;
		hints_ret->width_inc=grdata->w_unit;
		hints_ret->height_inc=grdata->h_unit;
	}
	
	hints_ret->flags|=PMinSize;
	hints_ret->min_width=1;
	hints_ret->min_height=0;
}


/*EXTL_DOC
 * Toggle shade (only titlebar visible) mode.
 */
EXTL_EXPORT
void ionframe_toggle_shade(WIonFrame *frame)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	int h;
		
	if(!grdata->bar_inside_frame){
		h=2*bd.ipad+grdata->bar_h;
	}else{
		h=2*grdata->spacing+BORDER_TOTAL(&bd)+grdata->bar_h;
	}
		
	genframe_do_toggle_shade((WGenFrame*)frame, h);
}


/*}}}*/


/*{{{ Drawing routines and such */


static void ionframe_recalc_bar(WIonFrame *frame)
{
	WGRData *grdata=GRDATA_OF(frame);
	int bar_w, tab_w, textw, n;
	WRegion *sub;

	{
		WRectangle geom;
		genframe_bar_geom((WGenFrame*)frame, &geom);
		bar_w=geom.w;
	}
	
	if(WGENFRAME_MCOUNT(frame)!=0){
		n=0;
		FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
			tab_w=genframe_nth_tab_w((WGenFrame*)frame, n++);
			textw=BORDER_IW(&(grdata->tab_border), tab_w);
			REGION_LABEL(sub)=region_make_label(sub, textw,
												grdata->tab_font);
		}
	}
}



void ionframe_draw(const WIonFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	dinfo->win=WGENFRAME_WIN(frame);
	dinfo->draw=WGENFRAME_DRAW(frame);
	dinfo->grdata=grdata;
	dinfo->gc=grdata->gc;
	dinfo->border=&bd;
	
	ionframe_border_geom(frame, &(dinfo->geom));
	
	if(REGION_IS_ACTIVE(frame))
		dinfo->colors=&(grdata->act_frame_colors);
	else
		dinfo->colors=&(grdata->frame_colors);
	
	/*if(complete)
		XClearWindow(wglobal.dpy, WGENFRAME_WIN(frame));*/
	
	draw_border_inverted(dinfo, TRUE);

	genframe_draw_bar((WGenFrame*)frame, complete);
}


void ionframe_draw_bar(const WIonFrame *frame, bool complete)
{
	WGRData *grdata=GRDATA_OF(frame);

	if(frame->genframe.flags&WGENFRAME_TAB_HIDE)
		return;
	
	if(complete){
		if(!grdata->bar_inside_frame){
			WBorder bd=frame_border(grdata);
			
			if(REGION_IS_ACTIVE(frame)){
				set_foreground(wglobal.dpy, grdata->tab_gc,
							   grdata->act_frame_colors.bg);
			}else{
				set_foreground(wglobal.dpy, grdata->tab_gc,
							   grdata->frame_colors.bg);
			}
			
			XFillRectangle(wglobal.dpy, WGENFRAME_WIN(frame), grdata->tab_gc,
						   0, 0, REGION_GEOM(frame).w, grdata->bar_h+bd.ipad);
		}else{
			WRectangle geom;
			genframe_bar_geom((WGenFrame*)frame, &geom);
		
			XClearArea(wglobal.dpy, WGENFRAME_WIN(frame), 
					   geom.x, geom.y, geom.w, geom.h, False);
		}
	}
	
	genframe_draw_bar_default((WGenFrame*)frame, FALSE);
}


void ionframe_draw_config_updated(WIonFrame *frame)
{
	set_tab_spacing(frame);
	genframe_draw_config_updated((WGenFrame*)frame);
}


/*}}}*/


/*{{{ Save/load */


static bool ionframe_save_to_file(WIonFrame *frame, FILE *file, int lvl)
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


WRegion *ionframe_load(WWindow *par, WRectangle geom, ExtlTab tab)
{
	int flags=0;
	ExtlTab substab, subtab;
	WIonFrame *frame;
	int n, i;
	
	extl_table_gets_i(tab, "flags", &flags);
	
	frame=create_ionframe(par, geom, flags);
	
	if(frame==NULL)
		return NULL;

	if(!extl_table_gets_t(tab, "subs", &substab))
		return (WRegion*)frame;
	
	n=extl_table_get_n(substab);
	for(i=1; i<=n; i++){
		if(extl_table_geti_t(substab, i, &subtab)){
			region_add_managed_load((WRegion*)frame, subtab);
			extl_unref_table(subtab);
		}
	}
	extl_unref_table(substab);
	
	return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionframe_dynfuntab[]={
	{draw_window, ionframe_draw},
	{genframe_draw_bar, ionframe_draw_bar},
	{genframe_recalc_bar, ionframe_recalc_bar},
	
	{mplex_managed_geom, ionframe_managed_geom},
	{genframe_bar_geom, ionframe_bar_geom},
	{genframe_border_inner_geom, ionframe_border_inner_geom},
	{region_resize_hints, ionframe_resize_hints},

	{region_draw_config_updated, ionframe_draw_config_updated},
		
	{(DynFun*)region_save_to_file, (DynFun*)ionframe_save_to_file},
	
	{region_close, ionframe_close},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WIonFrame, WGenFrame, ionframe_deinit, ionframe_dynfuntab);

	
/*}}}*/
