/*
 * ion/de/brush.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <libtu/objp.h>

#include "brush.h"
#include "font.h"
#include "colour.h"
#include "misc.h"


/*{{{ GC creation */


static void create_normal_gc(DEStyle *style, WRootWin *rootwin)
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
	
	style->normal_gc=XCreateGC(ioncore_g.dpy, WROOTWIN_ROOT(rootwin), 
                               gcvmask, &gcv);
}


static void create_tab_gcs(DEStyle *style, WRootWin *rootwin)
{
	Pixmap stipple_pixmap;
	XGCValues gcv;
	ulong gcvmask;
	GC tmp_gc;
	Display *dpy=ioncore_g.dpy;
	Window root=WROOTWIN_ROOT(rootwin);

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
	/*gcv.function=GXclear;*/
	gcv.stipple=stipple_pixmap;
	gcvmask=GCFillStyle|GCStipple/*|GCFunction*/;
		
	style->stipple_gc=XCreateGC(dpy, root, gcvmask, &gcv);
	XCopyGC(dpy, style->normal_gc, 
			GCLineStyle|GCLineWidth|GCJoinStyle|GCCapStyle,
			style->stipple_gc);
			
	XFreePixmap(dpy, stipple_pixmap);
	
	/* Create tag pixmap and copying GC */
	style->tag_pixmap_w=5;
	style->tag_pixmap_h=5;
	style->tag_pixmap=XCreatePixmap(dpy, root, 5, 5, 1);
	
	XSetForeground(dpy, tmp_gc, 0);
	XFillRectangle(dpy, style->tag_pixmap, tmp_gc, 0, 0, 5, 5);
	XSetForeground(dpy, tmp_gc, 1);
	XFillRectangle(dpy, style->tag_pixmap, tmp_gc, 0, 0, 5, 2);
	XFillRectangle(dpy, style->tag_pixmap, tmp_gc, 3, 2, 2, 3);
	
	gcv.foreground=DE_BLACK(rootwin);
	gcv.background=DE_WHITE(rootwin);
	gcv.line_width=2;
	gcvmask=GCLineWidth|GCForeground|GCBackground;
	
	style->copy_gc=XCreateGC(dpy, root, gcvmask, &gcv);
	
	XFreeGC(dpy, tmp_gc);
	
	style->tabbrush_data_ok=TRUE;
}


static void init_sub_ind_w(DEStyle *style)
{
	if(style->font==NULL){
		style->sub_ind_w=0;
	}else{
		style->sub_ind_w=defont_get_text_width(style->font, 
											   DE_SUB_IND, DE_SUB_IND_LEN);
	}
	style->mentbrush_data_ok=TRUE;
}


/*}}}*/


/*{{{ Style lookup */


static DEStyle *styles=NULL;


DEStyle *de_get_style(WRootWin *rootwin, const char *stylename)
{
	DEStyle *style, *maxstyle=NULL;
	int score, maxscore=0;
	
	for(style=styles; style!=NULL; style=style->next){
		if(style->rootwin!=rootwin)
			continue;
		score=gr_stylespec_score(style->style, stylename);
		if(score>maxscore){
			maxstyle=style;
			maxscore=score;
		}
	}
	
	return maxstyle;
}


/*}}}*/


/*{{{ Style initialisation and deinitialisation */


static void unref_style(DEStyle *style)
{
	style->usecount--;
	if(style->usecount==0){
		destyle_deinit(style);
		free(style);
	}
}


void destyle_deinit(DEStyle *style)
{
	int i;
	
	UNLINK_ITEM(styles, style, next, prev);
	
	if(style->style!=NULL)
		free(style->style);
	
	if(style->font!=NULL){
		de_free_font(style->font);
		style->font=NULL;
	}
	
	if(style->cgrp_alloced)
		de_free_colour_group(style->rootwin, &(style->cgrp));
	
	for(i=0; i<style->n_extra_cgrps; i++)
		de_free_colour_group(style->rootwin, style->extra_cgrps+i);
	
	if(style->extra_cgrps!=NULL)
		free(style->extra_cgrps);
	
	extl_unref_table(style->data_table);
	
	XFreeGC(ioncore_g.dpy, style->normal_gc);
	
	if(style->tabbrush_data_ok){
		XFreeGC(ioncore_g.dpy, style->copy_gc);
		XFreeGC(ioncore_g.dpy, style->stipple_gc);
		XFreePixmap(ioncore_g.dpy, style->tag_pixmap);
	}
    
    XSync(ioncore_g.dpy, False);
    
    if(style->based_on!=NULL){
        unref_style(style->based_on);
        style->based_on=NULL;
    }
}


