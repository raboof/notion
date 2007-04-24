/*
 * ion/de/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <limits.h>

#include <ioncore/global.h>
#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/gr-util.h>
#include "brush.h"
#include "font.h"
#include "private.h"

#include <X11/extensions/shape.h>


/*{{{ Colour group lookup */


static DEColourGroup *destyle_get_colour_group2(DEStyle *style,
                                                const GrStyleSpec *a1,
                                                const GrStyleSpec *a2)
{
    int i, score, maxscore=0;
    DEColourGroup *maxg=&(style->cgrp);
    
    while(style!=NULL){
        for(i=0; i<style->n_extra_cgrps; i++){
            score=gr_stylespec_score2(&style->extra_cgrps[i].spec, a1, a2);
            
            if(score>maxscore){
                maxg=&(style->extra_cgrps[i]);
                maxscore=score;
            }
        }
        style=style->based_on;
    }
    
    return maxg;
}


DEColourGroup *debrush_get_colour_group2(DEBrush *brush, 
                                         const GrStyleSpec *a1,
                                         const GrStyleSpec *a2)
{
    return destyle_get_colour_group2(brush->d, a1, a2);
}


DEColourGroup *debrush_get_colour_group(DEBrush *brush, const GrStyleSpec *attr)
{
    return destyle_get_colour_group2(brush->d, attr, NULL);
}


