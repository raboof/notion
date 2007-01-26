/*
 * ion/ioncore/infowin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include "common.h"
#include "global.h"
#include "window.h"
#include "infowin.h"
#include "resize.h"
#include "gr.h"
#include "event.h"


/*{{{ Init/deinit */


bool infowin_init(WInfoWin *p, WWindow *parent, const WFitParams *fp,
                  const char *style)
{
    XSetWindowAttributes attr;
    
    if(!window_init(&(p->wwin), parent, fp))
        return FALSE;
    
    p->buffer=ALLOC_N(char, INFOWIN_BUFFER_LEN);
    if(p->buffer==NULL)
        goto fail;
    p->buffer[0]='\0';
    
    if(style==NULL)
        p->style=scopy("*");
    else
        p->style=scopy(style);
    if(p->style==NULL)
        goto fail2;
    
    p->brush=NULL;
    
    gr_stylespec_init(&p->attr);
    
    infowin_updategr(p);
    
    if(p->brush==NULL)
        goto fail3;
    
    p->wwin.region.flags|=REGION_SKIP_FOCUS;
    
    /* Enable save unders */
    attr.save_under=True;
    XChangeWindowAttributes(ioncore_g.dpy, p->wwin.win, CWSaveUnder, &attr);
    
    window_select_input(&(p->wwin), IONCORE_EVENTMASK_NORMAL);

    return TRUE;

fail3:
    gr_stylespec_unalloc(&p->attr);
    free(p->style);
fail2:
    free(p->buffer);
fail:    
    window_deinit(&(p->wwin));
    return FALSE;
}


WInfoWin *create_infowin(WWindow *parent, const WFitParams *fp,
                         const char *style)
{
    CREATEOBJ_IMPL(WInfoWin, infowin, (p, parent, fp, style));
}


void infowin_deinit(WInfoWin *p)
{
    if(p->buffer!=NULL){
        free(p->buffer);
        p->buffer=NULL;
    }

    if(p->style!=NULL){
        free(p->style);
        p->style=NULL;
    }
    
    if(p->brush!=NULL){
        grbrush_release(p->brush);
        p->brush=NULL;
    }
    
    gr_stylespec_unalloc(&p->attr);
    
    window_deinit(&(p->wwin));
}


/*}}}*/


/*{{{ Drawing and geometry */


void infowin_draw(WInfoWin *p, bool complete)
{
    WRectangle g;
    
    if(p->brush==NULL)
        return;
    
    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(p).w;
    g.h=REGION_GEOM(p).h;

    grbrush_begin(p->brush, &g, GRBRUSH_NO_CLEAR_OK);
    grbrush_init_attr(p->brush, &p->attr);
    grbrush_draw_textbox(p->brush, &g, p->buffer, TRUE);
    grbrush_end(p->brush);
}


void infowin_updategr(WInfoWin *p)
{
    GrBrush *nbrush;
    
    assert(p->style!=NULL);
    
    nbrush=gr_get_brush(p->wwin.win, 
                        region_rootwin_of((WRegion*)p),
                        p->style);
    if(nbrush==NULL)
        return;
    
    if(p->brush!=NULL)
        grbrush_release(p->brush);
    
    p->brush=nbrush;
    
    window_draw(&(p->wwin), TRUE);
}



/*}}}*/


/*{{{ Content-setting */


GrStyleSpec *infowin_stylespec(WInfoWin *p)
{
    return &p->attr;
}


static void infowin_do_set_text(WInfoWin *p, const char *str)
{
    strncpy(INFOWIN_BUFFER(p), str, INFOWIN_BUFFER_LEN);
    INFOWIN_BUFFER(p)[INFOWIN_BUFFER_LEN-1]='\0';
}


static void infowin_resize(WInfoWin *p)
{
    WRQGeomParams rq=RQGEOMPARAMS_INIT;
    const char *str=INFOWIN_BUFFER(p);
    GrBorderWidths bdw;
    GrFontExtents fnte;
    
    rq.flags=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    
    rq.geom.x=REGION_GEOM(p).x;
    rq.geom.y=REGION_GEOM(p).y;
    
    grbrush_get_border_widths(p->brush, &bdw);
    grbrush_get_font_extents(p->brush, &fnte);
        
    rq.geom.w=bdw.left+bdw.right;
    rq.geom.w+=grbrush_get_text_width(p->brush, str, strlen(str));
    rq.geom.h=fnte.max_height+bdw.top+bdw.bottom;

    if(rectangle_compare(&rq.geom, &REGION_GEOM(p))!=RECTANGLE_SAME)
        region_rqgeom((WRegion*)p, &rq, NULL);
}


/*EXTL_DOC
 * Set contents of the info window.
 */
EXTL_EXPORT_MEMBER
void infowin_set_text(WInfoWin *p, const char *str)
{
    infowin_do_set_text(p, str);

    infowin_resize(p);
    
    /* sometimes unnecessary */
    window_draw((WWindow*)p, TRUE);
}


/*}}}*/


/*{{{ Load */


WRegion *infowin_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    char *style=NULL, *text=NULL;
    WInfoWin *p;
    
    extl_table_gets_s(tab, "style", &style);
    
    p=create_infowin(par, fp, style);
    
    free(style);
    
    if(p==NULL)
        return NULL;

    if(extl_table_gets_s(tab, "text", &text)){
        infowin_do_set_text(p, text);
        free(text);
    }
    
    return (WRegion*)p;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab infowin_dynfuntab[]={
    {window_draw, infowin_draw},
    {region_updategr, infowin_updategr},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WInfoWin, WWindow, infowin_deinit, infowin_dynfuntab);

    
/*}}}*/

