/*
 * ion/mod_query/listing.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <limits.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/gr.h>
#include <ioncore/strings.h>
#include "listing.h"


#define COL_SPACING 16
#define CONT_INDENT "xx"
#define CONT_INDENT_LEN 2
#define ITEMROWS(L, R) ((L)->itemrows==NULL ? 1 : (L)->itemrows[R])


static int strings_maxw(GrBrush *brush, char **strs, int nstrs)
{
    int maxw=0, w, i;
    
    for(i=0; i<nstrs; i++){
        w=grbrush_get_text_width(brush, strs[i], strlen(strs[i]));
        if(w>maxw)
            maxw=w;
    }
    
    return maxw;
}


static int getbeg(GrBrush *brush, int maxw, char *str, int l, int *wret)
{
    int n=0, nprev=0, w;
    GrFontExtents fnte;
    
    if(maxw<=0){
        *wret=0;
        return 0;
    }
    
    grbrush_get_font_extents(brush, &fnte);
    
    if(fnte.max_width!=0){
        /* Do an initial skip. */
        int n2=maxw/fnte.max_width;
    
        n=0;
        while(n2>0){
            n+=str_nextoff(str, n);
            n2--;
        }
    }
    
    w=grbrush_get_text_width(brush, str, n);
    nprev=n;
    *wret=w;

    while(w<=maxw){
        *wret=w;
        nprev=n;
        n+=str_nextoff(str, n);
        if(n==nprev)
            break;
        w=grbrush_get_text_width(brush, str, n);
    }
    
    return nprev;
}


static int string_nrows(GrBrush *brush, int maxw, char *str)
{
    int wrapw=grbrush_get_text_width(brush, "\\", 1);
    int ciw=grbrush_get_text_width(brush, CONT_INDENT, CONT_INDENT_LEN);
    int l2, l=strlen(str);
    int w;
    int nr=1;
    
    if(maxw<=0)
        return 1;
    
    while(1){
        w=grbrush_get_text_width(brush, str, l);
        if(w<maxw)
            break;
        l2=getbeg(brush, maxw-wrapw, str, l, &w);
        if(l2==0)
            break;
        if(nr==1)
            maxw-=ciw;
        nr++;
        l-=l2;
        str+=l2;
    }
    
    return nr;
}


static void draw_multirow(GrBrush *brush, Window win, int x, int y,
                          int maxw, int h, char *str, const char *style)
{
    int wrapw=grbrush_get_text_width(brush, "\\", 1);
    int ciw=grbrush_get_text_width(brush, CONT_INDENT, CONT_INDENT_LEN);
    int l2, l=strlen(str);
    int w;
    int nr=1;
    
    if(maxw<=0)
        return;

    while(1){
        w=grbrush_get_text_width(brush, str, l);
        if(w<maxw)
            break;
        l2=getbeg(brush, maxw-wrapw, str, l, &w);
        if(l2==0)
            break;
        grbrush_draw_string(brush, win, x, y, str, l2, TRUE, style);
        grbrush_draw_string(brush, win, x+w, y, "\\", 1, TRUE, style);
        if(nr==1){
            maxw-=ciw;
            x+=ciw;
        }
        nr++;
        y+=h;
        l-=l2;
        str+=l2;
    }
    
    grbrush_draw_string(brush, win, x, y, str, l, TRUE, style);
}

                          
static int col_fit(int w, int itemw, int spacing)
{
    int ncol=1;
    int tmp=w-itemw;
    itemw+=spacing;
    
    if(tmp>0)
        ncol+=tmp/itemw;
    
    return ncol;
}

static bool oneup(WListing *l, int *ip, int *rp)
{
    int i=*ip, r=*rp;
    int ir=ITEMROWS(l, i);
    
    if(r>0){
        (*rp)--;
        return TRUE;
    }
    
    if(i==0)
        return FALSE;
    
    (*ip)--;
    *rp=ITEMROWS(l, i-1)-1;
    return TRUE;
}


static bool onedown(WListing *l, int *ip, int *rp)
{
    int i=*ip, r=*rp;
    int ir=ITEMROWS(l, i);
    
    if(r<ir-1){
        (*rp)++;
        return TRUE;
    }
    
    if(i==l->nitemcol-1)
        return FALSE;
    
    (*ip)++;
    *rp=0;
    return TRUE;
}


void setup_listing(WListing *l, char **strs, int nstrs, bool onecol)
{
    if(l->strs!=NULL)
        deinit_listing(l);

    l->itemrows=ALLOC_N(int, nstrs);
    l->strs=strs;
    l->nstrs=nstrs;
    l->onecol=onecol;
}


