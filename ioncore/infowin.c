/*
 * ion/ioncore/infowin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
    p->attr=NULL;
    p->natural_w=1;
    p->natural_h=1;
    p->natural_w_tmpl=NULL;
    p->drawn=FALSE;
    
    infowin_updategr(p);
    
    if(p->brush==NULL)
        goto fail3;
    
    /* Enable save unders */
    attr.save_under=True;
    XChangeWindowAttributes(ioncore_g.dpy, p->wwin.win, CWSaveUnder, &attr);
    
    XSelectInput(ioncore_g.dpy, p->wwin.win, IONCORE_EVENTMASK_INPUT);
    
    return TRUE;

fail3:    
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
    if(p->buffer!=NULL)
        free(p->buffer);
    
    if(p->attr!=NULL)
        free(p->attr);
    
    if(p->style!=NULL)
        free(p->style);
    
    if(p->natural_w_tmpl!=NULL)
        free(p->natural_w_tmpl);
    
    if(p->brush!=NULL)
        grbrush_release(p->brush, p->wwin.win);
    
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
    
    grbrush_draw_textbox(p->brush, p->wwin.win, &g, p->buffer, p->attr,
                         complete);
    
    p->drawn=TRUE;
}


static void infowin_resize(WInfoWin *p)
{
    int rqflags=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    WRectangle g;
    
    g.w=p->natural_w;
    g.h=p->natural_h;
    g.x=REGION_GEOM(p).x;
    g.y=REGION_GEOM(p).y;
    
    if(g.w!=REGION_GEOM(p).w || g.h!=REGION_GEOM(p).h)
        region_rqgeom((WRegion*)p, rqflags, &g, NULL);
}


static void infowin_update_natural_size(WInfoWin *p)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    const char *str=(p->natural_w_tmpl!=NULL 
                     ? p->natural_w_tmpl 
                     : INFOWIN_BUFFER(p));
    
    grbrush_get_border_widths(p->brush, &bdw);
    grbrush_get_font_extents(p->brush, &fnte);
    
    p->natural_w=bdw.left+bdw.right;
    p->natural_w+=grbrush_get_text_width(p->brush, str, strlen(str));
    p->natural_h=fnte.max_height+bdw.top+bdw.bottom;
}


/*EXTL_DOC
 * Set natural width template string.
 */
EXTL_EXPORT_MEMBER
void infowin_set_natural_w(WInfoWin *p, const char *str)
{
    GrBorderWidths bdw;
    char *s=NULL;
    
    if(str!=NULL){
        s=scopy(str);
        if(s==NULL)
            return;
    }

    if(p->natural_w_tmpl!=NULL)
        free(p->natural_w_tmpl);
    
    p->natural_w_tmpl=s;
    
    infowin_update_natural_size(p);
    infowin_resize(p);
}


void infowin_updategr(WInfoWin *p)
{
    GrBrush *nbrush;
    
    assert(p->style!=NULL);
    
    nbrush=gr_get_brush(region_rootwin_of((WRegion*)p),
                        p->wwin.win, p->style);
    if(nbrush==NULL)
        return;
    
    if(p->brush!=NULL)
        grbrush_release(p->brush, p->wwin.win);
    
    p->brush=nbrush;
    
    infowin_update_natural_size(p);
    
    window_draw(&(p->wwin), TRUE);
}



/*}}}*/


/*{{{ Content-setting */


bool infowin_set_attr2(WInfoWin *p, const char *attr1, const char *attr2)
{
    char *p2=NULL;
    
    if(attr1!=NULL){
        if(attr2==NULL)
            p2=scopy(attr1);
        else
            libtu_asprintf(&p2, "%s-%s", attr1, attr2);
        if(p2==NULL)
            return FALSE;
    }
    
    if(p->attr)
        free(p->attr);
    
    p->attr=p2;
    
    return TRUE;
}


static void infowin_do_set_text(WInfoWin *p, const char *str)
{
    strncpy(INFOWIN_BUFFER(p), str, INFOWIN_BUFFER_LEN);
    INFOWIN_BUFFER(p)[INFOWIN_BUFFER_LEN-1]='\0';
    
    infowin_update_natural_size(p);
}


/*EXTL_DOC
 * Set contents of the info window.
 */
EXTL_EXPORT_MEMBER
void infowin_set_text(WInfoWin *p, const char *str)
{
    infowin_do_set_text(p, str);

    p->drawn=FALSE;
    
    if(p->natural_w_tmpl==NULL)
        infowin_resize(p);

    if(!p->drawn)
        window_draw((WWindow*)p, TRUE);
}


/*}}}*/


/*{{{ Misc. */


void infowin_size_hints(WInfoWin *p, XSizeHints *h)
{
    h->flags=PMaxSize|PMinSize;
    h->min_width=p->natural_w;
    h->min_height=p->natural_h;
    h->max_width=p->natural_w;
    h->max_height=p->natural_h;
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


/*{{{ Dynamic function table and class implementation */


static DynFunTab infowin_dynfuntab[]={
    {window_draw, infowin_draw},
    {region_size_hints, infowin_size_hints},
    {region_updategr, infowin_updategr},
    END_DYNFUNTAB
};


IMPLCLASS(WInfoWin, WWindow, infowin_deinit, infowin_dynfuntab);

    
/*}}}*/

