/*
 * ion/ionws/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/genframep.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/objp.h>
#include <ioncore/drawp.h>
#include <ioncore/resize.h>
#include <ioncore/targetid.h>
#include <ioncore/extl.h>

#include "ionframe.h"
#include "bindmaps.h"
#include "splitframe.h"


static void set_tab_spacing(WIonFrame *frame);

	
/*{{{ Destroy/create frame */


static bool ionframe_init(WIonFrame *frame, WWindow *parent, WRectangle geom,
						  int id, int flags)
{
	if(!genframe_init((WGenFrame*)frame, parent, geom, id))
		return FALSE;
	
	frame->genframe.flags|=flags;
	set_tab_spacing(frame);
	
	region_add_bindmap((WRegion*)frame, &(ionframe_bindmap));
	
	return TRUE;
}


WIonFrame *create_ionframe(WWindow *parent, WRectangle geom, int id, int flags)
{
	CREATEOBJ_IMPL(WIonFrame, ionframe, (p, parent, geom, id, flags));
}


WIonFrame* create_ionframe_simple(WWindow *parent, WRectangle geom)
{
	return create_ionframe(parent, geom, 0, 0);
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
	WScreen *scr=SCREEN_OF(frame);
	
	genframe_resize_hints((WGenFrame*)frame, hints_ret, relw_ret, relh_ret);
	
	if(!(hints_ret->flags&PResizeInc)){
		hints_ret->flags|=PResizeInc;
		hints_ret->width_inc=scr->w_unit;
		hints_ret->height_inc=scr->h_unit;
	}
	
	hints_ret->flags|=PMinSize;
	hints_ret->min_width=1;
	hints_ret->min_height=1;
}


/*}}}*/


/*{{{ Drawing routines and such */


static void ionframe_recalc_bar(WIonFrame *frame, bool draw)
{
	WScreen *scr=SCREEN_OF(frame);
	int bar_w, tab_w, textw, n;
	WRegion *sub;

	{
		WRectangle geom;
		genframe_bar_geom((WGenFrame*)frame, &geom);
		bar_w=geom.w;
	}
	
	if(frame->genframe.managed_count!=0){
		n=0;
		FOR_ALL_MANAGED_ON_LIST(frame->genframe.managed_list, sub){
			tab_w=genframe_nth_tab_w((WGenFrame*)frame, n++);
			textw=BORDER_IW(&(scr->grdata.tab_border), tab_w);
			REGION_LABEL(sub)=region_make_label(sub, textw,
												scr->grdata.tab_font);
		}
	}
	
	if(draw)
		genframe_draw_bar((WGenFrame*)frame, TRUE);
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
	
	if(complete)
		XClearWindow(wglobal.dpy, WGENFRAME_WIN(frame));
	
	draw_border_inverted(dinfo, TRUE);

	genframe_draw_bar((WGenFrame*)frame,
					  !complete || !grdata->bar_inside_frame);
}


void ionframe_draw_bar(const WIonFrame *frame, bool complete)
{
	WGRData *grdata=GRDATA_OF(frame);

	if(frame->genframe.flags&WGENFRAME_TAB_HIDE)
		return;
	
	if(complete && !grdata->bar_inside_frame){
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
		complete=FALSE;
	}
	
	genframe_draw_bar_default((WGenFrame*)frame, complete);
}


void ionframe_draw_config_updated(WIonFrame *frame)
{
	set_tab_spacing(frame);
	genframe_draw_config_updated((WGenFrame*)frame);
}


/*}}}*/


/*{{{ Save/load */


static bool ionframe_save_to_file(WIonFrame *ionframe, FILE *file, int lvl)
{
	WRegion *sub;
	
	begin_saved_region((WRegion*)ionframe, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "target_id = %d,\n", ionframe->genframe.target_id);
	save_indent_line(file, lvl);
	fprintf(file, "flags = %d,\n", ionframe->genframe.flags);
	save_indent_line(file, lvl);
	fprintf(file, "subs = {\n");
	FOR_ALL_MANAGED_ON_LIST(ionframe->genframe.managed_list, sub){
		region_save_to_file((WRegion*)sub, file, lvl+1);
	}
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
	
	return TRUE;
}


WRegion *ionframe_load(WWindow *par, WRectangle geom, ExtlTab tab)
{
	int target_id=0, flags=0;
	ExtlTab substab, subtab;
	WIonFrame *frame;
	int n, i;
	
	extl_table_gets_i(tab, "target_id", &target_id);
	extl_table_gets_i(tab, "flags", &flags);
	flags&=WGENFRAME_TAB_HIDE;
	
	frame=create_ionframe(par, geom, target_id, flags);
	
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
	
	{genframe_managed_geom, ionframe_managed_geom},
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
