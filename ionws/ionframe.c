/*
 * ion/ionws/ionframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/framep.h>
#include <ioncore/frame-draw.h>
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


#define BAR_INSIDE_BORDER(FRAME) (!((FRAME)->frame.flags&WGENFRAME_BAR_OUTSIDE))
#define BAR_OFF(FRAME) (0)


/*{{{ Destroy/create frame */


static bool ionframe_init(WIonFrame *frame, WWindow *parent, 
						  const WRectangle *geom)
{
	if(!frame_init((WFrame*)frame, parent, geom))
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
	frame_deinit((WFrame*)frame);
}


/*}}}*/


/*{{{ Shade */


/*EXTL_DOC
 * Toggle shade (only titlebar visible) mode.
 */
EXTL_EXPORT_MEMBER
void ionframe_toggle_shade(WIonFrame *frame)
{
	GrBorderWidths bdw;
	int h=frame->frame.bar_h;

	if(BAR_INSIDE_BORDER(frame) && frame->frame.brush!=NULL){
		grbrush_get_border_widths(frame->frame.brush, &bdw);
		h+=bdw.top+bdw.bottom+2*bdw.spacing;
	}else{
		h+=2*BAR_OFF(frame);
	}

	frame_do_toggle_shade((WFrame*)frame, h);
}


/*}}}*/


/*{{{ Dynfun implementations */


static void ionframe_resize_hints(WIonFrame *frame, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret)
{
	frame_resize_hints((WFrame*)frame, hints_ret, relw_ret, relh_ret);
	
	if(!(hints_ret->flags&PResizeInc)){
		/*hints_ret->flags|=PResizeInc;
		hints_ret->width_inc=grdata->w_unit;
		hints_ret->height_inc=grdata->h_unit;*/
	}
	
	hints_ret->flags|=PMinSize;
	hints_ret->min_width=1;
	hints_ret->min_height=0;
}


static void ionframe_brushes_updated(WIonFrame *frame)
{
	ExtlTab tab;
	bool b=TRUE;

	if(frame->frame.brush==NULL)
		return;
	
	grbrush_get_extra_values(frame->frame.brush, &tab);
	
	extl_table_gets_b(tab, "ionframe_bar_inside_border", &b);
	
	if(b)
		frame->frame.flags&=~WGENFRAME_BAR_OUTSIDE;
	else
		frame->frame.flags|=WGENFRAME_BAR_OUTSIDE;
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


/*{{{ Load */


WRegion *ionframe_load(WWindow *par, const WRectangle *geom, ExtlTab tab)
{
	WIonFrame *frame=create_ionframe(par, geom);
	if(frame!=NULL)
		frame_do_load((WFrame*)frame, tab);
	return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionframe_dynfuntab[]={
	{region_resize_hints, ionframe_resize_hints},

	{region_close, ionframe_close},

	{(DynFun*)frame_style, (DynFun*)ionframe_style},
	{(DynFun*)frame_tab_style, (DynFun*)ionframe_tab_style},
	
	{frame_brushes_updated, ionframe_brushes_updated},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WIonFrame, WFrame, ionframe_deinit, ionframe_dynfuntab);

	
/*}}}*/
