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
#include <ioncore/gr.h>
#include <ioncore/resize.h>
#include <ioncore/extl.h>
#include <ioncore/strings.h>

#include "ionframe.h"
#include "bindmaps.h"
#include "splitframe.h"


#define BAR_INSIDE_BORDER(FRAME) ((FRAME)->bar_inside_border)
#define BAR_OFF(FRAME) (0)


/*{{{ Destroy/create frame */


static bool ionframe_init(WIonFrame *frame, WWindow *parent, 
						  const WRectangle *geom)
{
	frame->bar_inside_border=TRUE;
	
	if(!genframe_init((WGenFrame*)frame, parent, geom))
		return FALSE;
	
	region_add_bindmap((WRegion*)frame, &(ionframe_bindmap));
	
	return TRUE;
}


WIonFrame *create_ionframe(WWindow *parent, const WRectangle *geom)
{
	CREATEOBJ_IMPL(WIonFrame, ionframe, (p, parent, geom));
}


static void ionframe_deinit(WIonFrame *frame)
{
	genframe_deinit((WGenFrame*)frame);
}


/*}}}*/
						  

/*{{{ Geometry */


static uint get_spacing(const WIonFrame *frame)
{
	GrBorderWidths bdw;
	
	if(frame->genframe.brush==NULL)
		return 0;
	
	grbrush_get_border_widths(frame->genframe.brush, &bdw);
	
	return bdw.spacing;
}


static void ionframe_border_geom(const WIonFrame *frame, WRectangle *geom)
{
	geom->x=0;
	geom->y=0;
	geom->w=REGION_GEOM(frame).w;
	geom->h=REGION_GEOM(frame).h;
	
	if(!BAR_INSIDE_BORDER(frame) && 
	   !(frame->genframe.flags&WGENFRAME_TAB_HIDE) &&
	   frame->genframe.brush!=NULL){
		geom->y+=frame->genframe.bar_h+BAR_OFF(frame);
		geom->h-=frame->genframe.bar_h+2*BAR_OFF(frame);
	}
}


static void ionframe_border_inner_geom(const WIonFrame *frame, 
									   WRectangle *geom)
{
	GrBorderWidths bdw;
	
	genframe_border_geom((const WGenFrame*)frame, geom);

	if(frame->genframe.brush!=NULL){
		grbrush_get_border_widths(frame->genframe.brush, &bdw);

		geom->x+=bdw.left;
		geom->y+=bdw.top;
		geom->w-=bdw.left+bdw.right;
		geom->h-=bdw.top+bdw.bottom;
	}
}


static void ionframe_bar_geom(const WIonFrame *frame, WRectangle *geom)
{
	uint off;
	
	if(BAR_INSIDE_BORDER(frame)){
		off=get_spacing(frame);
		genframe_border_inner_geom((const WGenFrame*)frame, geom);
	}else{
		off=BAR_OFF(frame);
		geom->x=0;
		geom->y=0;
		geom->w=REGION_GEOM(frame).w;
	}
	geom->x+=off;
	geom->y+=off;
	geom->w-=2*off;

	geom->h=(frame->genframe.flags&WGENFRAME_TAB_HIDE 
			 ? 0 : frame->genframe.bar_h);
}


static void ionframe_managed_geom(const WIonFrame *frame, WRectangle *geom)
{
	uint spacing=get_spacing(frame);
	
	genframe_border_inner_geom((const WGenFrame*)frame, geom);
	
	geom->x+=spacing;
	geom->y+=spacing;
	geom->w-=2*spacing;
	geom->h-=2*spacing;
	
	if(BAR_INSIDE_BORDER(frame) && !(frame->genframe.flags&WGENFRAME_TAB_HIDE)){
		geom->y+=frame->genframe.bar_h+spacing;
		geom->h-=frame->genframe.bar_h+spacing;
	}
}


static void ionframe_resize_hints(WIonFrame *frame, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret)
{
	genframe_resize_hints((WGenFrame*)frame, hints_ret, relw_ret, relh_ret);
	
	if(!(hints_ret->flags&PResizeInc)){
		/*hints_ret->flags|=PResizeInc;
		hints_ret->width_inc=grdata->w_unit;
		hints_ret->height_inc=grdata->h_unit;*/
	}
	
	hints_ret->flags|=PMinSize;
	hints_ret->min_width=1;
	hints_ret->min_height=0;
}


