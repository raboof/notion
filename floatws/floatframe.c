/*
 * ion/floatws/floatframe.c
n *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <string.h>

#include <ioncore/genframe.h>
#include <ioncore/genframep.h>
#include <ioncore/genframe-pointer.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/objp.h>
#include <ioncore/objp.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>
#include <ioncore/resize.h>
#include <ioncore/sizehint.h>
#include <ioncore/extlconv.h>
#include "floatframe.h"
#include "floatws.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool floatframe_init(WFloatFrame *frame, WWindow *parent,
							const WRectangle *geom)
{
	frame->bar_w=geom->w;
	frame->sticky=FALSE;
	
	if(!genframe_init((WGenFrame*)frame, parent, geom))
		return FALSE;

	region_add_bindmap((WRegion*)frame, &(floatframe_bindmap));
	
	return TRUE;
}


WFloatFrame *create_floatframe(WWindow *parent, const WRectangle *geom)
{
	CREATEOBJ_IMPL(WFloatFrame, floatframe, (p, parent, geom));
}


static void deinit_floatframe(WFloatFrame *frame)
{
	genframe_deinit((WGenFrame*)frame);
}


/*}}}*/
						  

/*{{{ Geometry */

static void floatframe_offsets(WRootWin *rootwin, const WFloatFrame *frame,
							   WRectangle *off)
{
	GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
	uint bar_h=0;
	
	if(frame!=NULL){
		if(frame->genframe.brush!=NULL)
			grbrush_get_border_widths(frame->genframe.brush, &bdw);
	}else if(rootwin!=NULL){
		gr_get_brush_values(rootwin, "frame-floatframe", &bdw, NULL, NULL);
	}
	
	off->x=-bdw.left;
	off->y=-bdw.top;
	off->w=bdw.left+bdw.right;
	off->h=bdw.top+bdw.bottom;

	if(frame!=NULL){
		bar_h=frame->genframe.bar_h;
	}else if(rootwin!=NULL){
		GrBorderWidths bdwt=GR_BORDER_WIDTHS_INIT;
		GrFontExtents fntet=GR_FONT_EXTENTS_INIT;
		gr_get_brush_values(rootwin, "frame-tab-floatframe", &bdwt, &fntet, 
							NULL);
		bar_h=bdwt.top+bdwt.bottom+fntet.max_height;
	}

	off->y-=bar_h;
	off->h+=bar_h;
}


void managed_to_floatframe_geom(WRootWin *rootwin, WRectangle *geom)
{
	WRectangle off;
	floatframe_offsets(rootwin, NULL, &off);
	geom->x+=off.x;
	geom->y+=off.y;
	geom->w+=off.w;
	geom->h+=off.h;
}


static void floatframe_to_managed_geom(WRootWin *rootwin,
									   const WFloatFrame *frame,
									   WRectangle *geom)
{
	WRectangle off;
	floatframe_offsets(rootwin, frame, &off);
	geom->x=-off.x;
	geom->y=-off.y;
	geom->w-=off.w;
	geom->h-=off.h;
}


static void floatframe_border_geom(const WFloatFrame *frame, WRectangle *geom)
{
	geom->x=0;
	geom->y=frame->genframe.bar_h;
	geom->w=REGION_GEOM(frame).w;
	geom->h=REGION_GEOM(frame).h-frame->genframe.bar_h;
}


static void floatframe_bar_geom(const WFloatFrame *frame, WRectangle *geom)
{
	geom->x=0;
	geom->y=0;
	geom->w=frame->bar_w;
	geom->h=frame->genframe.bar_h;
}


static void floatframe_managed_geom(const WFloatFrame *frame, WRectangle *geom)
{
	*geom=REGION_GEOM(frame);
	floatframe_to_managed_geom(NULL, frame, geom);
}


#define floatframe_border_inner_geom floatframe_managed_geom


