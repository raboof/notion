/*
 * ion/de/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/global.h>
#include <ioncore/common.h>
#include <ioncore/gr.h>
#include "brush.h"
#include "font.h"
#include "misc.h"

#include <X11/extensions/shape.h>


/*{{{ Colour group lookup */


DEColourGroup *debrush_get_colour_group2(DEBrush *brush, 
										 const char *attr_p1,
										 const char *attr_p2)
{
	int i, maxi=-1;
	int score, maxscore=0;
	
	for(i=0; i<brush->n_extra_cgrps; i++){
		score=gr_stylespec_score2(brush->extra_cgrps[i].spec,
								  attr_p1, attr_p2);
		if(score>maxscore){
			maxi=i;
			maxscore=score;
		}
	}
	
	if(maxi==-1)
		return &(brush->cgrp);
	else
		return &(brush->extra_cgrps[maxi]);
}



DEColourGroup *debrush_get_colour_group(DEBrush *brush, const char *attr)
{
	return debrush_get_colour_group2(brush, attr, NULL);
}


/*}}}*/


/*{{{ Borders */


/* Draw a border at x, y with outer width w x h. Top and left 'tl' pixels
 * wide with color 'tlc' and bottom and right 'br' pixels with colors 'brc'.
 */
static void do_draw_border(Window win, GC gc, int x, int y, int w, int h,
						   uint tl, uint br, DEColour tlc, DEColour brc)
{
	XPoint points[3];
	uint i=0, a=0, b=0;
	
	w--;
	h--;

	XSetForeground(wglobal.dpy, gc, tlc);

	
	a=(br!=0);
	b=0;
	
	for(i=0; i<tl; i++){
		points[0].x=x+i;		points[0].y=y+h+1-b;
		points[1].x=x+i;		points[1].y=y+i;
		points[2].x=x+w+1-a;	points[2].y=y+i;

		if(a<br)
			a++;
		if(b<br)
			b++;
	
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}

	
	XSetForeground(wglobal.dpy, gc, brc);

	a=(tl!=0);
	b=0;
	
	for(i=0; i<br; i++){
		points[0].x=x+w-i;		points[0].y=y+b;
		points[1].x=x+w-i;		points[1].y=y+h-i;
		points[2].x=x+a;		points[2].y=y+h-i;
	
		if(a<tl)
			a++;
		if(b<tl)
			b++;
		
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}
}


static void draw_border(Window win, GC gc, WRectangle *geom,
						uint tl, uint br, DEColour tlc, DEColour brc)
{
	do_draw_border(win, gc, geom->x, geom->y, geom->w, geom->h,
				   tl, br, tlc, brc);
	geom->x+=tl;
	geom->y+=tl;
	geom->w-=tl+br;
	geom->h-=tl+br;
}


void debrush_do_draw_border(DEBrush *brush, Window win, WRectangle geom,
							DEColourGroup *cg)
{
	DEBorder *bd=&(brush->border);
	GC gc=brush->normal_gc;
	
	switch(bd->style){
	case DEBORDER_RIDGE:
		draw_border(win, gc, &geom, bd->hl, bd->sh, cg->hl, cg->sh);
	case DEBORDER_INLAID:
		draw_border(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad);
		draw_border(win, gc, &geom, bd->sh, bd->hl, cg->sh, cg->hl);
		break;
	case DEBORDER_GROOVE:
		draw_border(win, gc, &geom, bd->sh, bd->hl, cg->sh, cg->hl);
		draw_border(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad);
		draw_border(win, gc, &geom, bd->hl, bd->sh, cg->hl, cg->sh);
		break;
	case DEBORDER_ELEVATED:
	default:
		draw_border(win, gc, &geom, bd->hl, bd->sh, cg->hl, cg->sh);
		draw_border(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad);
		break;
	}
}


void debrush_draw_border(DEBrush *brush, Window win,
						 const WRectangle *geom,
						 const char *attrib)
{
	DEColourGroup *cg=debrush_get_colour_group(brush, attrib);
	if(cg!=NULL)
		debrush_do_draw_border(brush, win, *geom, cg);
}


void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw)
{
	DEBorder *bd=&(brush->border);
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
	bdw->spacing=brush->spacing;
}


void dementbrush_get_border_widths(DEMEntBrush *brush, GrBorderWidths *bdw)
{
	debrush_get_border_widths(&(brush->debrush), bdw);
	bdw->right+=brush->sub_ind_w;
	bdw->tb_iright+=brush->sub_ind_w;
}


/*}}}*/


/*{{{ Boxes */


typedef void TextBoxExtraFn(DEBrush *brush_, Window win, const WRectangle *g,
							DEColourGroup *cg, GrBorderWidths *bdw,
							GrFontExtents *fnte,
							const char *a1, const char *a2);


static void copy_masked(DETabBrush *brush, Drawable src, Drawable dst,
						int src_x, int src_y, int w, int h,
						int dst_x, int dst_y)
{
	
	GC copy_gc=brush->copy_gc;
	
	XSetClipMask(wglobal.dpy, copy_gc, src);
	XSetClipOrigin(wglobal.dpy, copy_gc, dst_x, dst_y);
	XCopyPlane(wglobal.dpy, src, dst, copy_gc, src_x, src_y, w, h,
			   dst_x, dst_y, 1);
}


