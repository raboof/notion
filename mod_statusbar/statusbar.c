/*
 * ion/ioncore/statusbar.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/binding.h>
#include <ioncore/regbind.h>
#include <ioncore/event.h>
#include <ioncore/resize.h>
#include <ioncore/gr.h>

#include "statusbar.h"
#include "main.h"
#include "draw.h"


static void statusbar_set_strings(WStatusBar *sb, ExtlTab t);
static void statusbar_free_strings(WStatusBar *sb);
static void statusbar_update_natural_size(WStatusBar *p);


/*{{{ Init/deinit */


bool statusbar_init(WStatusBar *p, WWindow *parent, const WFitParams *fp)
{
    if(!window_init(&(p->wwin), parent, fp))
        return FALSE;

    p->brush=NULL;
    p->strings=NULL;
    p->nstrings=0;
    p->natural_w=1;
    p->natural_h=1;
    p->natural_w_tmpl=NULL;
    
    statusbar_updategr(p);

    if(p->brush==NULL){
        window_deinit(&(p->wwin));
        return FALSE;
    }
    
    XSelectInput(ioncore_g.dpy, p->wwin.win, IONCORE_EVENTMASK_NORMAL);

    region_add_bindmap((WRegion*)p, mod_statusbar_statusbar_bindmap);
    
    ((WRegion*)p)->flags|=REGION_SKIP_FOCUS;

    return TRUE;
}



WStatusBar *create_statusbar(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WStatusBar, statusbar, (p, parent, fp));
}


void statusbar_deinit(WStatusBar *p)
{
    statusbar_free_strings(p);
    
    if(p->brush!=NULL)
        grbrush_release(p->brush, p->wwin.win);
    
    if(p->natural_w_tmpl!=NULL)
        free(p->natural_w_tmpl);

    window_deinit(&(p->wwin));
}


/*}}}*/


/*{{{ Content stuff */


static GrTextElem *get_textelems(ExtlTab t, int *nret)
{
    int i, n=extl_table_get_n(t);
    GrTextElem *el;
    
    *nret=0;
    
    if(n<=0)
        return NULL;
    
    el=ALLOC_N(GrTextElem, n);
    
    if(el==NULL)
        return NULL;
    
    for(i=0; i<n; i++){
        ExtlTab tt;
        
        el[i].text=NULL;
        el[i].attr=NULL;
        el[i].iw=0;
        
        if(extl_table_geti_t(t, i+1, &tt)){
            extl_table_gets_s(tt, "text", &(el[i].text));
            extl_table_gets_s(tt, "attr", &(el[i].attr));
            extl_unref_table(tt);
        }
    }
    
    *nret=n;
    
    return el;
}
    

static void free_textelems(GrTextElem *el, int n)
{
    int i;
    
    for(i=0; i<n; i++){
        if(el[i].text!=NULL)
            free(el[i].text);
        if(el[i].attr!=NULL)
            free(el[i].attr);
    }
    
    free(el);
}


static void statusbar_set_strings(WStatusBar *sb, ExtlTab t)
{
    statusbar_free_strings(sb);
    
    sb->strings=get_textelems(t, &(sb->nstrings));
}


static void statusbar_free_strings(WStatusBar *sb)
{
    if(sb->strings!=NULL){
        free_textelems(sb->strings, sb->nstrings);
        sb->strings=NULL;
        sb->nstrings=0;
    }
}


/*EXTL_DOC
 * Set statusbar contents.
 */
EXTL_EXPORT_MEMBER
void statusbar_set_contents(WStatusBar *sb, ExtlTab t)
{
    statusbar_set_strings(sb, t);
    
    statusbar_update_natural_size(sb);
    
    window_draw(&sb->wwin, TRUE);
}


/*}}}*/



/*{{{ Size stuff */


static void statusbar_resize(WStatusBar *p)
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


static void statusbar_update_natural_size(WStatusBar *p)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    const char *str=(p->natural_w_tmpl!=NULL ? p->natural_w_tmpl : "");
    
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
void statusbar_set_natural_w(WStatusBar *p, const char *str)
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
    
    statusbar_update_natural_size(p);
    statusbar_resize(p);
}


void statusbar_size_hints(WStatusBar *p, XSizeHints *h)
{
    h->flags=PMaxSize|PMinSize;
    h->min_width=p->natural_w;
    h->min_height=p->natural_h;
    h->max_width=p->natural_w;
    h->max_height=p->natural_h;
}


/*}}}*/


/*{{{ Updategr */


void statusbar_updategr(WStatusBar *p)
{
    GrBrush *nbrush;
    
    nbrush=gr_get_brush(region_rootwin_of((WRegion*)p),
                        p->wwin.win, "stdisp-statusbar");
    if(nbrush==NULL)
        return;
    
    if(p->brush!=NULL)
        grbrush_release(p->brush, p->wwin.win);
    
    p->brush=nbrush;
    
    statusbar_update_natural_size(p);
    
    window_draw(&(p->wwin), TRUE);
}


/*}}}*/


/*{{{ Load */


WRegion *statusbar_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    return (WRegion*)create_statusbar(par, fp);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab statusbar_dynfuntab[]={
    {window_draw, statusbar_draw},
    {region_updategr, statusbar_updategr},
    {region_size_hints, statusbar_size_hints},
    END_DYNFUNTAB
};


IMPLCLASS(WStatusBar, WWindow, statusbar_deinit, statusbar_dynfuntab);

    
/*}}}*/