void initial_to_floatframe_geom(WFloatWS *ws, WRectangle *geom, int gravity)
{
	WRectangle off;
#ifndef CF_NO_WSREL_INIT_GEOM
	/* 'app -geometry -0-0' should place the app at the lower right corner
	 * of ws even if ws is nested etc.
	 */
	int top=0, left=0, bottom=0, right=0;
	WRootWin *root;
	
	root=region_rootwin_of((WRegion*)ws);
	region_rootpos((WRegion*)ws, &left, &top);
	right=REGION_GEOM(root).w-left-REGION_GEOM(ws).w;
	bottom=REGION_GEOM(root).h-top-REGION_GEOM(ws).h;
#endif

	floatframe_offsets(ROOTWIN_OF(ws), NULL, &off);

	geom->w+=off.w;
	geom->h+=off.h;
#ifndef CF_NO_WSREL_INIT_GEOM
	geom->x+=gravity_deltax(gravity, -off.x+left, off.x+off.w+right);
	geom->y+=gravity_deltay(gravity, -off.y+top, off.y+off.h+bottom);
	geom->x+=REGION_GEOM(ws).x;
	geom->y+=REGION_GEOM(ws).y;
#else
	geom->x+=gravity_deltax(gravity, -off.x, off.x+off.w);
	geom->y+=gravity_deltay(gravity, -off.y, off.y+off.h);
	region_convert_root_geom(region_parent((WRegion*)ws), geom);
#endif
}

	
/* geom parameter==client requested geometry minus border crap */
static void floatframe_request_clientwin_geom(WFloatFrame *frame, 
											  WClientWin *cwin,
											  int rqflags, 
											  const WRectangle *geom_)
{
	int gravity=NorthWestGravity;
	XSizeHints hints;
	WRectangle off;
	WRegion *par;
	WRectangle geom=*geom_;
	
	if(cwin->size_hints.flags&PWinGravity)
		gravity=cwin->size_hints.win_gravity;

	floatframe_offsets(NULL, frame, &off);

	genframe_resize_hints((WGenFrame*)frame, &hints, NULL, NULL);
	correct_size(&(geom.w), &(geom.h), &hints, TRUE);
	
	geom.w+=off.w;
	geom.h+=off.h;
	
	/* If WEAK_? is set, then geom.(x|y) is root-relative as it was not 
	 * requested by the client and clientwin_handle_configure_request has
	 * no better guess. Otherwise the coordinates are those requested by 
	 * the client (modulo borders/gravity) and we interpret them to be 
	 * root-relative coordinates for this frame modulo gravity.
	 */
	if(rqflags&REGION_RQGEOM_WEAK_X)
		geom.x+=off.x;
	else
		geom.x+=gravity_deltax(gravity, -off.x, off.x+off.w);

	if(rqflags&REGION_RQGEOM_WEAK_Y)
		geom.y+=off.y;  /* geom.y was clientwin root relative y */
	else
		geom.y+=gravity_deltay(gravity, -off.y, off.y+off.h);

	par=region_parent((WRegion*)frame);
	region_convert_root_geom(par, &geom);
	if(par!=NULL){
		if(geom.x+geom.w<4)
			geom.x=-geom.w+4;
		if(geom.x>REGION_GEOM(par).w-4)
			geom.x=REGION_GEOM(par).w-4;
		if(geom.y+geom.h<4)
			geom.y=-geom.h+4;
		if(geom.y>REGION_GEOM(par).h-4)
			geom.y=REGION_GEOM(par).h-4;
	}

	region_request_geom((WRegion*)frame, REGION_RQGEOM_NORMAL, &geom, NULL);
}


/*}}}*/


/*{{{ Drawing routines and such */


static void floatframe_brushes_updated(WFloatFrame *frame)
{
	ExtlTab tab;
	
	/* Get new bar width limits */

	frame->tab_min_w=100;
	frame->bar_max_width_q=0.95;

	if(frame->genframe.brush==NULL)
		return;
	
	grbrush_get_extra_values(frame->genframe.brush, &tab);
	
	if(extl_table_gets_i(tab, "floatframe_tab_min_w", &(frame->tab_min_w))){
		if(frame->tab_min_w<=0)
			frame->tab_min_w=1;
	}

	if(extl_table_gets_d(tab, "floatframe_bar_max_w_q", 
						 &(frame->bar_max_width_q))){
		if(frame->bar_max_width_q<=0.0 || frame->bar_max_width_q>1.0)
			frame->bar_max_width_q=1.0;
	}
}


