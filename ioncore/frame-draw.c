/*
 * ion/ioncore/frame-draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/minmax.h>

#include "common.h"
#include "frame.h"
#include "framep.h"
#include "frame-draw.h"
#include "strings.h"
#include "names.h"
#include "gr.h"


#define BAR_INSIDE_BORDER(FRAME) (!((FRAME)->flags&FRAME_BAR_OUTSIDE))
#define BAR_OFF(FRAME) (0)


/*{{{ Dynfuns */


void frame_recalc_bar(WFrame *frame)
{
    CALL_DYN(frame_recalc_bar, frame, (frame));
    
}


void frame_draw_bar(const WFrame *frame, bool complete)
{
    CALL_DYN(frame_draw_bar, frame, (frame, complete));
}


void frame_bar_geom(const WFrame *frame, WRectangle *geom)
{
    CALL_DYN(frame_bar_geom, frame, (frame, geom));
}


void frame_border_geom(const WFrame *frame, WRectangle *geom)
{
    CALL_DYN(frame_border_geom, frame, (frame, geom));
}


void frame_border_inner_geom(const WFrame *frame, WRectangle *geom)
{
    CALL_DYN(frame_border_inner_geom, frame, (frame, geom));
}


void frame_brushes_updated(WFrame *frame)
{
    CALL_DYN(frame_brushes_updated, frame, (frame));
}


/*}}}*/


/*{{{ (WFrame) dynfun default implementations */


static uint get_spacing(const WFrame *frame)
{
    GrBorderWidths bdw;
    
    if(frame->brush==NULL)
        return 0;
    
    grbrush_get_border_widths(frame->brush, &bdw);
    
    return bdw.spacing;
}


void frame_border_geom_default(const WFrame *frame, WRectangle *geom)
{
    geom->x=0;
    geom->y=0;
    geom->w=REGION_GEOM(frame).w;
    geom->h=REGION_GEOM(frame).h;
    
    if(!BAR_INSIDE_BORDER(frame) && 
       !(frame->flags&FRAME_TAB_HIDE) &&
       frame->brush!=NULL){
        geom->y+=frame->bar_h+BAR_OFF(frame);
        geom->h-=frame->bar_h+2*BAR_OFF(frame);
    }
}


void frame_border_inner_geom_default(const WFrame *frame, 
                                        WRectangle *geom)
{
    GrBorderWidths bdw;
    
    frame_border_geom_default(frame, geom);

    if(frame->brush!=NULL){
        grbrush_get_border_widths(frame->brush, &bdw);

        geom->x+=bdw.left;
        geom->y+=bdw.top;
        geom->w-=bdw.left+bdw.right;
        geom->h-=bdw.top+bdw.bottom;
    }
    
    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);
}


void frame_bar_geom_default(const WFrame *frame, WRectangle *geom)
{
    uint off;
    
    if(BAR_INSIDE_BORDER(frame)){
        off=get_spacing(frame);
        frame_border_inner_geom_default(frame, geom);
    }else{
        off=BAR_OFF(frame);
        geom->x=0;
        geom->y=0;
        geom->w=REGION_GEOM(frame).w;
    }
    geom->x+=off;
    geom->y+=off;
    geom->w=maxof(0, geom->w-2*off);

    geom->h=(frame->flags&FRAME_TAB_HIDE 
             ? 0 : frame->bar_h);
}


void frame_managed_geom_default(const WFrame *frame, WRectangle *geom)
{
    uint spacing=get_spacing(frame);
    
    frame_border_inner_geom_default(frame, geom);
    
    geom->x+=spacing;
    geom->y+=spacing;
    geom->w-=2*spacing;
    geom->h-=2*spacing;
    
    if(BAR_INSIDE_BORDER(frame) && !(frame->flags&FRAME_TAB_HIDE)){
        geom->y+=frame->bar_h+spacing;
        geom->h-=frame->bar_h+spacing;
    }
    
    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);
}


static int init_title(WFrame *frame, int i)
{
    int textw;
    
    if(frame->titles[i].text!=NULL){
        free(frame->titles[i].text);
        frame->titles[i].text=NULL;
    }
    
    textw=frame_nth_tab_iw((WFrame*)frame, i);
    frame->titles[i].iw=textw;
    return textw;
}


void frame_recalc_bar_default(WFrame *frame)
{
    int textw, i;
    WLListIterTmp tmp;
    WRegion *sub;
    char *title;

    if(frame->bar_brush==NULL || frame->titles==NULL)
        return;
    
    i=0;
    
    if(FRAME_MCOUNT(frame)==0){
        textw=init_title(frame, i);
        if(textw>0){
            title=grbrush_make_label(frame->bar_brush, CF_STR_EMPTY, textw);
            frame->titles[i].text=title;
        }
        return;
    }
    
    FRAME_MX_FOR_ALL(sub, frame, tmp){
        textw=init_title(frame, i);
        if(textw>0){
            if(frame->flags&FRAME_SHOW_NUMBERS){
                char *s=NULL;
                libtu_asprintf(&s, "[%d]", i+1);
                if(s!=NULL){
                    title=grbrush_make_label(frame->bar_brush, s, textw);
                    free(s);
                }else{
                    title=NULL;
                }
            }else{
                title=region_make_label(sub, textw, frame->bar_brush);
            }
            frame->titles[i].text=title;
        }
        i++;
    }
}


