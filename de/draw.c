/*
 * ion/de/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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


static DEColourGroup *destyle_get_colour_group2(DEStyle *style,
                                                const char *attr_p1,
                                                const char *attr_p2)
{
    int i, score, maxscore=0;
    DEColourGroup *maxg=&(style->cgrp);
    
    while(style!=NULL){
        for(i=0; i<style->n_extra_cgrps; i++){
            score=gr_stylespec_score2(style->extra_cgrps[i].spec,
                                      attr_p1, attr_p2);
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
                                         const char *attr_p1,
                                         const char *attr_p2)
{
    return destyle_get_colour_group2(brush->d, attr_p1, attr_p2);
}


DEColourGroup *debrush_get_colour_group(DEBrush *brush, const char *attr)
{
    return destyle_get_colour_group2(brush->d, attr, NULL);
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
        points[0].x=x+w-i;        points[0].y=y+b;
        points[1].x=x+w-i;        points[1].y=y+h-i;
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


void debrush_do_draw_border(DEBrush *brush, Window win, WRectangle geom,
                            DEColourGroup *cg)
{
    DEBorder *bd=&(brush->d->border);
    GC gc=brush->d->normal_gc;
    
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


void debrush_tab_extras(DEBrush *brush, Window win,
                        const WRectangle *g, DEColourGroup *cg,
                        GrBorderWidths *bdw,
                        GrFontExtents *fnte,
                        const char *a1, const char *a2,
                        bool pre)
{
    DEStyle *d=brush->d;
    GC tmp;
    /* Not thread-safe, but neither is the rest of the drawing code
     * with shared GC:s.
     */
    static bool swapped=FALSE;
    
    if(pre){
        if(!MATCHES2("*-*-*-dragged", a1, a2))
            return;
        
        tmp=d->normal_gc;
        d->normal_gc=d->stipple_gc;
        d->stipple_gc=tmp;
        swapped=TRUE;
        XClearArea(ioncore_g.dpy, win, g->x, g->y, g->w, g->h, False);
        return;
    }
    
    if(MATCHES2("*-*-tagged", a1, a2)){
        XSetForeground(ioncore_g.dpy, d->copy_gc, cg->fg);
            
        copy_masked(brush, d->tag_pixmap, win, 0, 0,
                    d->tag_pixmap_w, d->tag_pixmap_h,
                    g->x+g->w-bdw->right-d->tag_pixmap_w, 
                    g->y+bdw->top);
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


void debrush_menuentry_extras(DEBrush *brush, Window win,
                              const WRectangle *g, DEColourGroup *cg,
                              GrBorderWidths *bdw,
                              GrFontExtents *fnte,
                              const char *a1, const char *a2, 
                              bool pre)
{
    int tx, ty;

    if(pre)
        return;
    
    if(!MATCHES2("*-*-submenu", a1, a2))
        return;
        
    ty=(g->y+bdw->top+fnte->baseline
        +(g->h-bdw->top-bdw->bottom-fnte->max_height)/2);
    tx=g->x+g->w-bdw->right;

    debrush_do_draw_string(brush, win, tx, ty, DE_SUB_IND,
                           DE_SUB_IND_LEN, FALSE, cg);
}


void debrush_do_draw_box(DEBrush *brush, Window win,
                         const WRectangle *geom, DEColourGroup *cg,
                         bool needfill)
{
    GC gc=brush->d->normal_gc;
    
    if(TRUE/*needfill*/){
        XSetForeground(ioncore_g.dpy, gc, cg->bg);
        XFillRectangle(ioncore_g.dpy, win, gc, geom->x, geom->y, 
                       geom->w, geom->h);
    }
    
    debrush_do_draw_border(brush, win, *geom, cg);
}


static void debrush_do_draw_textbox(DEBrush *brush, Window win, 
                                    const WRectangle *geom, const char *text, 
                                    DEColourGroup *cg, bool needfill,
                                    const char *a1, const char *a2)
{
    uint len;
    GrBorderWidths bdw;
    GrFontExtents fnte;
    uint tx, ty, tw;

    if(brush->extras_fn!=NULL)
        brush->extras_fn(brush, win, geom, cg, &bdw, &fnte, a1, a2, TRUE);
    
    debrush_do_draw_box(brush, win, geom, cg, needfill);
    
    do{ /*...while(0)*/
        if(text==NULL)
            break;
        
        len=strlen(text);
    
        if(len==0)
            break;
    
        grbrush_get_border_widths(&(brush->grbrush), &bdw);
        grbrush_get_font_extents(&(brush->grbrush), &fnte);
        
        if(brush->d->textalign!=DEALIGN_LEFT){
            tw=grbrush_get_text_width((GrBrush*)brush, text, len);
            
            if(brush->d->textalign==DEALIGN_CENTER)
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
    
    if(brush->extras_fn!=NULL)
        brush->extras_fn(brush, win, geom, cg, &bdw, &fnte, a1, a2, FALSE);
}


void debrush_draw_textbox(DEBrush *brush, Window win, 
                          const WRectangle *geom, const char *text, 
                          const char *attr, bool needfill)
{
    DEColourGroup *cg=debrush_get_colour_group(brush, attr);
    if(cg!=NULL){
        debrush_do_draw_textbox(brush, win, geom, text, cg, needfill, 
                                attr, NULL);
    }
}


void debrush_draw_textboxes(DEBrush *brush, Window win, 
                            const WRectangle *geom,
                            int n, const GrTextElem *elem, 
                            bool needfill, const char *common_attrib)
{
    WRectangle g=*geom;
    DEColourGroup *cg;
    GrBorderWidths bdw;
    int i;
    
    grbrush_get_border_widths(&(brush->grbrush), &bdw);
    
    for(i=0; i<n; i++){
        g.w=bdw.left+elem[i].iw+bdw.right;
        cg=debrush_get_colour_group2(brush, common_attrib, elem[i].attr);
        
        if(cg!=NULL){
            debrush_do_draw_textbox(brush, win, &g, elem[i].text, cg, needfill,
                                    common_attrib, elem[i].attr);
        }
        
        g.x+=g.w;
        if(bdw.spacing>0 && needfill){
            XClearArea(ioncore_g.dpy, win, g.x, g.y,
                       brush->d->spacing, g.h, False);
        }
        g.x+=bdw.spacing;
    }
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
    
    XSetClipRectangles(ioncore_g.dpy, brush->d->normal_gc,
                       0, 0, &rect, 1, Unsorted);
}


void debrush_clear_clipping_rectangle(DEBrush *brush, Window unused)
{
    XSetClipMask(ioncore_g.dpy, brush->d->normal_gc, None);
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
    
    XShapeCombineRectangles(ioncore_g.dpy, win, ShapeBounding, 0, 0, r, n,
                            ShapeSet, YXBanded);
}


void debrush_enable_transparency(DEBrush *brush, Window win, 
                                 GrTransparency mode)
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
    
    XChangeWindowAttributes(ioncore_g.dpy, win, attrflags, &attr);
    XClearWindow(ioncore_g.dpy, win);
}


void debrush_fill_area(DEBrush *brush, Window win, const WRectangle *geom,
                       const char *attr)
{
    DEColourGroup *cg=debrush_get_colour_group(brush, attr);
    GC gc=brush->d->normal_gc;

    if(cg==NULL)
        return;
    
    XSetForeground(ioncore_g.dpy, gc, cg->bg);
    XFillRectangle(ioncore_g.dpy, win, gc, geom->x, geom->y, geom->w, geom->h);
}


void debrush_clear_area(DEBrush *brush, Window win, const WRectangle *geom)
{
    XClearArea(ioncore_g.dpy, win, geom->x, geom->y, geom->w, geom->h, False);
}


/*}}}*/