void fit_listing(GrBrush *brush, const WRectangle *geom, WListing *l)
{
    int ncol, nrow=0, visrow=INT_MAX;
    int i, maxw, w, h;
    GrFontExtents fnte;
    GrBorderWidths bdw;
    
    grbrush_get_font_extents(brush, &fnte);
    grbrush_get_border_widths(brush, &bdw);
    
    w=geom->w-bdw.left-bdw.right;
    h=geom->h-bdw.top-bdw.bottom;
    
    maxw=strings_maxw(brush, l->strs, l->nstrs);
    l->itemw=maxw+COL_SPACING;
    l->itemh=fnte.max_height;
    
    if(l->onecol)
        ncol=1;
    else
        ncol=col_fit(w, l->itemw-COL_SPACING, COL_SPACING);

    if(l->itemrows!=NULL){
        for(i=0; i<l->nstrs; i++){
            if(ncol!=1){
                l->itemrows[i]=1;
            }else{
                l->itemrows[i]=string_nrows(brush, w, l->strs[i]);
                nrow+=l->itemrows[i];
            }
        }
    }
    
    if(ncol>1){
        nrow=l->nstrs/ncol+(l->nstrs%ncol ? 1 : 0);
        l->nitemcol=nrow;
    }else{
        l->nitemcol=l->nstrs;
    }
    
    if(l->itemh>0)
        visrow=h/l->itemh;
    
    if(visrow>nrow)
        visrow=nrow;
    
    l->ncol=ncol;
    l->nrow=nrow;
    l->visrow=visrow;
    l->toth=visrow*l->itemh;

    l->firstitem=l->nitemcol-1;
    l->firstoff=ITEMROWS(l, l->nitemcol-1)-1;
    for(i=1; i<visrow; i++)
        oneup(l, &(l->firstitem), &(l->firstoff));
    
}


void deinit_listing(WListing *l)
{
    int i;
    
    if(l->strs==NULL)
        return;
    
    while(l->nstrs--)
        free(l->strs[l->nstrs]);
    free(l->strs);
    l->strs=NULL;
    if(l->itemrows){
        free(l->itemrows);
        l->itemrows=NULL;
    }
}


void init_listing(WListing *l)
{
    l->nstrs=0;
    l->strs=NULL;
    l->itemrows=NULL;
    l->strs=NULL;
    l->nstrs=0;
    l->onecol=TRUE;
    l->itemw=0;
    l->itemh=0;
    l->ncol=0;
    l->nrow=0;
    l->nitemcol=0;
    l->visrow=0;
    l->toth=0;
}


static void do_draw_listing(GrBrush *brush, Window win, 
                            const WRectangle *geom, WListing *l,
                            const char *style)
{
    int r, c, i, x, y;
    GrFontExtents fnte;
    
    if(l->nitemcol==0 || l->visrow==0)
        return;
    
    grbrush_get_font_extents(brush, &fnte);
    
    grbrush_set_clipping_rectangle(brush, win, geom);
    
    x=0;
    c=0;
    while(1){
        y=geom->y+fnte.baseline;
        i=l->firstitem+c*l->nitemcol;
        r=-l->firstoff;
        y+=r*l->itemh;
        while(r<l->visrow){
            if(i>=l->nstrs)
                goto finished;
            
            draw_multirow(brush, win, geom->x+x, y,
                          geom->w-x, l->itemh, l->strs[i], 
                          style);

            y+=l->itemh*ITEMROWS(l, i);
            r+=ITEMROWS(l, i);
            i++;
        }
        x+=l->itemw;
        c++;
    }
    
finished:
    grbrush_clear_clipping_rectangle(brush, win);
}


void draw_listing(GrBrush *brush, Window win, const WRectangle *geom,
                  WListing *l, bool complete, const char *style)
{
    WRectangle geom2;
    GrBorderWidths bdw;
    
    grbrush_clear_area(brush, win, geom);
    
    grbrush_draw_border(brush, win, geom, style);

    grbrush_get_border_widths(brush, &bdw);
    
    geom2.x=geom->x+bdw.left;
    geom2.y=geom->y+bdw.top;
    geom2.w=geom->w-bdw.left-bdw.right;
    geom2.h=geom->h-bdw.top-bdw.bottom;
    
    do_draw_listing(brush, win, &geom2, l, style);
}


static bool do_scrollup_listing(WListing *l, int n)
{
    int i=l->firstitem;
    int r=l->firstoff;
    bool ret=FALSE;
    
    while(n>0){
        if(!oneup(l, &i, &r))
            break;
        ret=TRUE;
        n--;
    }

    l->firstitem=i;
    l->firstoff=r;
    
    return ret;
}


static bool do_scrolldown_listing(WListing *l, int n)
{
    int i=l->firstitem;
    int r=l->firstoff;
    int br=r, bi=i;
    int bc=l->visrow;
    bool ret=FALSE;
    
    while(--bc>0)
        onedown(l, &bi, &br);
    
    while(n>0){
        if(!onedown(l, &bi, &br))
            break;
        onedown(l, &i, &r);
        ret=TRUE;
        n--;
    }

    l->firstitem=i;
    l->firstoff=r;
    
    return ret;
}


bool scrollup_listing(WListing *l)
{
    return do_scrollup_listing(l, l->visrow);
}


bool scrolldown_listing(WListing *l)
{
    return do_scrolldown_listing(l, l->visrow);
}