DEColourGroup *debrush_get_current_colour_group(DEBrush *brush)
{
    return debrush_get_colour_group(brush, debrush_get_current_attr(brush));
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

    XSetForeground(ioncore_g.dpy, gc, tlc);

    
    a=(br!=0);
    b=0;
    
    for(i=0; i<tl; i++){
        points[0].x=x+i;        points[0].y=y+h+1-b;
        points[1].x=x+i;        points[1].y=y+i;
        points[2].x=x+w+1-a;    points[2].y=y+i;

        if(a<br)
            a++;
        if(b<br)
            b++;
    
        XDrawLines(ioncore_g.dpy, win, gc, points, 3, CoordModeOrigin);
    }

    
    XSetForeground(ioncore_g.dpy, gc, brc);

    a=(tl!=0);
    b=0;
    
    for(i=0; i<br; i++){
        points[0].x=x+w-i;      points[0].y=y+b;
        points[1].x=x+w-i;      points[1].y=y+h-i;
        points[2].x=x+a;        points[2].y=y+h-i;
    
        if(a<tl)
            a++;
        if(b<tl)
            b++;
        
        XDrawLines(ioncore_g.dpy, win, gc, points, 3, CoordModeOrigin);
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


static void draw_borderline(Window win, GC gc, WRectangle *geom,
                            uint tl, uint br, DEColour tlc, DEColour brc, 
                            GrBorderLine line)
{
    if(line==GR_BORDERLINE_LEFT && geom->h>0 && tl>0){
        XSetForeground(ioncore_g.dpy, gc, tlc);
        XSetBackground(ioncore_g.dpy, gc, tlc);
        XFillRectangle(ioncore_g.dpy, win, gc, geom->x, geom->y, tl, geom->h);
        geom->x+=tl;
    }else if(line==GR_BORDERLINE_TOP && geom->w>0 && tl>0){
        XSetForeground(ioncore_g.dpy, gc, tlc);
        XSetBackground(ioncore_g.dpy, gc, tlc);
        XFillRectangle(ioncore_g.dpy, win, gc, geom->x, geom->y, geom->w, tl);
        geom->y+=tl;
    }else if(line==GR_BORDERLINE_RIGHT && geom->h>0 && br>0){
        XSetForeground(ioncore_g.dpy, gc, brc);
        XSetBackground(ioncore_g.dpy, gc, brc);
        XFillRectangle(ioncore_g.dpy, win, gc, geom->x+geom->w-br, geom->y, br, geom->h);
        geom->w-=br;
    }else if(line==GR_BORDERLINE_BOTTOM && geom->w>0 && br>0){
        XSetForeground(ioncore_g.dpy, gc, brc);
        XSetBackground(ioncore_g.dpy, gc, brc);
        XFillRectangle(ioncore_g.dpy, win, gc, geom->x, geom->y+geom->h-br, geom->w, br);
        geom->h-=br;
    }
}


void debrush_do_draw_borderline(DEBrush *brush, WRectangle geom,
                                DEColourGroup *cg, GrBorderLine line)
{
    DEBorder *bd=&(brush->d->border);
    GC gc=brush->d->normal_gc;
    Window win=brush->win;
    
    switch(bd->style){
    case DEBORDER_RIDGE:
        draw_borderline(win, gc, &geom, bd->hl, bd->sh, cg->hl, cg->sh, line);
    case DEBORDER_INLAID:
        draw_borderline(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad, line);
        draw_borderline(win, gc, &geom, bd->sh, bd->hl, cg->sh, cg->hl, line);
        break;
    case DEBORDER_GROOVE:
        draw_borderline(win, gc, &geom, bd->sh, bd->hl, cg->sh, cg->hl, line);
        draw_borderline(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad, line);
        draw_borderline(win, gc, &geom, bd->hl, bd->sh, cg->hl, cg->sh, line);
        break;
    case DEBORDER_ELEVATED:
    default:
        draw_borderline(win, gc, &geom, bd->hl, bd->sh, cg->hl, cg->sh, line);
        draw_borderline(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad, line);
        break;
    }
}


void debrush_do_draw_padline(DEBrush *brush, WRectangle geom,
                             DEColourGroup *cg, GrBorderLine line)
{
    DEBorder *bd=&(brush->d->border);
    GC gc=brush->d->normal_gc;
    Window win=brush->win;
    
    draw_borderline(win, gc, &geom, bd->pad, bd->pad, cg->pad, cg->pad, line);
}


void debrush_draw_borderline(DEBrush *brush, const WRectangle *geom,
                             GrBorderLine line)
{
    DEColourGroup *cg=debrush_get_current_colour_group(brush);
    if(cg!=NULL)
        debrush_do_draw_borderline(brush, *geom, cg, line);
}


static void debrush_do_do_draw_border(DEBrush *brush, WRectangle geom, 
                                      DEColourGroup *cg)
{
    DEBorder *bd=&(brush->d->border);
    GC gc=brush->d->normal_gc;
    Window win=brush->win;
    
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


void debrush_do_draw_border(DEBrush *brush, WRectangle geom, 
                            DEColourGroup *cg)
{
    DEBorder *bd=&(brush->d->border);

    switch(bd->sides){
    case DEBORDER_ALL:
        debrush_do_do_draw_border(brush, geom, cg);
        break;
    case DEBORDER_TB:
        debrush_do_draw_padline(brush, geom, cg, GR_BORDERLINE_LEFT);
        debrush_do_draw_padline(brush, geom, cg, GR_BORDERLINE_RIGHT);
        debrush_do_draw_borderline(brush, geom, cg, GR_BORDERLINE_TOP);
        debrush_do_draw_borderline(brush, geom, cg, GR_BORDERLINE_BOTTOM);
        break;
    case DEBORDER_LR:
        debrush_do_draw_padline(brush, geom, cg, GR_BORDERLINE_TOP);
        debrush_do_draw_padline(brush, geom, cg, GR_BORDERLINE_BOTTOM);
        debrush_do_draw_borderline(brush, geom, cg, GR_BORDERLINE_LEFT);
        debrush_do_draw_borderline(brush, geom, cg, GR_BORDERLINE_RIGHT);
        break;
    }
}

    
void debrush_draw_border(DEBrush *brush, 
                         const WRectangle *geom)
{
    DEColourGroup *cg=debrush_get_current_colour_group(brush);
    if(cg!=NULL)
        debrush_do_draw_border(brush, *geom, cg);
}


/*}}}*/


/*{{{ Boxes */


static void copy_masked(DEBrush *brush, Drawable src, Drawable dst,
                        int src_x, int src_y, int w, int h,
                        int dst_x, int dst_y)
{
    
    GC copy_gc=brush->d->copy_gc;
    
    XSetClipMask(ioncore_g.dpy, copy_gc, src);
    XSetClipOrigin(ioncore_g.dpy, copy_gc, dst_x, dst_y);
    XCopyPlane(ioncore_g.dpy, src, dst, copy_gc, src_x, src_y, w, h,
               dst_x, dst_y, 1);
}



GR_DEFATTR(dragged);
GR_DEFATTR(tagged);
GR_DEFATTR(submenu);
GR_DEFATTR(numbered);
GR_DEFATTR(tabnumber);


static void ensure_attrs()
{
    GR_ALLOCATTR_BEGIN;
    GR_ALLOCATTR(dragged);
    GR_ALLOCATTR(tagged);
    GR_ALLOCATTR(submenu);
    GR_ALLOCATTR(numbered);
    GR_ALLOCATTR(tabnumber);
    GR_ALLOCATTR_END;
}


static int get_ty(const WRectangle *g, const GrBorderWidths *bdw, 
                  const GrFontExtents *fnte)
{
    return (g->y+bdw->top+fnte->baseline
            +(g->h-bdw->top-bdw->bottom-fnte->max_height)/2);
}


void debrush_tab_extras(DEBrush *brush, const WRectangle *g, 
                        DEColourGroup *cg, const GrBorderWidths *bdw,
                        const GrFontExtents *fnte,
                        const GrStyleSpec *a1, 
                        const GrStyleSpec *a2,
                        bool pre, int index)
{
    DEStyle *d=brush->d;
    GC tmp;
    /* Not thread-safe, but neither is the rest of the drawing code
     * with shared GC:s.
     */
    static bool swapped=FALSE;

    ensure_attrs();
    
    if(pre){
        if(!gr_stylespec_isset(a1, GR_ATTR(dragged)) &&
           !gr_stylespec_isset(a2, GR_ATTR(dragged))){
           
            tmp=d->normal_gc;
            d->normal_gc=d->stipple_gc;
            d->stipple_gc=tmp;
            swapped=TRUE;
            XClearArea(ioncore_g.dpy, brush->win, g->x, g->y, g->w, g->h, False);
        }
        return;
    }
    
    if(gr_stylespec_isset(a1, GR_ATTR(tagged)) ||
       gr_stylespec_isset(a2, GR_ATTR(tagged))){
       
        XSetForeground(ioncore_g.dpy, d->copy_gc, cg->fg);
            
        copy_masked(brush, d->tag_pixmap, brush->win, 0, 0,
                    d->tag_pixmap_w, d->tag_pixmap_h,
                    g->x+g->w-bdw->right-d->tag_pixmap_w, 
                    g->y+bdw->top);
    }
    
    if((gr_stylespec_isset(a1, GR_ATTR(numbered)) ||
        gr_stylespec_isset(a2, GR_ATTR(numbered))) && index>=0){
        
        DEColourGroup *cg;
        GrStyleSpec tmp;
        
        gr_stylespec_init(&tmp);
        gr_stylespec_append(&tmp, a2);
        gr_stylespec_set(&tmp, GR_ATTR(tabnumber));
        
        cg=debrush_get_colour_group2(brush, a1, &tmp);
        
        gr_stylespec_unalloc(&tmp);

        if(cg!=NULL){
            int ty, tx, l;
            char *s=NULL;
            
            libtu_asprintf(&s, "[%d]", index+1);
            
            if(s!=NULL){
                l=strlen(s);
                ty=get_ty(g, bdw, fnte);
                tx=g->x+g->w-bdw->right-debrush_get_text_width(brush, s, l);
                if(tx>=(int)bdw->left)
                    debrush_do_draw_string(brush, tx, ty, s, l, TRUE, cg);
                free(s);
            }
        }
    }

    if(swapped){
        tmp=d->normal_gc;
        d->normal_gc=d->stipple_gc;
        d->stipple_gc=tmp;
        swapped=FALSE;
    }
    /*if(MATCHES2("*-*-*-dragged", a1, a2)){
        XFillRectangle(ioncore_g.dpy, win, d->stipple_gc, 
                       g->x, g->y, g->w, g->h);
    }*/
}


void debrush_menuentry_extras(DEBrush *brush, 
                              const WRectangle *g, 
                              DEColourGroup *cg, 
                              const GrBorderWidths *bdw,
                              const GrFontExtents *fnte,
                              const GrStyleSpec *a1, 
                              const GrStyleSpec *a2, 
                              bool pre, int index)
{
    int tx, ty;

    if(pre)
        return;
    
    ensure_attrs();
    
    if(gr_stylespec_isset(a1, GR_ATTR(submenu)) ||
       gr_stylespec_isset(a2, GR_ATTR(submenu))){

        ty=get_ty(g, bdw, fnte);
        tx=g->x+g->w-bdw->right;

        debrush_do_draw_string(brush, tx, ty, DE_SUB_IND, DE_SUB_IND_LEN, 
                               FALSE, cg);
    }
}


void debrush_do_draw_box(DEBrush *brush, const WRectangle *geom, 
                         DEColourGroup *cg, bool needfill)
{
    GC gc=brush->d->normal_gc;
    
    if(TRUE/*needfill*/){
        XSetForeground(ioncore_g.dpy, gc, cg->bg);
        XFillRectangle(ioncore_g.dpy, brush->win, gc, geom->x, geom->y, 
                       geom->w, geom->h);
    }
    
    debrush_do_draw_border(brush, *geom, cg);
}


static void debrush_do_draw_textbox(DEBrush *brush, 
                                    const WRectangle *geom, 
                                    const char *text, 
                                    DEColourGroup *cg, 
                                    bool needfill,
                                    const GrStyleSpec *a1, 
                                    const GrStyleSpec *a2,
                                    int index)
{
    uint len;
    GrBorderWidths bdw;
    GrFontExtents fnte;
    uint tx, ty, tw;

    grbrush_get_border_widths(&(brush->grbrush), &bdw);
    grbrush_get_font_extents(&(brush->grbrush), &fnte);
    
    if(brush->extras_fn!=NULL)
        brush->extras_fn(brush, geom, cg, &bdw, &fnte, a1, a2, TRUE, index);
    
    debrush_do_draw_box(brush, geom, cg, needfill);
    
    do{ /*...while(0)*/
        if(text==NULL)
            break;
        
        len=strlen(text);
    
        if(len==0)
            break;
    
        if(brush->d->textalign!=DEALIGN_LEFT){
            tw=grbrush_get_text_width((GrBrush*)brush, text, len);
            
            if(brush->d->textalign==DEALIGN_CENTER)
                tx=geom->x+bdw.left+(geom->w-bdw.left-bdw.right-tw)/2;
            else
                tx=geom->x+geom->w-bdw.right-tw;
        }else{
            tx=geom->x+bdw.left;
        }
        
        ty=get_ty(geom, &bdw, &fnte);
        
        debrush_do_draw_string(brush, tx, ty, text, len, FALSE, cg);
    }while(0);
    
    if(brush->extras_fn!=NULL)
        brush->extras_fn(brush, geom, cg, &bdw, &fnte, a1, a2, FALSE, index);
}


void debrush_draw_textbox(DEBrush *brush, const WRectangle *geom, 
                          const char *text, bool needfill)
{
    GrStyleSpec *attr=debrush_get_current_attr(brush);
    DEColourGroup *cg=debrush_get_colour_group(brush, attr);
    
    if(cg!=NULL){
        debrush_do_draw_textbox(brush, geom, text, cg, needfill, 
                                attr, NULL, -1);
    }
}


void debrush_draw_textboxes(DEBrush *brush, const WRectangle *geom,
                            int n, const GrTextElem *elem, 
                            bool needfill)
{
    GrStyleSpec *common_attrib;
    WRectangle g=*geom;
    DEColourGroup *cg;
    GrBorderWidths bdw;
    int i;
    
    common_attrib=debrush_get_current_attr(brush);
    
    grbrush_get_border_widths(&(brush->grbrush), &bdw);
    
    for(i=0; ; i++){
        g.w=bdw.left+elem[i].iw+bdw.right;
        cg=debrush_get_colour_group2(brush, common_attrib, &elem[i].attr);
        
        if(cg!=NULL){
            debrush_do_draw_textbox(brush, &g, elem[i].text, cg, needfill,
                                    common_attrib, &elem[i].attr, i);
        }
        
        if(i==n-1)
            break;
        
        g.x+=g.w;
        if(bdw.spacing>0 && needfill){
            XClearArea(ioncore_g.dpy, brush->win, g.x, g.y,
                       brush->d->spacing, g.h, False);
        }
        g.x+=bdw.spacing;
    }
}


/*}}}*/


/*{{{ Misc. */

#define MAXSHAPE 16

void debrush_set_window_shape(DEBrush *brush, bool rough,
                              int n, const WRectangle *rects)
{
    XRectangle r[MAXSHAPE];
    int i;
    
    if(n>MAXSHAPE)
        n=MAXSHAPE;
    
    if(n==0){
	/* n==0 should clear the shape. As there's absolutely no
	 * documentation for XShape (as is typical of all sucky X
	 * extensions), I don't know how the shape should properly 
	 * be cleared. Thus we just use a huge rectangle.
	 */
	n=1;
	r[0].x=0;
	r[0].y=0;
	r[0].width=USHRT_MAX;
	r[0].height=USHRT_MAX;
    }else{
	for(i=0; i<n; i++){
	    r[i].x=rects[i].x;
	    r[i].y=rects[i].y;
	    r[i].width=rects[i].w;
	    r[i].height=rects[i].h;
	}
    }
    
    XShapeCombineRectangles(ioncore_g.dpy, brush->win,
                            ShapeBounding, 0, 0, r, n,
                            ShapeSet, Unsorted);
}


void debrush_enable_transparency(DEBrush *brush, GrTransparency mode)
{
    XSetWindowAttributes attr;
    ulong attrflags=0;

    if(mode==GR_TRANSPARENCY_DEFAULT)
        mode=brush->d->transparency_mode;
    
    if(mode==GR_TRANSPARENCY_YES){
        attrflags=CWBackPixmap;
        attr.background_pixmap=ParentRelative;
    }else{
        attrflags=CWBackPixel;
        attr.background_pixel=brush->d->cgrp.bg;
    }
    
    XChangeWindowAttributes(ioncore_g.dpy, brush->win, attrflags, &attr);
    XClearWindow(ioncore_g.dpy, brush->win);
}


void debrush_fill_area(DEBrush *brush, const WRectangle *geom)
{
    DEColourGroup *cg=debrush_get_current_colour_group(brush);
    GC gc=brush->d->normal_gc;

    if(cg==NULL)
        return;
    
    XSetForeground(ioncore_g.dpy, gc, cg->bg);
    XFillRectangle(ioncore_g.dpy, brush->win, gc, 
                   geom->x, geom->y, geom->w, geom->h);
}


void debrush_clear_area(DEBrush *brush, const WRectangle *geom)
{
    XClearArea(ioncore_g.dpy, brush->win,
               geom->x, geom->y, geom->w, geom->h, False);
}


/*}}}*/


/*{{{ Clipping rectangles */

/* Should actually set the clipping rectangle for all GC:s and use 
 * window-specific GC:s to do this correctly...
 */

static void debrush_set_clipping_rectangle(DEBrush *brush, 
                                           const WRectangle *geom)
{
    XRectangle rect;
    
    assert(!brush->clip_set);
    
    rect.x=geom->x;
    rect.y=geom->y;
    rect.width=geom->w;
    rect.height=geom->h;
    
    XSetClipRectangles(ioncore_g.dpy, brush->d->normal_gc,
                       0, 0, &rect, 1, Unsorted);
    brush->clip_set=TRUE;
}


static void debrush_clear_clipping_rectangle(DEBrush *brush)
{
    if(brush->clip_set){
        XSetClipMask(ioncore_g.dpy, brush->d->normal_gc, None);
        brush->clip_set=FALSE;
    }
}


/*}}}*/


/*{{{ debrush_begin/end */


void debrush_begin(DEBrush *brush, const WRectangle *geom, int flags)
{
    
    if(flags&GRBRUSH_AMEND)
        flags|=GRBRUSH_NO_CLEAR_OK;
    
    if(!(flags&GRBRUSH_KEEP_ATTR))
        debrush_init_attr(brush, NULL);
    
    if(!(flags&GRBRUSH_NO_CLEAR_OK))
        debrush_clear_area(brush, geom);
    
    if(flags&GRBRUSH_NEED_CLIP)
        debrush_set_clipping_rectangle(brush, geom);
}


void debrush_end(DEBrush *brush)
{
    debrush_clear_clipping_rectangle(brush);
}


/*}}}*/