static void floatframe_set_shape(WFloatFrame *frame)
{
	WRectangle gs[2];
	
	if(frame->genframe.brush!=NULL){
		floatframe_bar_geom(frame, gs+0);
		floatframe_border_geom(frame, gs+1);
	
		grbrush_set_window_shape(frame->genframe.brush, WGENFRAME_WIN(frame),
								 TRUE, 2, gs);
	}
}


#define CF_TAB_MAX_TEXT_X_OFF 10


static int init_title(WFloatFrame *frame, int i)
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


static void floatframe_recalc_bar(WFloatFrame *frame)
{
	int bar_w=0, textw=0, tmaxw=frame->tab_min_w, tmp=0;
	WRegion *sub;
	const char *p;
	GrBorderWidths bdw;
	char *title;
	uint bdtotal;
	int i, m;
	
	if(frame->genframe.bar_brush==NULL)
		return;
	
	m=WGENFRAME_MCOUNT(frame);
	
	if(m>0){
		grbrush_get_border_widths(frame->genframe.bar_brush, &bdw);
		bdtotal=((m-1)*(bdw.tb_ileft+bdw.tb_iright)
				 +bdw.right+bdw.left);

		FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
			p=region_name(sub);
			if(p==NULL)
				continue;
			
			textw=grbrush_get_text_width(frame->genframe.bar_brush,
										 p, strlen(p));
			if(textw>tmaxw)
				tmaxw=textw;
		}

		bar_w=frame->bar_max_width_q*REGION_GEOM(frame).w;
		if(bar_w<frame->tab_min_w && 
		   REGION_GEOM(frame).w>frame->tab_min_w)
			bar_w=frame->tab_min_w;
		
		tmp=bar_w-bdtotal-m*tmaxw;
		
		if(tmp>0){
			/* No label truncation needed, good. See how much can be padded. */
			tmp/=m*2;
			if(tmp>CF_TAB_MAX_TEXT_X_OFF)
				tmp=CF_TAB_MAX_TEXT_X_OFF;
			bar_w=(tmaxw+tmp*2)*m+bdtotal;
		}else{
			/* Some labels must be truncated */
		}
	}else{
		bar_w=frame->tab_min_w;
		if(bar_w>frame->bar_max_width_q*REGION_GEOM(frame).w)
			bar_w=frame->bar_max_width_q*REGION_GEOM(frame).w;
	}

	if(frame->bar_w!=bar_w){
		frame->bar_w=bar_w;
		floatframe_set_shape(frame);
	}

	if(m==0 || frame->genframe.titles==NULL)
		return;
	
	i=0;
	FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(frame), sub){
		textw=init_title(frame, i);
		if(textw>0){
			title=region_make_label(sub, textw, frame->genframe.bar_brush);
			frame->genframe.titles[i].text=title;
		}
		i++;
	}
}


static void floatframe_size_changed(WFloatFrame *frame, bool wchg, bool hchg)
{
	int bar_w=frame->bar_w;
	
	if(wchg)
		genframe_recalc_bar((WGenFrame*)frame);
	if(hchg || (wchg && bar_w==frame->bar_w))
		floatframe_set_shape(frame);
}


static const char *floatframe_style_default(WFloatFrame *frame)
{
	return "frame-floatframe";
}


static const char *floatframe_tab_style_default(WFloatFrame *frame)
{
	return "tab-frame-floatframe";
}


/*static void floatframe_draw_config_updated(WFloatFrame *floatframe)
{
	genframe_draw_config_updated((WGenFrame*)floatframe);
}*/


/*}}}*/


/*{{{ Add/remove */


