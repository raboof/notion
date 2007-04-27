/*
 * ion/mod_statusbar/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include "statusbar.h"
#include "draw.h"


static void calc_elems_x(WRectangle *g, WSBElem *elems, int nelems)
{
    int x=g->x;
    
    while(nelems>0){
        elems->x=x;
        if(elems->type==WSBELEM_STRETCH)
            x+=elems->text_w+elems->stretch;
        else
            x+=elems->text_w;
        
        nelems--;
        elems++;
    }
}


static void calc_elems_x_ra(WRectangle *g, WSBElem *elems, int nelems)
{
    int x=g->x+g->w;
    
    elems+=nelems-1;
    
    while(nelems>0){
        if(elems->type==WSBELEM_STRETCH)
            x-=elems->text_w+elems->stretch;
        else
            x-=elems->text_w;
        elems->x=x;
        
        elems--;
        nelems--;
    }
}


void statusbar_calculate_xs(WStatusBar *sb)
{
    WRectangle g;
    GrBorderWidths bdw;
    WMPlex *mgr=NULL;
    bool right_align=FALSE;
    int minx, maxx;
    int nleft=0, nright=0;
    
    if(sb->brush==NULL || sb->elems==NULL)
        return;
    
    grbrush_get_border_widths(sb->brush, &bdw);

    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(sb).w;
    g.h=REGION_GEOM(sb).h;
    
    mgr=OBJ_CAST(REGION_PARENT(sb), WMPlex);
    if(mgr!=NULL){
        WRegion *std=NULL;
        WMPlexSTDispInfo din;
        din.pos=MPLEX_STDISP_TL;
        mplex_get_stdisp(mgr, &std, &din);
        if(std==(WRegion*)sb)
            right_align=(din.pos==MPLEX_STDISP_TR || din.pos==MPLEX_STDISP_BR);
    }
    
    g.x+=bdw.left;
    g.w-=bdw.left+bdw.right;
    g.y+=bdw.top;
    g.h-=bdw.top+bdw.bottom;

    if(sb->filleridx>=0){
        nleft=sb->filleridx;
        nright=sb->nelems-(sb->filleridx+1);
    }else if(!right_align){
        nleft=sb->nelems;
        nright=0;
    }else{
        nleft=0;
        nright=sb->nelems;
    }

    if(nleft>0)
        calc_elems_x(&g, sb->elems, nleft);
    
    if(nright>0)
        calc_elems_x_ra(&g, sb->elems+sb->nelems-nright, nright);
}



static void draw_elems(GrBrush *brush, WRectangle *g, int ty,
                       WSBElem *elems, int nelems, bool needfill, 
                       bool complete)
{
    int prevx=g->x;
    int maxx=g->x+g->w;
    
    while(nelems>0){
        if(prevx<elems->x){
            g->x=prevx;
            g->w=elems->x-prevx;
            grbrush_clear_area(brush, g);
        }
            
        if(elems->type==WSBELEM_TEXT || elems->type==WSBELEM_METER){
            const char *s=(elems->text!=NULL
                           ? elems->text 
                           : STATUSBAR_NX_STR);
            
            grbrush_set_attr(brush, elems->attr);
            grbrush_set_attr(brush, elems->meter);
                
            grbrush_draw_string(brush, elems->x, ty, s, strlen(s), needfill);
            
            grbrush_unset_attr(brush, elems->meter);
            grbrush_unset_attr(brush, elems->attr);
            
            prevx=elems->x+elems->text_w;
        }
        elems++;
        nelems--;
    }

    if(prevx<maxx){
        g->x=prevx;
        g->w=maxx-prevx;
        grbrush_clear_area(brush, g);
    }
}


void statusbar_draw(WStatusBar *sb, bool complete)
{
    WRectangle g;
    GrBorderWidths bdw;
    GrFontExtents fnte;
    Window win=sb->wwin.win;
    int ty;

    if(sb->brush==NULL)
        return;
    
    grbrush_get_border_widths(sb->brush, &bdw);
    grbrush_get_font_extents(sb->brush, &fnte);

    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(sb).w;
    g.h=REGION_GEOM(sb).h;
    
    grbrush_begin(sb->brush, &g, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    grbrush_draw_border(sb->brush, &g);
    
    if(sb->elems==NULL)
        return;
    
    g.x+=bdw.left;
    g.w-=bdw.left+bdw.right;
    g.y+=bdw.top;
    g.h-=bdw.top+bdw.bottom;

    ty=(g.y+fnte.baseline+(g.h-fnte.max_height)/2);
        
    draw_elems(sb->brush, &g, ty, sb->elems, sb->nelems, TRUE, complete);
    
    grbrush_end(sb->brush);
}


