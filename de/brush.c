/*
 * ion/de/brush.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/extl.h>
#include <ioncore/extlconv.h>
#include <ioncore/ioncore.h>

#include "brush.h"
#include "font.h"
#include "colour.h"


/*{{{ GC creation */


static void create_normal_gc(DEBrush *brush, WRootWin *rootwin)
{
	XGCValues gcv;
	ulong gcvmask;
	GC gc;

	/* Create normal gc */
	gcv.line_style=LineSolid;
	gcv.line_width=1;
	gcv.join_style=JoinBevel;
	gcv.cap_style=CapButt;
	gcv.fill_style=FillSolid;
	gcvmask=(GCLineStyle|GCLineWidth|GCFillStyle|
			 GCJoinStyle|GCCapStyle);
#ifndef CF_UTF8
	if(brush->font!=NULL){
		gcv.font=brush->font->fid;
		gcvmask|=GCFont;
	}
#endif
	brush->normal_gc=XCreateGC(wglobal.dpy, ROOT_OF(rootwin), gcvmask, &gcv);
}


/* This should be called after reading the configuration file */
static void create_tab_gcs(DETabBrush *brush, WRootWin *rootwin)
{
	Pixmap stipple_pixmap;
	XGCValues gcv;
	ulong gcvmask;
	GC tmp_gc;
	Display *dpy=wglobal.dpy;
	Window root=ROOT_OF(rootwin);

	/* Create a temporary 1-bit GC for drawing the tag and stipple pixmaps */
	stipple_pixmap=XCreatePixmap(dpy, root, 2, 2, 1);
	gcv.foreground=1;
	tmp_gc=XCreateGC(dpy, stipple_pixmap, GCForeground, &gcv);

	/* Create stipple pattern and stipple GC */
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 1);
	XSetForeground(dpy, tmp_gc, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 1);
	
	gcv.fill_style=FillStippled;
	gcv.function=GXclear;
	gcv.stipple=stipple_pixmap;
	gcvmask=GCFillStyle|GCStipple|GCFunction;
		
	brush->stipple_gc=XCreateGC(dpy, root, gcvmask, &gcv);
	
	XFreePixmap(dpy, stipple_pixmap);
	
	/* Create tag pixmap and copying GC */
	brush->tag_pixmap_w=5;
	brush->tag_pixmap_h=5;
	brush->tag_pixmap=XCreatePixmap(dpy, root, 5, 5, 1);
	
	XSetForeground(dpy, tmp_gc, 0);
	XFillRectangle(dpy, brush->tag_pixmap, tmp_gc, 0, 0, 5, 5);
	XSetForeground(dpy, tmp_gc, 1);
	XFillRectangle(dpy, brush->tag_pixmap, tmp_gc, 0, 0, 5, 2);
	XFillRectangle(dpy, brush->tag_pixmap, tmp_gc, 3, 2, 2, 3);
	
	gcv.foreground=DE_BLACK(rootwin);
	gcv.background=DE_WHITE(rootwin);
	gcv.line_width=2;
	gcvmask=GCLineWidth|GCForeground|GCBackground;
	
	brush->copy_gc=XCreateGC(dpy, root, gcvmask, &gcv);
	
	XFreeGC(dpy, tmp_gc);
}


/*}}}*/


/*{{{ Brush lookup */


static DEBrush *brushes=NULL;


static DEBrush *do_get_brush(WRootWin *rootwin, const char *style, bool inc)
{
	DEBrush *brush, *maxbrush=NULL;
	int score, maxscore=0;
	
	for(brush=brushes; brush!=NULL; brush=brush->next){
		if(brush->rootwin!=rootwin)
			continue;
		score=gr_stylespec_score(brush->style, style);
		if(score>maxscore){
			maxbrush=brush;
			maxscore=score;
		}
	}
	
	if(maxbrush!=NULL && inc)
		maxbrush->usecount++;
	
	return maxbrush;
}


DEBrush *de_get_brush(WRootWin *rootwin, Window win, const char *style)
{
	DEBrush *brush=do_get_brush(rootwin, style, TRUE);
	
	/* Set background colour */
	if(brush!=NULL)
		grbrush_enable_transparency(&(brush->grbrush), win,
									GR_TRANSPARENCY_DEFAULT);
	
	return brush;
}


DEBrush *debrush_get_slave(DEBrush *master, WRootWin *rootwin, 
						   Window win, const char *style)
{
	return do_get_brush(rootwin, style, TRUE);
}


/*}}}*/


