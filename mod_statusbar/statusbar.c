/*
 * ion/mod_statusbar/statusbar.c
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
#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/binding.h>
#include <ioncore/regbind.h>
#include <ioncore/event.h>
#include <ioncore/resize.h>
#include <ioncore/gr.h>
#include <ioncore/names.h>
#include <ioncore/strings.h>

#include "statusbar.h"
#include "main.h"
#include "draw.h"


static void statusbar_set_elems(WStatusBar *sb, ExtlTab t);
static void statusbar_free_elems(WStatusBar *sb);
static void statusbar_update_natural_size(WStatusBar *p);


/*{{{ Init/deinit */


bool statusbar_init(WStatusBar *p, WWindow *parent, const WFitParams *fp)
{
    if(!window_init(&(p->wwin), parent, fp))
        return FALSE;

    region_register((WRegion*)p);

    p->brush=NULL;
    p->elems=NULL;
    p->nelems=0;
    p->natural_w=1;
    p->natural_h=1;
    
    statusbar_updategr(p);

    if(p->brush==NULL){
        window_deinit(&(p->wwin));
        return FALSE;
    }
    
    window_select_input(&(p->wwin), IONCORE_EVENTMASK_NORMAL);

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
    statusbar_free_elems(p);
    
    if(p->brush!=NULL)
        grbrush_release(p->brush);
    
    window_deinit(&(p->wwin));
}


/*}}}*/


/*{{{ Content stuff */


static WSBElem *get_sbelems(ExtlTab t, int *nret)
{
    int i, n=extl_table_get_n(t);
    WSBElem *el;
    
    *nret=0;
    
    if(n<=0)
        return NULL;
    
    el=ALLOC_N(WSBElem, n);
    
    if(el==NULL)
        return NULL;
    
    for(i=0; i<n; i++){
        ExtlTab tt;
        
        el[i].type=WSBELEM_NONE;
        el[i].meter=NULL;
        el[i].text_w=0;
        el[i].text=NULL;
        el[i].max_w=0;
        el[i].tmpl=NULL;
        el[i].attr=NULL;
        el[i].stretch=0;
        el[i].align=WSBELEM_ALIGN_CENTER;
        el[i].zeropad=0;

        if(extl_table_geti_t(t, i+1, &tt)){
            if(extl_table_gets_i(tt, "type", &(el[i].type))){
                if(el[i].type==WSBELEM_TEXT || el[i].type==WSBELEM_STRETCH){
                    extl_table_gets_s(tt, "text", &(el[i].text));
                }else if(el[i].type==WSBELEM_METER){
                    extl_table_gets_s(tt, "meter", &(el[i].meter));
                    extl_table_gets_s(tt, "tmpl", &(el[i].tmpl));
                    extl_table_gets_i(tt, "align", &(el[i].align));
                    extl_table_gets_i(tt, "zeropad", &(el[i].zeropad));
                    el[i].zeropad=maxof(el[i].zeropad, 0);
                }
            }
            extl_unref_table(tt);
        }
    }
    
    *nret=n;
    
    return el;
}
    

static void free_sbelems(WSBElem *el, int n)
{
    int i;
    
    for(i=0; i<n; i++){
        if(el[i].text!=NULL)
            free(el[i].text);
        if(el[i].meter!=NULL)
            free(el[i].meter);
        if(el[i].tmpl!=NULL)
            free(el[i].tmpl);
        if(el[i].attr!=NULL)
            free(el[i].attr);
    }
    
    free(el);
}


static void statusbar_set_elems(WStatusBar *sb, ExtlTab t)
{
    statusbar_free_elems(sb);
    
    sb->elems=get_sbelems(t, &(sb->nelems));
}


static void statusbar_free_elems(WStatusBar *sb)
{
    if(sb->elems!=NULL){
        free_sbelems(sb->elems, sb->nelems);
        sb->elems=NULL;
        sb->nelems=0;
    }
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


static void calc_elem_w(WSBElem *el, GrBrush *brush)
{
    const char *str;

    if(el->type==WSBELEM_METER){
        str=(el->text!=NULL ? el->text : STATUSBAR_NX_STR);
        el->text_w=grbrush_get_text_width(brush, str, strlen(str));
        str=el->tmpl;
        el->max_w=maxof((str!=NULL
                         ? grbrush_get_text_width(brush, str, strlen(str))
                         : 0),
                        el->text_w);
    }else{
        str=el->text;
        el->text_w=(str!=NULL
                    ? grbrush_get_text_width(brush, str, strlen(str))
                    : 0);
        el->max_w=el->text_w;
    }
}


static void statusbar_do_update_natural_size(WStatusBar *p)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    int totw=0;
    int i;

    grbrush_get_border_widths(p->brush, &bdw);
    grbrush_get_font_extents(p->brush, &fnte);
    
    for(i=0; i<p->nelems; i++)
        totw+=p->elems[i].max_w;
    
    p->natural_w=bdw.left+totw+bdw.right;
    p->natural_h=fnte.max_height+bdw.top+bdw.bottom;
}