static void tabbrush_textbox_extras(DETabBrush *brush, Window win,
									const WRectangle *g, DEColourGroup *cg,
									GrBorderWidths *bdw,
									GrFontExtents *fnte,
									const char *a1, const char *a2)
{
	if(MATCHES2("*-*-tagged", a1, a2)){
		XSetForeground(wglobal.dpy, brush->copy_gc, cg->fg);
			
		copy_masked(brush, brush->tag_pixmap, win, 0, 0,
					brush->tag_pixmap_w, brush->tag_pixmap_h,
					g->x+g->w-bdw->right-brush->tag_pixmap_w, 
					g->y+bdw->top);
	}

	if(MATCHES2("*-*-*-dragged", a1, a2)){
		XFillRectangle(wglobal.dpy, win, brush->stipple_gc, 
					   g->x, g->y, g->w, g->h);
	}
}


static void mentbrush_textbox_extras(DEMEntBrush *brush, Window win,
									 const WRectangle *g, DEColourGroup *cg,
									 GrBorderWidths *bdw,
									 GrFontExtents *fnte,
									 const char *a1, const char *a2)
{
	int tx, ty;

	if(!MATCHES2("*-submenu", a1, a2))
		return;
		
	ty=(g->y+bdw->top+fnte->baseline
		+(g->h-bdw->top-bdw->bottom-fnte->max_height)/2);
	tx=g->x+g->w-bdw->right;

	debrush_do_draw_string((DEBrush*)brush, win, tx, ty, DE_SUB_IND,
						   DE_SUB_IND_LEN, FALSE, cg);
}


void debrush_do_draw_box(DEBrush *brush, Window win, 
						 const WRectangle *geom, DEColourGroup *cg,
						 bool needfill)
{
	GC gc=brush->normal_gc;
	
	if(TRUE/*needfill*/){
		XSetForeground(wglobal.dpy, gc, cg->bg);
		XFillRectangle(wglobal.dpy, win, gc, geom->x, geom->y, 
					   geom->w, geom->h);
	}
	
	debrush_do_draw_border(brush, win, *geom, cg);
}


static void debrush_do_draw_textbox(DEBrush *brush, Window win, 
									const WRectangle *geom, const char *text, 
									DEColourGroup *cg, bool needfill,
									const char *a1, const char *a2,
									TextBoxExtraFn *extrafn)
{
	uint len;
	GrBorderWidths bdw;
	GrFontExtents fnte;
	uint tx, ty, tw;
	
	debrush_do_draw_box(brush, win, geom, cg, needfill);
	
	do{ /*...while(0)*/
		if(text==NULL)
			break;
		
		len=strlen(text);
	
		if(len==0)
			break;
	
		grbrush_get_border_widths(&(brush->grbrush), &bdw);
		grbrush_get_font_extents(&(brush->grbrush), &fnte);
		
		if(brush->textalign!=DEALIGN_LEFT){
			tw=debrush_get_text_width(brush, text, len);
			
			if(brush->textalign==DEALIGN_CENTER)
				tx=geom->x+bdw.left+(geom->w-bdw.left-bdw.right-tw)/2;
			else
				tx=geom->x+geom->w-bdw.right-tw;
		}else{
			tx=geom->x+bdw.left;
		}
		
		ty=(geom->y+bdw.top+fnte.baseline
			+(geom->h-bdw.top-bdw.bottom-fnte.max_height)/2);
		
		debrush_do_draw_string(brush, win, tx, ty, text, len, FALSE, cg);
	}while(0);
	
	if(extrafn!=NULL)
		extrafn(brush, win, geom, cg, &bdw, &fnte, a1, a2);
}


static void do_draw_textboxes(DEBrush *brush, Window win, 
							  const WRectangle *geom,
							  int n, const GrTextElem *elem, 
							  bool needfill, const char *common_attrib,
							  TextBoxExtraFn *extrafn)
{

	WRectangle g=*geom;
	DEColourGroup *cg;
	GrBorderWidths bdw;
	int i;
	
	grbrush_get_border_widths(&(brush->grbrush), &bdw);
	
	for(i=0; i<n; i++){
		g.w=bdw.left+elem[i].iw+bdw.right;
		cg=debrush_get_colour_group2(brush, common_attrib, elem[i].attr);
		
		debrush_do_draw_textbox(brush, win, &g, elem[i].text, cg, needfill,
								common_attrib, elem[i].attr, extrafn);
		
		g.x+=g.w;
		if(bdw.spacing>0 && needfill){
			XClearArea(wglobal.dpy, win, g.x, g.y,
					   brush->spacing, g.h, False);
		}
		g.x+=bdw.spacing;
	}
}