/*{{{ Brush initialisation and deinitialisation */


static bool debrush_init(DEBrush *brush, WRootWin *rootwin, const char *name)
{
	if(!grbrush_init(&(brush->grbrush)))
		return FALSE;
	   
	brush->style=scopy(name);
	if(brush->style==NULL){
		warn_err();
		return FALSE;
	}
	
	brush->usecount=1;
	/* Fallback brushes are not released on de_reset() */
	brush->is_fallback=FALSE;
	
	brush->rootwin=rootwin;
	
	brush->border.sh=1;
	brush->border.hl=1;
	brush->border.pad=1;
	brush->border.style=DEBORDER_INLAID;

	brush->spacing=0;
	
	brush->textalign=DEALIGN_CENTER;

	brush->cgrp_alloced=FALSE;
	brush->cgrp.spec=NULL;
	brush->cgrp.bg=DE_BLACK(rootwin);
	brush->cgrp.pad=DE_BLACK(rootwin);
	brush->cgrp.fg=DE_WHITE(rootwin);
	brush->cgrp.hl=DE_WHITE(rootwin);
	brush->cgrp.sh=DE_WHITE(rootwin);
	
	brush->font=NULL;
	
	brush->transparency_mode=GR_TRANSPARENCY_NO;
	
	brush->n_extra_cgrps=0;
	brush->extra_cgrps=NULL;
	
	brush->data_table=extl_table_none();
	
	create_normal_gc(brush, rootwin);
	
	return TRUE;
}


static bool detabbrush_init(DETabBrush *brush, WRootWin *rootwin, 
							const char *name)
{
	if(!debrush_init(&(brush->debrush), rootwin, name))
		return FALSE;
	create_tab_gcs(brush, rootwin);
	return TRUE;
}


static bool dementbrush_init(DEMEntBrush *brush, WRootWin *rootwin, 
							 const char *name)
{
	if(!debrush_init(&(brush->debrush), rootwin, name))
		return FALSE;
	brush->sub_ind_w=20;
	return TRUE;
}


DEBrush *create_debrush(WRootWin *rootwin, const char *name)
{
	CREATEOBJ_IMPL(DEBrush, debrush, (p, rootwin, name));
}


DETabBrush *create_detabbrush(WRootWin *rootwin, const char *name)
{
	CREATEOBJ_IMPL(DETabBrush, detabbrush, (p, rootwin, name));
}


DEMEntBrush *create_dementbrush(WRootWin *rootwin, const char *name)
{
	CREATEOBJ_IMPL(DEMEntBrush, dementbrush, (p, rootwin, name));
}


static void dump_brush(DEBrush *brush)
{
	/* Allow the brush still be used but get if off the list. */
	UNLINK_ITEM(brushes, brush, next, prev);
	debrush_release(brush, None);
}


DEBrush *de_find_or_create_brush(WRootWin *rootwin, const char *name)
{
	DEBrush *oldbrush, *brush;
	Window root=ROOT_OF(rootwin);
	
	for(oldbrush=brushes; oldbrush!=NULL; oldbrush=oldbrush->next){
		if(oldbrush->rootwin==rootwin && oldbrush->style!=NULL && 
		   strcmp(oldbrush->style, name)==0){
			break;
		}
	}

	if(MATCHES("tab-frame", name))
		brush=(DEBrush*)create_detabbrush(rootwin, name);
	else if(MATCHES("tab-menuentry", name))
		brush=(DEBrush*)create_dementbrush(rootwin, name);
	else
		brush=create_debrush(rootwin, name);
	
	if(brush==NULL)
		return NULL;
	
	if(oldbrush!=NULL && !oldbrush->is_fallback)
		dump_brush(oldbrush);
	
	LINK_ITEM_FIRST(brushes, brush, next, prev);
	
	return brush;
}


void debrush_deinit(DEBrush *brush)
{
	int i;

	grbrush_deinit(&(brush->grbrush));

	if(brush->style!=NULL)
		free(brush->style);
	
	if(brush->prev!=NULL){
		UNLINK_ITEM(brushes, brush, next, prev);
	}
	
	if(brush->font!=NULL)
		de_free_font(brush->font);
	
	if(brush->cgrp_alloced)
		de_free_colour_group(brush->rootwin, &(brush->cgrp));
	
	for(i=0; i<brush->n_extra_cgrps; i++)
		de_free_colour_group(brush->rootwin, brush->extra_cgrps+i);

	XSync(wglobal.dpy, False);
	
	if(brush->extra_cgrps!=NULL)
		free(brush->extra_cgrps);
	
	extl_unref_table(brush->data_table);
	
	XFreeGC(wglobal.dpy, brush->normal_gc);
}


