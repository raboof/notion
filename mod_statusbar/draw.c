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


static void draw_strings(GrBrush *brush, Window win, int x, int y,
                         GrTextElem *texts, int ntexts, bool needfill, 
                         const char *dfltattr)
{
    while(ntexts>0){
        if(texts->text!=NULL){
            int sl=strlen(texts->text);
            int w=grbrush_get_text_width(brush, texts->text, sl);
            grbrush_draw_string(brush, win, x, y, texts->text, sl, needfill, 
                                texts->attr ? texts->attr : dfltattr);
            x+=w;
        }
        ntexts--;
        texts++;
    }
}


static void draw_strings_ra(GrBrush *brush, Window win, int x, int y,
                            GrTextElem *texts, int ntexts, bool needfill, 
                            const char *dfltattr)
{
    texts+=ntexts;
    
    while(ntexts>0){
        ntexts--;
        texts--;
        if(texts->text!=NULL){
            int sl=strlen(texts->text);
            int w=grbrush_get_text_width(brush, texts->text, sl);
            x-=w;
            grbrush_draw_string(brush, win, x, y, texts->text, sl, needfill,
                                texts->attr ? texts->attr : dfltattr);
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
    
    /*grbrush_draw_border(sb->brush, win, &g, NULL);*/
    grbrush_draw_textbox(sb->brush, win, &g, NULL, NULL, complete);

    if(sb->strings==NULL)
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
        draw_strings(sb->brush, win, g.x+bdw.left, ty,
                     sb->strings, sb->nstrings, !complete, NULL);
    }else{
        draw_strings_ra(sb->brush, win, g.x+g.w-bdw.right, ty,
                        sb->strings, sb->nstrings, !complete, NULL);
    }
}