static void dump_style(DEStyle *style)
{
	/* Allow the style still be used but get if off the list. */
	UNLINK_ITEM(styles, style, next, prev);
	unref_style(style);
}


static bool destyle_init(DEStyle *style, WRootWin *rootwin, const char *name)
{
	style->style=scopy(name);
	if(style->style==NULL){
		warn_err();
		return FALSE;
	}
	
    style->based_on=NULL;
    
	style->usecount=1;
	/* Fallback brushes are not released on de_reset() */
	style->is_fallback=FALSE;
	
	style->rootwin=rootwin;
	
	style->border.sh=1;
	style->border.hl=1;
	style->border.pad=1;
	style->border.style=DEBORDER_INLAID;

	style->spacing=0;
	
	style->textalign=DEALIGN_CENTER;

	style->cgrp_alloced=FALSE;
	style->cgrp.spec=NULL;
	style->cgrp.bg=DE_BLACK(rootwin);
	style->cgrp.pad=DE_BLACK(rootwin);
	style->cgrp.fg=DE_WHITE(rootwin);
	style->cgrp.hl=DE_WHITE(rootwin);
	style->cgrp.sh=DE_WHITE(rootwin);
	
	style->font=NULL;
	
	style->transparency_mode=GR_TRANSPARENCY_NO;
	
	style->n_extra_cgrps=0;
	style->extra_cgrps=NULL;
	
	style->data_table=extl_table_none();
	
	create_normal_gc(style, rootwin);
	
	style->tabbrush_data_ok=FALSE;
	style->mentbrush_data_ok=FALSE;
	
	return TRUE;
}


static DEStyle *do_create_style(WRootWin *rootwin, const char *name)
{
	DEStyle *style=ALLOC(DEStyle);
	if(style!=NULL){
		if(!destyle_init(style, rootwin, name)){
			free(style);
			return NULL;
		}
	}
	return style;
}


DEStyle *de_create_style(WRootWin *rootwin, const char *name)
{
	DEStyle *oldstyle, *style;

	style=do_create_style(rootwin, name);
	
	if(style==NULL)
		return NULL;
	
	for(oldstyle=styles; oldstyle!=NULL; oldstyle=oldstyle->next){
		if(oldstyle->rootwin==rootwin && oldstyle->style!=NULL && 
		   strcmp(oldstyle->style, name)==0){
			break;
		}
	}
	
	if(oldstyle!=NULL && !oldstyle->is_fallback)
		dump_style(oldstyle);
	
	LINK_ITEM_FIRST(styles, style, next, prev);
	
	return style;
}



/*EXTL_DOC
 * Clear all styles from drawing engine memory.
 */
EXTL_EXPORT
void de_reset()
{
	DEStyle *style, *next;
	for(style=styles; style!=NULL; style=next){
		next=style->next;
		if(!style->is_fallback)
			dump_style(style);
	}
}


void de_deinit_styles()
{
	DEStyle *style, *next;
	for(style=styles; style!=NULL; style=next){
		next=style->next;
		if(style->usecount>1){
			warn_obj("DE module", "Style %s still in use [%d] but the module "
					 "is being unloaded!", style->style, style->usecount);
		}
		dump_style(style);
	}
}


/*}}}*/


/*{{{ Brush creation and releasing */


static bool debrush_init(DEBrush *brush, DEStyle *style)
{
	brush->d=style;
	style->usecount++;
	
	if(!grbrush_init(&(brush->grbrush))){
		style->usecount--;
		return FALSE;
	}
	
	return TRUE;
}


static bool detabbrush_init(DETabBrush *brush, DEStyle *style)
{
	if(!style->tabbrush_data_ok)
		create_tab_gcs(style, style->rootwin);
	return debrush_init(&(brush->debrush), style);
}


static bool dementbrush_init(DEMEntBrush *brush, DEStyle *style)
{
	if(!style->mentbrush_data_ok)
		init_sub_ind_w(style);
	return debrush_init(&(brush->debrush), style);
}


DEBrush *create_debrush(DEStyle *style)
{
	CREATEOBJ_IMPL(DEBrush, debrush, (p, style));
}


DETabBrush *create_detabbrush(DEStyle *style)
{
	CREATEOBJ_IMPL(DETabBrush, detabbrush, (p, style));
}


