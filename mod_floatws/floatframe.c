/*
 * ion/mod_floatws/floatframe.c
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
#include <ioncore/frame.h>
#include <ioncore/framep.h>
#include <ioncore/frame-pointer.h>
#include <ioncore/frame-draw.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/regbind.h>
#include <ioncore/resize.h>
#include <ioncore/sizehint.h>
#include <ioncore/extlconv.h>
#include <libtu/minmax.h>
#include "floatframe.h"
#include "floatws.h"
#include "main.h"


/*{{{ Destroy/create frame */


static bool floatframe_init(WFloatFrame *frame, WWindow *parent,
                            const WFitParams *fp)
{
    frame->bar_w=fp->g.w;
    frame->tab_min_w=0;
    frame->bar_max_width_q=1.0;
    
    if(!frame_init((WFrame*)frame, parent, fp, "frame-floating-floatws"))
        return FALSE;
    
    frame->frame.flags|=(FRAME_BAR_OUTSIDE|FRAME_DEST_EMPTY);

    region_add_bindmap((WRegion*)frame, mod_floatws_floatframe_bindmap);
    
    return TRUE;
}


WFloatFrame *create_floatframe(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WFloatFrame, floatframe, (p, parent, fp));
}


/*}}}*/
                          

/*{{{ Geometry */

#define BAR_H(FRAME) \
    ((FRAME)->frame.flags&FRAME_TAB_HIDE ? 0 : (FRAME)->frame.bar_h)

void floatframe_offsets(const WFloatFrame *frame, WRectangle *off)
{
    GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
    uint bar_h=0;
    
    if(frame->frame.brush!=NULL)
        grbrush_get_border_widths(frame->frame.brush, &bdw);
    
    off->x=-bdw.left;
    off->y=-bdw.top;
    off->w=bdw.left+bdw.right;
    off->h=bdw.top+bdw.bottom;

    bar_h=BAR_H(frame);

    off->y-=bar_h;
    off->h+=bar_h;
}


void floatframe_geom_from_managed_geom(const WFloatFrame *frame, 
                                       WRectangle *geom)
{
    WRectangle off;
    floatframe_offsets(frame, &off);
    geom->x+=off.x;
    geom->y+=off.y;
    geom->w+=off.w;
    geom->h+=off.h;
}


void floatframe_border_geom(const WFloatFrame *frame, WRectangle *geom)
{
    geom->x=0;
    geom->y=BAR_H(frame);
    geom->w=REGION_GEOM(frame).w;
    geom->h=REGION_GEOM(frame).h-BAR_H(frame);
    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);

}


void floatframe_bar_geom(const WFloatFrame *frame, WRectangle *geom)
{
    geom->x=0;
    geom->y=0;
    geom->w=frame->bar_w;
    geom->h=BAR_H(frame);
}


void floatframe_managed_geom(const WFloatFrame *frame, WRectangle *geom)
{
    WRectangle off;
    *geom=REGION_GEOM(frame);
    floatframe_offsets(frame, &off);
    geom->x=-off.x;
    geom->y=-off.y;
    geom->w=maxof(geom->w-off.w, 0);
    geom->h=maxof(geom->h-off.h, 0);
}


#define floatframe_border_inner_geom floatframe_managed_geom


void floatframe_geom_from_initial_geom(WFloatFrame *frame, 
                                       WFloatWS *ws,
                                       WRectangle *geom, 
                                       int gravity)
{
    WRectangle off;
#ifndef CF_NO_WSREL_INIT_GEOM
    /* 'app -geometry -0-0' should place the app at the lower right corner
     * of ws even if ws is nested etc.
     */
    int top=0, left=0, bottom=0, right=0;
    WRootWin *root;
    
    root=region_rootwin_of((WRegion*)ws);
    region_rootpos((WRegion*)ws, &left, &top);
    right=REGION_GEOM(root).w-left-REGION_GEOM(ws).w;
    bottom=REGION_GEOM(root).h-top-REGION_GEOM(ws).h;
#endif

    floatframe_offsets(frame, &off);

    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);
    geom->w+=off.w;
    geom->h+=off.h;
#ifndef CF_NO_WSREL_INIT_GEOM
    geom->x+=xgravity_deltax(gravity, -off.x+left, off.x+off.w+right);
    geom->y+=xgravity_deltay(gravity, -off.y+top, off.y+off.h+bottom);
    geom->x+=REGION_GEOM(ws).x;
    geom->y+=REGION_GEOM(ws).y;
#else
    geom->x+=xgravity_deltax(gravity, -off.x, off.x+off.w);
    geom->y+=xgravity_deltay(gravity, -off.y, off.y+off.h);
    region_convert_root_geom(REGION_PARENT_REG(ws), geom);