static void statusbar_update_natural_size(WStatusBar *p)
{
    int i;
    
    for(i=0; i<p->nelems; i++)
        calc_elem_w(&(p->elems[i]), p->brush);
    
    statusbar_do_update_natural_size(p);
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


/*{{{ Exports */


/*EXTL_DOC
 * Set statusbar template.
 */
EXTL_EXPORT_MEMBER
void statusbar_set_template(WStatusBar *sb, ExtlTab t)
{
    statusbar_set_elems(sb, t);
    
    statusbar_update_natural_size(sb);
    
    statusbar_resize(sb);
}


static void reset_stretch(WStatusBar *sb)
{
    int i;
    
    for(i=0; i<sb->nelems; i++)
        sb->elems[i].stretch=0;
}


static void positive_stretch(WStatusBar *sb)
{
    int i;
    
    for(i=0; i<sb->nelems; i++)
        sb->elems[i].stretch=maxof(0, sb->elems[i].stretch);
}


static void spread_stretch(WStatusBar *sb)
{
    int i, j, k;
    int diff;
    WSBElem *el, *lel, *rel;
    const char *str;
    
    for(i=0; i<sb->nelems; i++){
        el=&(sb->elems[i]);

        if(el->type!=WSBELEM_METER)
            continue;
        
        diff=el->max_w-el->text_w;
        
        lel=NULL;
        rel=NULL;
        
        if(el->align!=WSBELEM_ALIGN_RIGHT){
            for(j=i+1; j<sb->nelems; j++){
                if(sb->elems[j].type==WSBELEM_STRETCH){
                    rel=&(sb->elems[j]);
                    break;
                }
            }
        }
        
        if(el->align!=WSBELEM_ALIGN_LEFT){
            for(k=i-1; k>=0; k--){
                if(sb->elems[k].type==WSBELEM_STRETCH){
                    lel=&(sb->elems[k]);
                    break;
                }
            }
        }
        
        if(rel!=NULL && lel!=NULL){
            int l=diff/2;
            int r=diff-l;
            lel->stretch+=l;
            rel->stretch+=r;
        }else if(lel!=NULL){
            lel->stretch+=diff;
        }else if(rel!=NULL){
            rel->stretch+=diff;
        }
    }
}



/*EXTL_DOC
 * Set statusbar template.
 */
EXTL_EXPORT_MEMBER
void statusbar_update(WStatusBar *sb, ExtlTab t)
{
    int i;
    WSBElem *el;
    bool grow=FALSE;
    
    if(sb->brush==NULL)
        return;
    
    for(i=0; i<sb->nelems; i++){
        el=&(sb->elems[i]);
        
        if(el->type!=WSBELEM_METER)
            continue;
        
        if(el->text!=NULL){
            free(el->text);
            el->text=NULL;
        }

        if(el->attr!=NULL){
            free(el->attr);
            el->attr=NULL;
        }
        
        if(el->meter!=NULL){
            const char *str;
            char *attrnm;
            
            extl_table_gets_s(t, el->meter, &(el->text));
            
            if(el->text==NULL){
                str=STATUSBAR_NX_STR;
            }else{
                /* Zero-pad */
                int l=strlen(el->text);
                int ml=str_len(el->text);
                int diff=el->zeropad-ml;
                if(diff>0){
                    char *tmp=ALLOC_N(char, l+diff+1);
                    if(tmp!=NULL){
                        memset(tmp, '0', diff);
                        memcpy(tmp+diff, el->text, l+1);
                        free(el->text);
                        el->text=tmp;
                    }
                }
                str=el->text;
            }
            
            el->text_w=grbrush_get_text_width(sb->brush, str, strlen(str));
            
            if(el->text_w>el->max_w){
                el->max_w=el->text_w;
                grow=TRUE;
            }
            
            attrnm=scat(el->meter, "_hint");
            if(attrnm!=NULL){
                extl_table_gets_s(t, attrnm, &(el->attr));
                free(attrnm);
            }
        }
    }

    reset_stretch(sb);
    spread_stretch(sb);
    positive_stretch(sb);
    
    if(grow){
        statusbar_do_update_natural_size(sb);
        statusbar_resize(sb);
    }
    
    window_draw((WWindow*)sb, FALSE);
}


/*}}}*/


/*{{{ Updategr */


void statusbar_updategr(WStatusBar *p)
{
    GrBrush *nbrush;
    
    nbrush=gr_get_brush(p->wwin.win, region_rootwin_of((WRegion*)p),
                        "stdisp-statusbar");
    if(nbrush==NULL)
        return;
    
    if(p->brush!=NULL)
        grbrush_release(p->brush);
    
    p->brush=nbrush;
    
    statusbar_update_natural_size(p);
    
    reset_stretch(p);
    spread_stretch(p);
    positive_stretch(p);
    
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


EXTL_EXPORT
IMPLCLASS(WStatusBar, WWindow, statusbar_deinit, statusbar_dynfuntab);

    
/*}}}*/