/*EXTL_DOC
 * Toggle shade (only titlebar visible) mode.
 */
EXTL_EXPORT_MEMBER
void ionframe_toggle_shade(WIonFrame *frame)
{
	GrBorderWidths bdw;
	int h=frame->genframe.bar_h;

	if(BAR_INSIDE_BORDER(frame) && frame->genframe.brush!=NULL){
		grbrush_get_border_widths(frame->genframe.brush, &bdw);
		h+=bdw.top+bdw.bottom+2*bdw.spacing;
	}else{
		h+=2*BAR_OFF(frame);
	}

	genframe_do_toggle_shade((WGenFrame*)frame, h);
}


/*}}}*/


/*{{{ Drawing-related routines */


static int init_title(WIonFrame *frame, int i)
{
	int textw;
	
	if(frame->genframe.titles[i].text!=NULL){
		free(frame->genframe.titles[i].text);
		frame->genframe.titles[i].text=NULL;
	}
	
	textw=genframe_nth_tab_iw((WGenFrame*)frame, i);
	frame->genframe.titles[i].iw=textw;
	return textw;
}


static void ionframe_recalc_bar(WIonFrame *frame)
{
	int textw, i;
	WRegion *sub;
	char *title;

	if(frame->genframe.bar_brush==NULL ||
	   frame->genframe.titles==NULL){
		return;
	}
	
	i=0;
	
	if(WGENFRAME_MCOUNT(frame)==0){
		textw=init_title(frame, i);
		if(textw>0){
			title=make_label(frame->genframe.bar_brush, CF_STR_EMPTY, textw);
			frame->genframe.titles[i].text=title;
		}
		return;
	}
	
	FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
		textw=init_title(frame, i);
		if(textw>0){
			title=region_make_label(sub, textw, frame->genframe.bar_brush);
			frame->genframe.titles[i].text=title;
		}
		i++;
	}
}


static void ionframe_brushes_updated(WIonFrame *frame)
{
	ExtlTab tab;
	
	frame->bar_inside_border=TRUE;

	if(frame->genframe.brush==NULL)
		return;
	
	grbrush_get_extra_values(frame->genframe.brush, &tab);
	
	extl_table_gets_b(tab, "ionframe_bar_inside_border", 
					  &(frame->bar_inside_border));
}


static const char *ionframe_style(WIonFrame *frame)
{
	return "frame-ionframe";
}


static const char *ionframe_tab_style(WIonFrame *frame)
{
	return "tab-frame-ionframe";
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
			fprintf(file, "switchto = true,\n");
		}
		save_indent_line(file, lvl+1);
		fprintf(file, "},\n");
	}
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
	
	return TRUE;
}


WRegion *ionframe_load(WWindow *par, const WRectangle *geom, ExtlTab tab)
{
	int flags=0;
	ExtlTab substab, subtab;
	WIonFrame *frame;
	int n, i;
	
	frame=create_ionframe(par, geom);
	
	if(frame==NULL)
		return NULL;
	
	extl_table_gets_i(tab, "flags", &flags);
	
	if(flags&WGENFRAME_TAB_HIDE)
		genframe_toggle_tab((WGenFrame*)frame);

	if(!extl_table_gets_t(tab, "subs", &substab))
		return (WRegion*)frame;
	
	n=extl_table_get_n(substab);
	for(i=1; i<=n; i++){
		if(extl_table_geti_t(substab, i, &subtab)){
			mplex_attach_new((WMPlex*)frame, subtab);
			extl_unref_table(subtab);
		}
	}
	extl_unref_table(substab);
	
	return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionframe_dynfuntab[]={
	{genframe_recalc_bar, ionframe_recalc_bar},
	
	{mplex_managed_geom, ionframe_managed_geom},
	{genframe_bar_geom, ionframe_bar_geom},
	{genframe_border_inner_geom, ionframe_border_inner_geom},
	{genframe_border_geom, ionframe_border_geom},
	{region_resize_hints, ionframe_resize_hints},

	{(DynFun*)region_save_to_file, (DynFun*)ionframe_save_to_file},
	
	{region_close, ionframe_close},

	{(DynFun*)genframe_style, (DynFun*)ionframe_style},
	{(DynFun*)genframe_tab_style, (DynFun*)ionframe_tab_style},
	
	{genframe_brushes_updated, ionframe_brushes_updated},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WIonFrame, WGenFrame, ionframe_deinit, ionframe_dynfuntab);

	
/*}}}*/