#endif
}

    
/* geom parameter==client requested geometry minus border crap */
static void floatframe_rqgeom_clientwin(WFloatFrame *frame, WClientWin *cwin,
                                        int rqflags, const WRectangle *geom_)
{
    int gravity=NorthWestGravity;
    XSizeHints hints;
    WRectangle off;
    WRegion *par;
    WRectangle geom=*geom_;
    
    if(cwin->size_hints.flags&PWinGravity)
        gravity=cwin->size_hints.win_gravity;

    floatframe_offsets(frame, &off);

    region_size_hints((WRegion*)frame, &hints);
    xsizehints_correct(&hints, &(geom.w), &(geom.h), TRUE);
    
    geom.w=maxof(geom.w, 0);
    geom.h=maxof(geom.h, 0);
    geom.w+=off.w;
    geom.h+=off.h;
    
    /* If WEAK_? is set, then geom.(x|y) is root-relative as it was not 
     * requested by the client and clientwin_handle_configure_request has
     * no better guess. Otherwise the coordinates are those requested by 
     * the client (modulo borders/gravity) and we interpret them to be 
     * root-relative coordinates for this frame modulo gravity.
     */
    if(rqflags&REGION_RQGEOM_WEAK_X)
        geom.x+=off.x;
    else
        geom.x+=xgravity_deltax(gravity, -off.x, off.x+off.w);

    if(rqflags&REGION_RQGEOM_WEAK_Y)
        geom.y+=off.y;  /* geom.y was clientwin root relative y */
    else
        geom.y+=xgravity_deltay(gravity, -off.y, off.y+off.h);

    par=REGION_PARENT_REG(frame);
    region_convert_root_geom(par, &geom);
    if(par!=NULL){
        if(geom.x+geom.w<4)
            geom.x=-geom.w+4;
        if(geom.x>REGION_GEOM(par).w-4)
            geom.x=REGION_GEOM(par).w-4;
        if(geom.y+geom.h<4)
            geom.y=-geom.h+4;
        if(geom.y>REGION_GEOM(par).h-4)
            geom.y=REGION_GEOM(par).h-4;
    }

    region_rqgeom((WRegion*)frame, REGION_RQGEOM_NORMAL, &geom, NULL);
}


void floatframe_resize_hints(WFloatFrame *frame, XSizeHints *hints_ret)
{
    WRectangle subgeom;
    WLListIterTmp tmp;
    WRegion *sub;
    int woff, hoff;
    
    mplex_managed_geom((WMPlex*)frame, &subgeom);
    
    woff=maxof(REGION_GEOM(frame).w-subgeom.w, 0);
    hoff=frame->frame.bar_h;

    if(FRAME_CURRENT(&(frame->frame))!=NULL){
        region_size_hints(FRAME_CURRENT(&(frame->frame)), hints_ret);
    }else{
        hints_ret->flags=0;
    }
    
    FRAME_L1_FOR_ALL(sub, &(frame->frame), tmp){
        xsizehints_adjust_for(hints_ret, sub);
    }
    
    if(!hints_ret->flags&PBaseSize){
        hints_ret->base_width=0;
        hints_ret->base_height=0;
        hints_ret->flags|=PBaseSize;
    }
    hints_ret->base_width+=woff;
    hints_ret->base_height+=hoff;

    hints_ret->flags|=PMinSize;
    hints_ret->min_width=woff;
    hints_ret->min_height=hoff;
}


/*}}}*/


/*{{{ Drawing routines and such */


static void floatframe_brushes_updated(WFloatFrame *frame)
{
    /* Get new bar width limits */

    frame->tab_min_w=100;
    frame->bar_max_width_q=0.95;

    if(frame->frame.brush==NULL)
        return;
    
    if(grbrush_get_extra(frame->frame.brush, "floatframe_tab_min_w",
                         'i', &(frame->tab_min_w))){
        if(frame->tab_min_w<=0)
            frame->tab_min_w=1;
    }

    if(grbrush_get_extra(frame->frame.brush, "floatframe_bar_max_w_q", 
                         'd', &(frame->bar_max_width_q))){
        if(frame->bar_max_width_q<=0.0 || frame->bar_max_width_q>1.0)
            frame->bar_max_width_q=1.0;
    }
}


static void floatframe_set_shape(WFloatFrame *frame)
{
    WRectangle gs[2];
    int n=0;
    
    if(frame->frame.brush!=NULL){
        if(!(frame->frame.flags&FRAME_TAB_HIDE)){
            floatframe_bar_geom(frame, gs+n);
            n++;
        }
        floatframe_border_geom(frame, gs+n);
        n++;
    
        grbrush_set_window_shape(frame->frame.brush, 
                                 frame->frame.mplex.win.win,
                                 TRUE, n, gs);
    }
}


#define CF_TAB_MAX_TEXT_X_OFF 10


static int init_title(WFloatFrame *frame, int i)
{
    int textw;
    
    if(frame->frame.titles[i].text!=NULL){
        free(frame->frame.titles[i].text);
        frame->frame.titles[i].text=NULL;
    }
    
    textw=frame_nth_tab_iw((WFrame*)frame, i);
    frame->frame.titles[i].iw=textw;
    return textw;
}