void floatframe_remove_managed(WFloatFrame *frame, WRegion *reg)
{
	mplex_remove_managed((WMPlex*)frame, reg);
	if(WGENFRAME_MCOUNT(frame)==0 && !WOBJ_IS_BEING_DESTROYED(frame))
		defer_destroy((WObj*)frame);
}


/*}}}*/


/*{{{ Actions */


/*EXTL_DOC
 * Start moving \var{frame} with the mouse (or other pointing device).
 * this should only be used by binding to the \emph{mpress} or
 * \emph{mdrag} actions.
 */
EXTL_EXPORT_MEMBER
void floatframe_p_move(WFloatFrame *frame)
{
	genframe_p_move((WGenFrame*)frame);
}


/*EXTL_DOC
 * Toggle shade (only titlebar visible) mode.
 */
EXTL_EXPORT_MEMBER
void floatframe_toggle_shade(WFloatFrame *frame)
{
	WRectangle geom;
	floatframe_bar_geom(frame, &geom);
	genframe_do_toggle_shade((WGenFrame*)frame, geom.h);
}


/*EXTL_DOC
 * Toggle \var{frame} stickyness. Only works across frames on 
 * \type{WFloatWS} that have the same \type{WMPlex} parent.
 */
EXTL_EXPORT_MEMBER
void floatframe_toggle_sticky(WFloatFrame *frame)
{
	if(frame->sticky){
		objlist_remove(&floatws_sticky_list, (WObj*)frame);
		frame->sticky=FALSE;
	}else{
		objlist_insert(&floatws_sticky_list, (WObj*)frame);
		frame->sticky=TRUE;
	}
}

/*EXTL_DOC
 * Is \var{frame} sticky?
 */
EXTL_EXPORT_MEMBER
bool floatframe_is_sticky(WFloatFrame *frame)
{
	return frame->sticky;
}


/*}}}*/


/*{{{ Save/load */


static bool floatframe_save_to_file(WFloatFrame *frame, FILE *file, int lvl)
{
	WRegion *sub;
	
	begin_saved_region((WRegion*)frame, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "flags = %d,\n", frame->genframe.flags);
	if(frame->sticky){
		save_indent_line(file, lvl);
		fprintf(file, "sticky = true,\n");
	}
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


WRegion *floatframe_load(WWindow *par, const WRectangle *geom, ExtlTab tab)
{
	int flags=0;
	ExtlTab substab, subtab;
	WFloatFrame *frame;
	int n, i;
	
	if(!extl_table_gets_t(tab, "subs", &substab))
		return NULL;

	frame=create_floatframe(par, geom);
	
	if(frame!=NULL){
		n=extl_table_get_n(substab);
		for(i=1; i<=n; i++){
			if(extl_table_geti_t(substab, i, &subtab)){
				mplex_attach_new((WMPlex*)frame, subtab);
				extl_unref_table(subtab);
			}
		}
	}
	
	extl_unref_table(substab);
	
	if(extl_table_is_bool_set(tab, "sticky"))
		floatframe_toggle_sticky(frame);
	
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
	{mplex_size_changed, floatframe_size_changed},
	{genframe_recalc_bar, floatframe_recalc_bar},

	{mplex_managed_geom, floatframe_managed_geom},
	{genframe_bar_geom, floatframe_bar_geom},
	{genframe_border_inner_geom, floatframe_border_inner_geom},
	{genframe_border_geom, floatframe_border_geom},
	{region_remove_managed, floatframe_remove_managed},
	
	{region_request_clientwin_geom, floatframe_request_clientwin_geom},
	
	{(DynFun*)region_save_to_file, (DynFun*)floatframe_save_to_file},

	{(DynFun*)genframe_style, (DynFun*)floatframe_style_default},
	{(DynFun*)genframe_tab_style, (DynFun*)floatframe_tab_style_default},
	
	/*{region_draw_config_updated, floatframe_draw_config_updated},*/
	
	{genframe_brushes_updated, floatframe_brushes_updated},
	
	END_DYNFUNTAB
};


IMPLOBJ(WFloatFrame, WGenFrame, deinit_floatframe, floatframe_dynfuntab);

		
/*}}}*/
