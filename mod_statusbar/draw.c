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


static void draw_elems(GrBrush *brush, int x, int y,
                       WSBElem *elems, int nelems, bool needfill, 
                       const char *dfltattr)
{
    while(nelems>0){
        if(elems->type==WSBELEM_STRETCH){
            x+=elems->text_w+elems->stretch;
        }else{
            const char *s=(elems->text!=NULL
                           ? elems->text 
                           : STATUSBAR_NX_STR);
            grbrush_draw_string(brush, x, y, s, strlen(s), needfill, 
                                elems->attr ? elems->attr : dfltattr);
            x+=elems->text_w;
        }
        nelems--;
        elems++;
    }
}


static void draw_elems_ra(GrBrush *brush, int x, int y,
                          WSBElem *elems, int nelems, bool needfill, 
                          const char *dfltattr)
{
    elems+=nelems;
    
    while(nelems>0){
        nelems--;
        elems--;
        if(elems->type==WSBELEM_STRETCH){
            x-=elems->text_w+elems->stretch;
        }else{
            const char *s=(elems->text!=NULL
                           ? elems->text 
                           : STATUSBAR_NX_STR);
            x-=elems->text_w;
            grbrush_draw_string(brush, x, y, s, strlen(s), needfill,
                                elems->attr ? elems->attr : dfltattr);
        }
    }
}


void statusbar_draw(WStatusBar *sb, bool complete)
{
    WRectangle g;
    GrBorderWidths bdw;
    GrFontExtents fnte;
    Window win=sb->wwin.win;
    bool right_align=FALSE;
    WMPlex *mgr;
    int ty;
    
    if(sb->brush==NULL)
        return;
    
    grbrush_get_border_widths(sb->brush, &bdw);
    grbrush_get_font_extents(sb->brush, &fnte);

    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(sb).w;
    g.h=REGION_GEOM(sb).h;
    
    /*grbrush_draw_border(sb->brush, &g, NULL);*/
    grbrush_draw_textbox(sb->brush, &g, NULL, NULL, complete);

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

    if(!right_align){
        draw_elems(sb->brush, g.x+bdw.left, ty,
                   sb->elems, sb->nelems, TRUE, NULL);
    }else{
        draw_elems_ra(sb->brush, g.x+g.w-bdw.right, ty,
                      sb->elems, sb->nelems, TRUE, NULL);
    }
}