static void floatframe_recalc_bar(WFloatFrame *frame)
{
    int bar_w=0, textw=0, tmaxw=frame->tab_min_w, tmp=0;
    WLListIterTmp itmp;
    WRegion *sub;
    const char *p;
    GrBorderWidths bdw;
    char *title;
    uint bdtotal;
    int i, m;
    
    if(frame->frame.bar_brush==NULL)
        return;
    
    m=FRAME_MCOUNT(frame);
    
    if(m>0){
        grbrush_get_border_widths(frame->frame.bar_brush, &bdw);
        bdtotal=((m-1)*(bdw.tb_ileft+bdw.tb_iright)
                 +bdw.right+bdw.left);

        FRAME_L1_FOR_ALL(sub, &(frame->frame), itmp){
            p=region_name(sub);
            if(p==NULL)
                continue;
            
            textw=grbrush_get_text_width(frame->frame.bar_brush,
                                         p, strlen(p));
            if(textw>tmaxw)
                tmaxw=textw;
        }

        bar_w=frame->bar_max_width_q*REGION_GEOM(frame).w;
        if(bar_w<frame->tab_min_w && 
           REGION_GEOM(frame).w>frame->tab_min_w)
            bar_w=frame->tab_min_w;
        
        tmp=bar_w-bdtotal-m*tmaxw;
        
        if(tmp>0){
            /* No label truncation needed, good. See how much can be padded. */
            tmp/=m*2;
            if(tmp>CF_TAB_MAX_TEXT_X_OFF)
                tmp=CF_TAB_MAX_TEXT_X_OFF;
            bar_w=(tmaxw+tmp*2)*m+bdtotal;
        }else{
            /* Some labels must be truncated */
        }
    }else{
        bar_w=frame->tab_min_w;
        if(bar_w>frame->bar_max_width_q*REGION_GEOM(frame).w)
            bar_w=frame->bar_max_width_q*REGION_GEOM(frame).w;
    }

    if(frame->bar_w!=bar_w){
        frame->bar_w=bar_w;
        floatframe_set_shape(frame);
    }

    if(m==0 || frame->frame.titles==NULL)
        return;
    
    i=0;
    FRAME_L1_FOR_ALL(sub, &(frame->frame), itmp){
        textw=init_title(frame, i);
        if(textw>0){
            title=region_make_label(sub, textw, frame->frame.bar_brush);
            frame->frame.titles[i].text=title;
        }
        i++;
    }
}


static void floatframe_size_changed(WFloatFrame *frame, bool wchg, bool hchg)
{
    int bar_w=frame->bar_w;
    
    if(wchg)
        frame_recalc_bar((WFrame*)frame);
    if(hchg || (wchg && bar_w==frame->bar_w))
        floatframe_set_shape(frame);
}


/*}}}*/


/*{{{ Actions */


/*EXTL_DOC
 * Toggle \var{frame} stickyness. Only works across frames on 
 * \type{WFloatWS} that have the same \type{WMPlex} parent.
 */
EXTL_EXPORT_MEMBER
void floatframe_toggle_sticky(WFloatFrame *frame)
{
    WFloatStacking *st=mod_floatws_find_stacking((WRegion*)frame);
    if(st!=NULL)
        st->sticky=!st->sticky;
}

/*EXTL_DOC
 * Is \var{frame} sticky?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool floatframe_is_sticky(WFloatFrame *frame)
{
    WFloatStacking *st=mod_floatws_find_stacking((WRegion*)frame);
    return (st!=NULL ? st->sticky : FALSE);
}


/*}}}*/


/*{{{ Save/load */


static ExtlTab floatframe_get_configuration(WFloatFrame *frame)
{
    return frame_get_configuration(&(frame->frame));
}


WRegion *floatframe_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WFloatFrame *frame=create_floatframe(par, fp);
    
    if(frame==NULL)
        return NULL;
    
    frame_do_load((WFrame*)frame, tab);
    
    if(FRAME_MCOUNT(frame)==0){
        /* Nothing to manage, destroy */
        destroy_obj((Obj*)frame);
        frame=NULL;
    }
    
    return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynfuntab and class implementation */


static DynFunTab floatframe_dynfuntab[]={
    {mplex_size_changed, floatframe_size_changed},
    {frame_recalc_bar, floatframe_recalc_bar},

    {mplex_managed_geom, floatframe_managed_geom},
    {frame_bar_geom, floatframe_bar_geom},
    {frame_border_inner_geom, floatframe_border_inner_geom},
    {frame_border_geom, floatframe_border_geom},
    
    {region_rqgeom_clientwin, floatframe_rqgeom_clientwin},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)floatframe_get_configuration},

    {frame_brushes_updated, floatframe_brushes_updated},
    
    {region_size_hints, floatframe_resize_hints},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WFloatFrame, WFrame, NULL, floatframe_dynfuntab);

        
/*}}}*/