void frame_draw_bar_default(const WFrame *frame, bool complete)
{
    WRectangle geom;
    const char *cattr=(REGION_IS_ACTIVE(frame) 
                       ? "active" : "inactive");
    
    if(frame->bar_brush==NULL ||
       frame->flags&FRAME_TAB_HIDE ||
       frame->titles==NULL){
        return;
    }
    
    frame_bar_geom(frame, &geom);

    grbrush_begin(frame->bar_brush, &geom, GRBRUSH_AMEND);
    
    grbrush_draw_textboxes(frame->bar_brush, &geom, frame->titles_n, 
                           frame->titles, complete, cattr);
    
    grbrush_end(frame->bar_brush);
}


void frame_draw_default(const WFrame *frame, bool complete)
{
    WRectangle geom;
    const char *attr=(REGION_IS_ACTIVE(frame) 
                      ? "active" : "inactive");
    
    if(frame->brush==NULL)
        return;
    
    frame_border_geom(frame, &geom);
    
    grbrush_begin(frame->brush, &geom, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    grbrush_draw_border(frame->brush, &geom, attr);
    frame_draw_bar(frame, TRUE);
    
    grbrush_end(frame->brush);
}


void frame_brushes_updated_default(WFrame *frame)
{
    ExtlTab tab;
    bool b=TRUE;

    if(frame->brush==NULL)
        return;
    
    grbrush_get_extra(frame->brush, "bar_inside_border", 'b', &b);
    
    if(b)
        frame->flags&=~FRAME_BAR_OUTSIDE;
    else
        frame->flags|=FRAME_BAR_OUTSIDE;
}


/*}}}*/


/*{{{ Misc. */


void frame_updategr(WFrame *frame)
{
    Window win=frame->mplex.win.win;
    WRectangle geom;
    WRegion *sub;
    
    frame_release_brushes(frame);
    
    frame_initialise_gr(frame);

    mplex_managed_geom(&frame->mplex, &geom);
    
    /* Update children */
    region_updategr_default((WRegion*)frame);
    
    mplex_fit_managed(&frame->mplex);
    frame_recalc_bar(frame);
    frame_set_background(frame, TRUE);
}


void frame_initialise_gr(WFrame *frame)
{
    Window win=frame->mplex.win.win;
    GrBorderWidths bdw;
    GrFontExtents fnte;
    WRootWin *rw=region_rootwin_of((WRegion*)frame);
    const char *style=stringstore_get(frame->style);
    char *tab_style;
    
    assert(style!=NULL);
    
    frame->bar_h=0;
    
    frame->brush=gr_get_brush(win, rw, style);
    if(frame->brush==NULL)
        return;
    
    tab_style=scat("tab-", style);
    if(tab_style==NULL)
        return;
    
    frame->bar_brush=grbrush_get_slave(frame->brush, rw, tab_style);
    free(tab_style);
    
    if(frame->bar_brush==NULL)
        return;
    
    grbrush_get_border_widths(frame->bar_brush, &bdw);
    grbrush_get_font_extents(frame->bar_brush, &fnte);
    
    frame->bar_h=bdw.top+bdw.bottom+fnte.max_height;
    
    frame_brushes_updated(frame);
}


void frame_release_brushes(WFrame *frame)
{
    if(frame->bar_brush!=NULL){
        grbrush_release(frame->bar_brush);
        frame->bar_brush=NULL;
    }
    
    if(frame->brush!=NULL){
        grbrush_release(frame->brush);
        frame->brush=NULL;
    }
}


bool frame_set_background(WFrame *frame, bool set_always)
{
    GrTransparency mode=GR_TRANSPARENCY_DEFAULT;
    
    if(FRAME_CURRENT(frame)!=NULL){
        if(OBJ_IS(FRAME_CURRENT(frame), WClientWin)){
            WClientWin *cwin=(WClientWin*)FRAME_CURRENT(frame);
            mode=(cwin->flags&CLIENTWIN_PROP_TRANSPARENT
                  ? GR_TRANSPARENCY_YES : GR_TRANSPARENCY_NO);
        }else if(!OBJ_IS(FRAME_CURRENT(frame), WGroup)){
            mode=GR_TRANSPARENCY_NO;
        }
    }
    
    if(mode!=frame->tr_mode || set_always){
        frame->tr_mode=mode;
        if(frame->brush!=NULL){
            grbrush_enable_transparency(frame->brush, mode);
            window_draw((WWindow*)frame, TRUE);
        }
        return TRUE;
    }
    
    return FALSE;
}


/*}}}*/