DEMEntBrush *create_dementbrush(DEStyle *style)
{
	CREATEOBJ_IMPL(DEMEntBrush, dementbrush, (p, style));
}


static DEBrush *do_get_brush(WRootWin *rootwin, Window win, 
							 const char *stylename, bool slave)
{
	DEStyle *style=de_get_style(rootwin, stylename);
	DEBrush *brush;
	
	if(style==NULL)
		return NULL;
	
	if(MATCHES("tab-frame", stylename))
		brush=(DEBrush*)create_detabbrush(style);
	else if(MATCHES("tab-menuentry", stylename))
		brush=(DEBrush*)create_dementbrush(style);
	else
		brush=create_debrush(style);
	
	/* Set background colour */
	if(brush!=NULL && ! slave){
		grbrush_enable_transparency(&(brush->grbrush), win,
									GR_TRANSPARENCY_DEFAULT);
	}
	
	return brush;
}


DEBrush *de_get_brush(WRootWin *rootwin, Window win, const char *stylename)
{
	return do_get_brush(rootwin, win, stylename, FALSE);
}


DEBrush *debrush_get_slave(DEBrush *master, WRootWin *rootwin, 
						   Window win, const char *stylename)
{
	return do_get_brush(rootwin, win, stylename, TRUE);
}


void debrush_deinit(DEBrush *brush)
{
	unref_style(brush->d);
	brush->d=NULL;
	grbrush_deinit(&(brush->grbrush));
}


void detabbrush_deinit(DETabBrush *brush)
{
	debrush_deinit(&(brush->debrush));
}


void dementbrush_deinit(DEMEntBrush *brush)
{
	debrush_deinit(&(brush->debrush));
}


void debrush_release(DEBrush *brush, Window win)
{
	destroy_obj((Obj*)brush);
}


/*}}}*/


/*{{{ Border widths */


void destyle_get_border_widths(DEStyle *style, GrBorderWidths *bdw)
{
	DEBorder *bd=&(style->border);
	uint tmp;
	
	switch(bd->style){
	case DEBORDER_RIDGE:
	case DEBORDER_GROOVE:
		tmp=bd->sh+bd->hl+bd->pad;
		bdw->top=tmp; bdw->bottom=tmp; bdw->left=tmp; bdw->right=tmp;
		break;
	case DEBORDER_INLAID:
		tmp=bd->sh+bd->pad; bdw->top=tmp; bdw->left=tmp;
		tmp=bd->hl+bd->pad; bdw->bottom=tmp; bdw->right=tmp;
		break;
	case DEBORDER_ELEVATED:
	default:
		tmp=bd->hl+bd->pad; bdw->top=tmp; bdw->left=tmp;
		tmp=bd->sh+bd->pad; bdw->bottom=tmp; bdw->right=tmp;
		break;
	}
	
	bdw->tb_ileft=bdw->left;
	bdw->tb_iright=bdw->right;
	bdw->spacing=style->spacing;
}


void destyle_get_mentbrush_border_widths(DEStyle *style, GrBorderWidths *bdw)
{
	if(!style->mentbrush_data_ok)
		init_sub_ind_w(style);
	destyle_get_border_widths(style, bdw);
	bdw->right+=style->sub_ind_w;
	bdw->tb_iright+=style->sub_ind_w;
}


void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw)
{
	destyle_get_border_widths(brush->d, bdw);
}


void dementbrush_get_border_widths(DEMEntBrush *brush, GrBorderWidths *bdw)
{
	destyle_get_mentbrush_border_widths(brush->debrush.d, bdw);
}


/*}}}*/


/*{{{ Misc. */


bool destyle_get_extra(DEStyle *style, const char *key, char type, void *data)
{
    if(extl_table_get(style->data_table, 's', type, key, data))
        return TRUE;
    if(style->based_on!=NULL)
        return destyle_get_extra(style->based_on, key, type, data);
    return FALSE;
}


bool debrush_get_extra(DEBrush *brush, const char *key, char type, void *data)
{
    if(brush->d==NULL)
        return FALSE;
    return destyle_get_extra(brush->d, key, type, data);
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
	{(DynFun*)grbrush_get_extra, (DynFun*)debrush_get_extra},
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


IMPLCLASS(DEBrush, GrBrush, debrush_deinit, debrush_dynfuntab);
IMPLCLASS(DETabBrush, DEBrush, detabbrush_deinit, detabbrush_dynfuntab);
IMPLCLASS(DEMEntBrush, DEBrush, dementbrush_deinit, dementbrush_dynfuntab);


/*}}}*/