void debrush_draw_textbox(DEBrush *brush, Window win, 
						  const WRectangle *geom, const char *text, 
						  const char *attr, bool needfill)
{
	DEColourGroup *cg=debrush_get_colour_group(brush, attr);
	if(cg!=NULL){
		debrush_do_draw_textbox(brush, win, geom, text, cg, needfill, 
								attr, NULL, NULL);
	}
}


void detabbrush_draw_textbox(DETabBrush *brush, Window win, 
							 const WRectangle *geom, const char *text, 
							 const char *attr, bool needfill)
{
	DEColourGroup *cg=debrush_get_colour_group(&(brush->debrush), attr);
	if(cg!=NULL){
		debrush_do_draw_textbox(&(brush->debrush), win, geom, text, cg, 
								needfill, attr, NULL,
								(TextBoxExtraFn*)tabbrush_textbox_extras);
	}
}


void dementbrush_draw_textbox(DEMEntBrush *brush, Window win, 
							  const WRectangle *geom, const char *text, 
							  const char *attr, bool needfill)
{
	DEColourGroup *cg=debrush_get_colour_group(&(brush->debrush), attr);
	if(cg!=NULL){
		debrush_do_draw_textbox(&(brush->debrush), win, geom, text, cg, 
								needfill, attr, NULL,
								(TextBoxExtraFn*)mentbrush_textbox_extras);
	}
}


void debrush_draw_textboxes(DEBrush *brush, Window win, 
							const WRectangle *geom,
							int n, const GrTextElem *elem, 
							bool needfill, const char *common_attrib)
{
	do_draw_textboxes(brush, win, geom, n, elem, needfill, common_attrib, 
					  NULL);
}



void detabbrush_draw_textboxes(DETabBrush *brush, Window win, 
							   const WRectangle *geom,
							   int n, const GrTextElem *elem, 
							   bool needfill, const char *common_attrib)
{
	do_draw_textboxes(&(brush->debrush), win, geom, n, elem, 
					  needfill, common_attrib,
					  (TextBoxExtraFn*)tabbrush_textbox_extras);
}


void dementbrush_draw_textboxes(DEMEntBrush *brush, Window win, 
								const WRectangle *geom,
								int n, const GrTextElem *elem, 
								bool needfill, const char *common_attrib)
{
	do_draw_textboxes(&(brush->debrush), win, geom, n, elem, 
					  needfill, common_attrib,
					  (TextBoxExtraFn*)mentbrush_textbox_extras);
}


/*}}}*/


/*{{{ Clipping rectangles */

/* Should actually set the clipping rectangle for all GC:s and use 
 * window-specific GC:s to do this correctly...
 */

void debrush_set_clipping_rectangle(DEBrush *brush, Window unused,
									const WRectangle *geom)
{
	XRectangle rect;
	
	rect.x=geom->x;
	rect.y=geom->y;
	rect.width=geom->w;
	rect.height=geom->h;
	
	XSetClipRectangles(wglobal.dpy, brush->normal_gc,
					   0, 0, &rect, 1, Unsorted);
}


void debrush_clear_clipping_rectangle(DEBrush *brush, Window unused)
{
	XSetClipMask(wglobal.dpy, brush->normal_gc, None);
}


/*}}}*/


/*{{{ Misc. */

#define MAXSHAPE 16

void debrush_set_window_shape(DEBrush *brush, Window win, bool rough,
							  int n, const WRectangle *rects)
{
	XRectangle r[MAXSHAPE];
	int i;
	
	if(n>MAXSHAPE)
		n=MAXSHAPE;
	
	for(i=0; i<n; i++){
		r[i].x=rects[i].x;
		r[i].y=rects[i].y;
		r[i].width=rects[i].w;
		r[i].height=rects[i].h;
	}
	
	XShapeCombineRectangles(wglobal.dpy, win, ShapeBounding, 0, 0, r, n,
							ShapeSet, YXBanded);
}


void debrush_enable_transparency(DEBrush *brush, Window win, 
								 GrTransparency mode)
{
	XSetWindowAttributes attr;
	ulong attrflags=0;
	
	if(mode==GR_TRANSPARENCY_DEFAULT)
		mode=brush->transparency_mode;
	
	if(mode==GR_TRANSPARENCY_YES){
		attrflags=CWBackPixmap;
		attr.background_pixmap=ParentRelative;
	}else{
		attrflags=CWBackPixel;
		attr.background_pixel=brush->cgrp.bg;
	}
	
	XChangeWindowAttributes(wglobal.dpy, win, attrflags, &attr);
	XClearWindow(wglobal.dpy, win);
}


void debrush_fill_area(DEBrush *brush, Window win, const WRectangle *geom,
					   const char *attr)
{
	DEColourGroup *cg=debrush_get_colour_group(brush, attr);
	GC gc=brush->normal_gc;

	if(cg==NULL)
		return;
	
	XSetForeground(wglobal.dpy, gc, cg->bg);
	XFillRectangle(wglobal.dpy, win, gc, geom->x, geom->y, geom->w, geom->h);
}


void debrush_clear_area(DEBrush *brush, Window win, const WRectangle *geom)
{
	XClearArea(wglobal.dpy, win, geom->x, geom->y, geom->w, geom->h, False);
}


/*}}}*/