void detabbrush_deinit(DETabBrush *brush)
{
	debrush_deinit(&(brush->debrush));
	
	XFreeGC(wglobal.dpy, brush->copy_gc);
	XFreeGC(wglobal.dpy, brush->stipple_gc);
	XFreePixmap(wglobal.dpy, brush->tag_pixmap);
}


void dementbrush_deinit(DEMEntBrush *brush)
{
	debrush_deinit(&(brush->debrush));
}


void debrush_release(DEBrush *brush, Window win)
{
	brush->usecount--;
	if(brush->usecount<=0)
		destroy_obj((WObj*)brush);
}


/*EXTL_DOC
 * Clear all styles from drawing engine memory.
 */
EXTL_EXPORT
void de_reset()
{
	DEBrush *brush, *next;
	for(brush=brushes; brush!=NULL; brush=next){
		next=brush->next;
		if(!brush->is_fallback)
			dump_brush(brush);
	}
}


void de_deinit_brushes()
{
	DEBrush *brush, *next;
	for(brush=brushes; brush!=NULL; brush=next){
		next=brush->next;
		brush->usecount--;
		if(brush->usecount>0){
			warn_obj("DE module", "Brush %s still in use [%d] but the module "
					 "is being unloaded!", brush->style, brush->usecount);
		}else{
			destroy_obj((WObj*)brush);
		}
	}
}


/*}}}*/


/*{{{ Misc. */


void debrush_get_extra_values(DEBrush *brush, ExtlTab *tab)
{
	*tab=extl_ref_table(brush->data_table);
}


bool de_get_brush_values(WRootWin *rootwin, const char *style,
						 GrBorderWidths *bdw, GrFontExtents *fnte,
						 ExtlTab *tab)
{
	DEBrush *brush=do_get_brush(rootwin, style, FALSE);
	
	if(brush==NULL)
		return FALSE;
	
	if(bdw!=NULL)
		grbrush_get_border_widths(&(brush->grbrush), bdw);
	if(fnte!=NULL)
		grbrush_get_font_extents(&(brush->grbrush), fnte);
	if(tab!=NULL)
		grbrush_get_extra_values(&(brush->grbrush), tab);
	
	return TRUE;
}


/*}}}*/


/*{{{ Class implementations */


static DynFunTab debrush_dynfuntab[]={
	{grbrush_release, debrush_release},
	{grbrush_draw_border, debrush_draw_border},
	{grbrush_get_border_widths, debrush_get_border_widths},
	{grbrush_draw_string, debrush_draw_string},
	{grbrush_get_font_extents, debrush_get_font_extents},
	{(DynFun*)grbrush_get_text_width, (DynFun*)debrush_get_text_width},
	{grbrush_draw_textbox, debrush_draw_textbox},
	{grbrush_draw_textboxes, debrush_draw_textboxes},
	{grbrush_set_clipping_rectangle, debrush_set_clipping_rectangle},
	{grbrush_clear_clipping_rectangle, debrush_clear_clipping_rectangle},
	{grbrush_set_window_shape, debrush_set_window_shape},
	{grbrush_enable_transparency, debrush_enable_transparency},
	{grbrush_clear_area, debrush_clear_area},
	{grbrush_fill_area, debrush_fill_area},
	{grbrush_get_extra_values, debrush_get_extra_values},
	{(DynFun*)grbrush_get_slave, (DynFun*)debrush_get_slave},
	END_DYNFUNTAB
};
									   

static DynFunTab detabbrush_dynfuntab[]={
	{grbrush_draw_textbox, detabbrush_draw_textbox},
	{grbrush_draw_textboxes, detabbrush_draw_textboxes},
	END_DYNFUNTAB
};
									   

static DynFunTab dementbrush_dynfuntab[]={
	{grbrush_draw_textbox, dementbrush_draw_textbox},
	{grbrush_draw_textboxes, dementbrush_draw_textboxes},
	{grbrush_get_border_widths, dementbrush_get_border_widths},
	END_DYNFUNTAB
};
									   

IMPLOBJ(DEBrush, GrBrush, debrush_deinit, debrush_dynfuntab);
IMPLOBJ(DETabBrush, DEBrush, detabbrush_deinit, detabbrush_dynfuntab);
IMPLOBJ(DEMEntBrush, DEBrush, dementbrush_deinit, dementbrush_dynfuntab);


/*}}}*/

