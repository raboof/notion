/*
 * ion/mod_statusbar/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include "statusbar.h"
#include "draw.h"


static void draw_elems(GrBrush *brush, WRectangle *g, int ty,
                       WSBElem *elems, int nelems, bool needfill, 
                       const char *dfltattr, bool complete)
{
    int x=g->x;
    int maxx=g->x+g->w;
    
    while(nelems>0){
        if(elems->type==WSBELEM_STRETCH){
            int w=elems->text_w+elems->stretch;
            if(!complete && w>0){
                g->x=x;
                g->w=w;
                grbrush_clear_area(brush, g);
            }
            x+=w;
        }else{
            const char *s=(elems->text!=NULL
                           ? elems->text 
                           : STATUSBAR_NX_STR);
            grbrush_draw_string(brush, x, ty, s, strlen(s), needfill, 
                                elems->attr ? elems->attr : dfltattr);
            x+=elems->text_w;
        }
        nelems--;
        elems++;
    }

    if(!complete && x<maxx){
        g->x=x;
        g->w=maxx-x;
        grbrush_clear_area(brush, g);
    }
}


static void draw_elems_ra(GrBrush *brush, WRectangle *g, int ty,
                          WSBElem *elems, int nelems, bool needfill, 
                          const char *dfltattr, bool complete)
{
    int i;
    int elsw=0;
    WRectangle tmp=*g;
    
    for(i=0; i<nelems; i++){
        elsw+=elems[i].text_w;
        if(elems[i].type==WSBELEM_STRETCH)
            elsw+=elems[i].stretch;
    }
    
    tmp.w=g->w-elsw;
    g->x+=tmp.w;
    g->w=elsw;
        
    if(tmp.w>0 && complete)
        grbrush_clear_area(brush, g);

    draw_elems(brush, g, ty, elems, nelems, needfill, dfltattr, complete);
}


void statusbar_draw(WStatusBar *sb, bool complete)
{
    WRectangle g;
    GrBorderWidths bdw;
    GrFontExtents fnte;
    Window win=sb->wwin.win;
    bool right_align=FALSE;
    WMPlex *mgr;
    int ty, minx, maxx;
    
    if(sb->brush==NULL)
        return;
    
    grbrush_get_border_widths(sb->brush, &bdw);
    grbrush_get_font_extents(sb->brush, &fnte);

    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(sb).w;
    g.h=REGION_GEOM(sb).h;
    
    grbrush_begin(sb->brush, &g, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    grbrush_draw_border(sb->brush, &g, NULL);
    
    if(sb->elems==NULL)
        return;
    
    mgr=OBJ_CAST(REGION_PARENT(sb), WMPlex);
    if(mgr!=NULL){
        WRegion *std=NULL;
        int corner=MPLEX_STDISP_TL;
        mplex_get_stdisp(mgr, &std, &corner);
        if(std==(WRegion*)sb)
            right_align=(corner==MPLEX_STDISP_TR || corner==MPLEX_STDISP_BR);
    }
    
    ty=(g.y+bdw.top+fnte.baseline+(g.h-bdw.top-bdw.bottom-fnte.max_height)/2);
    
    g.x+=bdw.left;
    g.w-=bdw.left+bdw.right;
    
    if(!right_align){
        draw_elems(sb->brush, &g, ty, sb->elems, sb->nelems, 
                   TRUE, NULL, complete);
    }else{
        draw_elems_ra(sb->brush, &g, ty, sb->elems, sb->nelems, 
                      TRUE, NULL, complete);
    }
    
    grbrush_end(sb->brush);
}

