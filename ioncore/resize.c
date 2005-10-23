/* 
 * ion/ioncore/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <limits.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libextl/extl.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "global.h"
#include "resize.h"
#include "gr.h"
#include "sizehint.h"
#include "event.h"
#include "cursor.h"
#include "extlconv.h"
#include "grab.h"
#include "framep.h"
#include "infowin.h"


#define XOR_RESIZE (!ioncore_g.opaque_resize)


/*{{{ Size/position display and rubberband */


static void draw_rubberbox(WRootWin *rw, const WRectangle *rect)
{
    XPoint fpts[5];
    
    fpts[0].x=rect->x;
    fpts[0].y=rect->y;
    fpts[1].x=rect->x+rect->w;
    fpts[1].y=rect->y;
    fpts[2].x=rect->x+rect->w;
    fpts[2].y=rect->y+rect->h;
    fpts[3].x=rect->x;
    fpts[3].y=rect->y+rect->h;
    fpts[4].x=rect->x;
    fpts[4].y=rect->y;
    
    XDrawLines(ioncore_g.dpy, WROOTWIN_ROOT(rw), rw->xor_gc, fpts, 5, 
               CoordModeOrigin);
}


static int max_width(GrBrush *brush, const char *str)
{
    int maxw=0, w;
    
    while(str && *str!='\0'){
        w=grbrush_get_text_width(brush, str, 1);
        if(w>maxw)
            maxw=w;
        str++;
    }
    
    return maxw;
}


static int chars_for_num(int d)
{
    int n=0;
    
    do{
        n++;
        d/=10;
    }while(d);
    
    return n;
}


static WInfoWin *setup_moveres_display(WWindow *parent, int cx, int cy)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    WInfoWin *infowin;
    WFitParams fp;
    
    fp.mode=REGION_FIT_EXACT;
    fp.g.x=0;
    fp.g.y=0;
    fp.g.w=1;
    fp.g.h=1;
    
    infowin=create_infowin(parent, &fp, "moveres_display");
    
    if(infowin==NULL)
        return NULL;
    
    grbrush_get_border_widths(INFOWIN_BRUSH(infowin), &bdw);
    grbrush_get_font_extents(INFOWIN_BRUSH(infowin), &fnte);
    
    /* Create move/resize position/size display window */
    fp.g.w=3;
    fp.g.w+=chars_for_num(REGION_GEOM(parent).w);
    fp.g.w+=chars_for_num(REGION_GEOM(parent).h);
    fp.g.w*=max_width(INFOWIN_BRUSH(infowin), "0123456789x+");     
    fp.g.w+=bdw.left+bdw.right;
    fp.g.h=fnte.max_height+bdw.top+bdw.bottom;;
    
    fp.g.x=cx-fp.g.w/2;
    fp.g.y=cy-fp.g.h/2;
    
    region_fitrep((WRegion*)infowin, NULL, &fp);
    region_map((WRegion*)infowin);
    
    return infowin;
}


static void moveres_draw_infowin(WMoveresMode *mode)
{
    WRectangle geom;
    char *buf;
    
    if(mode->infowin==NULL)
        return;
    
    buf=INFOWIN_BUFFER(mode->infowin);
    
    if(buf==NULL)
        return;
    
    if(mode->mode==MOVERES_SIZE){
        int w, h;
        
        w=mode->geom.w;
        h=mode->geom.h;
        
        if((mode->hints.flags&PResizeInc) &&
           (mode->hints.width_inc>1 || mode->hints.height_inc>1)){
            if(mode->hints.flags&PBaseSize){
                w-=mode->hints.base_width;
                h-=mode->hints.base_height;
            }
            w/=mode->hints.width_inc;
            h/=mode->hints.height_inc;
        }
        
        snprintf(buf, INFOWIN_BUFFER_LEN, "%dx%d", w, h);
    }else{
        snprintf(buf, INFOWIN_BUFFER_LEN, "%+d %+d", 
                 mode->geom.x, mode->geom.y);
    }
    
    window_draw((WWindow*)mode->infowin, TRUE);
}


static void moveres_draw_rubberband(WMoveresMode *mode)
{
    WRectangle rgeom=mode->geom;
    int rx, ry;
    WRootWin *rootwin=(mode->reg==NULL 
                       ? NULL 
                       : region_rootwin_of(mode->reg));
    
    if(rootwin==NULL)
        return;
    
    rgeom.x+=mode->parent_rx;
    rgeom.y+=mode->parent_ry;
    
    if(mode->rubfn==NULL)
        draw_rubberbox(rootwin, &rgeom);
    else
        mode->rubfn(rootwin, &rgeom);
}


/*}}}*/


/*{{{ Move/resize mode */


WMoveresMode *tmpmode=NULL;


EXTL_EXPORT
IMPLCLASS(WMoveresMode, Obj, NULL, NULL);


WMoveresMode *moveres_mode(WRegion *reg)
{
    if(tmpmode==NULL)
        return NULL;
    return ((reg==NULL || tmpmode->reg==reg) ? tmpmode : NULL);
}


WRegion *moveresmode_target(WMoveresMode *mode)
{
    return mode->reg;
}


static bool moveresmode_init(WMoveresMode *mode, WRegion *reg,
                             WDrawRubberbandFn *rubfn, bool cumulative)
{
    WWindow *parent;
    WRegion *mgr;
    
    if(tmpmode!=NULL)
        return FALSE;
    
    parent=REGION_PARENT(reg);
    
    if(parent==NULL)
        return FALSE;
    
    tmpmode=mode;
    
    mode->snap_enabled=FALSE;
    region_size_hints(reg, &mode->hints);
    
    region_rootpos((WRegion*)parent, &mode->parent_rx, &mode->parent_ry);
    
    mode->geom=REGION_GEOM(reg);
    mode->origgeom=REGION_GEOM(reg);
    mode->dx1=0;
    mode->dx2=0;
    mode->dy1=0;
    mode->dy2=0;
    mode->rubfn=rubfn;
    mode->resize_cumulative=cumulative;
    mode->rqflags=(XOR_RESIZE ? REGION_RQGEOM_TRYONLY : 0);
    mode->reg=reg;
    mode->mode=MOVERES_SIZE;
    
    /* Get snapping geometry */
    mgr=REGION_MANAGER(reg);
    
    if(mgr!=NULL){
        mode->snapgeom=REGION_GEOM(mgr);
        
        if(mgr==(WRegion*)parent){
            mode->snapgeom.x=0;
            mode->snapgeom.y=0;    
            mode->snap_enabled=FALSE;
        }else if(REGION_PARENT(mgr)==parent){
            mode->snap_enabled=TRUE;
        }
    }
    
    if(!mode->hints.flags&PMinSize || mode->hints.min_width<1)
        mode->hints.min_width=1;
    if(!mode->hints.flags&PMinSize || mode->hints.min_height<1)
        mode->hints.min_height=1;
    
    /* Set up info window */
    {
        int x=mode->geom.x+mode->geom.w/2;
        int y=mode->geom.y+mode->geom.h/2;
        mode->infowin=setup_moveres_display(parent, x, y);
    }
                                        
    moveres_draw_infowin(mode);
    
    if(XOR_RESIZE){
        XGrabServer(ioncore_g.dpy);
        moveres_draw_rubberband(mode);
    }
    
    return TRUE;
}


static WMoveresMode *create_moveresmode(WRegion *reg,
                                        WDrawRubberbandFn *rubfn,
                                        bool cumulative)
{
    CREATEOBJ_IMPL(WMoveresMode, moveresmode, (p, reg, rubfn, cumulative));
}


WMoveresMode *region_begin_resize(WRegion *reg, WDrawRubberbandFn *rubfn, 
                                  bool cumulative)
{
    WMoveresMode *mode=create_moveresmode(reg, rubfn, cumulative);
    
    if(mode!=NULL){
        mode->mode=MOVERES_SIZE;
        ioncore_change_grab_cursor(IONCORE_CURSOR_RESIZE);
    }

    return mode;
}


WMoveresMode *region_begin_move(WRegion *reg, WDrawRubberbandFn *rubfn, 
                                bool cumulative)
{
    WMoveresMode *mode=create_moveresmode(reg, rubfn, cumulative);
    
    if(mode!=NULL){
        mode->mode=MOVERES_POS;
        ioncore_change_grab_cursor(IONCORE_CURSOR_MOVE);
    }
    
    return mode;
}


static void moveresmode_delta(WMoveresMode *mode, 
                              int dx1, int dx2, int dy1, int dy2,
                              WRectangle *rret)
{
    WRectangle geom;
    int w=0, h=0;
    int realdx1, realdx2, realdy1, realdy2;
    
    realdx1=(mode->dx1+=dx1);
    realdx2=(mode->dx2+=dx2);
    realdy1=(mode->dy1+=dy1);
    realdy2=(mode->dy2+=dy2);
    geom=mode->origgeom;
    
    /* snap */
    if(mode->snap_enabled){
        WRectangle *sg=&mode->snapgeom;
        int er=CF_EDGE_RESISTANCE;
        
        if(mode->dx1!=0 && geom.x+mode->dx1<sg->x && geom.x+mode->dx1>sg->x-er)
            realdx1=sg->x-geom.x;
        if(mode->dx2!=0 && geom.x+geom.w+mode->dx2>sg->x+sg->w && geom.x+geom.w+mode->dx2<sg->x+sg->w+er)
            realdx2=sg->x+sg->w-geom.x-geom.w;
        if(mode->dy1!=0 && geom.y+mode->dy1<sg->y && geom.y+mode->dy1>sg->y-er)
            realdy1=sg->y-geom.y;
        if(mode->dy2!=0 && geom.y+geom.h+mode->dy2>sg->y+sg->h && geom.y+geom.h+mode->dy2<sg->y+sg->h+er)
            realdy2=sg->y+sg->h-geom.y-geom.h;
    }
    
    w=mode->origgeom.w-realdx1+realdx2;
    h=mode->origgeom.h-realdy1+realdy2;
    
    if(w<=0)
        w=mode->hints.min_width;
    if(h<=0)
        h=mode->hints.min_height;
    
    xsizehints_correct(&mode->hints, &w, &h, TRUE);
    
    /* Do not modify coordinates and sizes that were not requested to be
     * changed. 
     */
    
    if(mode->dx1==mode->dx2){
        if(mode->dx1==0 || realdx1!=mode->dx1)
            geom.x+=realdx1;
        else
            geom.x+=realdx2;
    }else{
        geom.w=w;
        if(mode->dx1==0 || realdx1!=mode->dx1)
            geom.x+=realdx1;
        else
            geom.x+=mode->origgeom.w-geom.w;
    }
    
    
    if(mode->dy1==mode->dy2){
        if(mode->dy1==0 || realdy1!=mode->dy1)
            geom.y+=realdy1;
        else
            geom.y+=realdy2;
    }else{
        geom.h=h;
        if(mode->dy1==0 || realdy1!=mode->dy1)
            geom.y+=realdy1;
        else
            geom.y+=mode->origgeom.h-geom.h;
    }
    
    if(XOR_RESIZE)
        moveres_draw_rubberband(mode);
    
    if(mode->reg!=NULL)
        region_rqgeom(mode->reg, mode->rqflags, &geom, &mode->geom);
    
    if(!mode->resize_cumulative){
        mode->dx1=0;
        mode->dx2=0;
        mode->dy1=0;
        mode->dy2=0;
        mode->origgeom=mode->geom;
    }
    
    moveres_draw_infowin(mode);
    
    if(XOR_RESIZE)
        moveres_draw_rubberband(mode);
    
    if(rret!=NULL)
        *rret=mode->geom;
}


void moveresmode_delta_resize(WMoveresMode *mode, 
                              int dx1, int dx2, int dy1, int dy2,
                              WRectangle *rret)
{
    mode->mode=MOVERES_SIZE;
    moveresmode_delta(mode, dx1, dx2, dy1, dy2, rret);
}


void moveresmode_delta_move(WMoveresMode *mode, 
                            int dx, int dy, WRectangle *rret)
{
    mode->mode=MOVERES_POS;
    moveresmode_delta(mode, dx, dx, dy, dy, rret);
}


/* It is ugly to do this here, but it will have to do for now... */
static void set_saved(WMoveresMode *mode, WRegion *reg)
{
    WFrame *frame;
    
    if(!OBJ_IS(reg, WFrame))
        return;
    
    frame=(WFrame*)reg;
    
    /* Restore saved sizes from the beginning of the resize action */
    if(mode->origgeom.w!=mode->geom.w){
        frame->saved_x=mode->origgeom.x;
        frame->saved_w=mode->origgeom.w;
    }
    
    if(mode->origgeom.h!=mode->geom.h){
        frame->saved_y=mode->origgeom.y;
        frame->saved_h=mode->origgeom.h;
    }
}


bool moveresmode_do_end(WMoveresMode *mode, bool apply)
{
    WRegion *reg=mode->reg;
    
    assert(reg!=NULL);
    assert(tmpmode==mode);
    
    tmpmode=NULL;
    
    if(XOR_RESIZE){
        moveres_draw_rubberband(mode);
        if(apply){
            WRectangle g2=mode->geom;
            region_rqgeom(reg, mode->rqflags&~REGION_RQGEOM_TRYONLY,
                          &g2, &mode->geom);
        }
        XUngrabServer(ioncore_g.dpy);
    }
    if(apply)
        set_saved(mode, reg);
    
    if(mode->infowin!=NULL){
        mainloop_defer_destroy((Obj*)mode->infowin);
        mode->infowin=NULL;
    }
    destroy_obj((Obj*)mode);
    
    return TRUE;
}


/*}}}*/


/*{{{ Request and other dynfuns */


void region_rqgeom(WRegion *reg, int flags, const WRectangle *geom,
                   WRectangle *geomret)
{
    bool tryonly=(flags&REGION_RQGEOM_TRYONLY);
    
    if(REGION_MANAGER(reg)!=NULL){
        region_managed_rqgeom(REGION_MANAGER(reg), reg, flags, geom,
                              geomret);
    }else{
        if(geomret!=NULL)
            *geomret=REGION_GEOM(reg);
        if(!tryonly)
            region_fit(reg, geom, REGION_FIT_EXACT);
    }

    /*if(!tryonly && geomret!=NULL)
        *geomret=REGION_GEOM(reg);*/
}


/*EXTL_DOC
 * Attempt to resize and/or move \var{reg}. The table \var{g} is a usual
 * geometry specification (fields \var{x}, \var{y}, \var{w} and \var{h}),
 * but may contain missing fields, in which case, \var{reg}'s manager may
 * attempt to leave that attribute unchanged.
 */
EXTL_EXPORT_AS(WRegion, rqgeom)
ExtlTab region_rqgeom_extl(WRegion *reg, ExtlTab g)
{
    WRectangle geom=REGION_GEOM(reg);
    WRectangle ogeom=REGION_GEOM(reg);
    int flags=REGION_RQGEOM_WEAK_ALL;
    
    if(extl_table_gets_i(g, "x", &(geom.x)))
       flags&=~REGION_RQGEOM_WEAK_X;
    if(extl_table_gets_i(g, "y", &(geom.y)))
       flags&=~REGION_RQGEOM_WEAK_Y;
    if(extl_table_gets_i(g, "w", &(geom.w)))
       flags&=~REGION_RQGEOM_WEAK_W;
    if(extl_table_gets_i(g, "h", &(geom.h)))
       flags&=~REGION_RQGEOM_WEAK_H;

    geom.w=maxof(1, geom.w);
    geom.h=maxof(1, geom.h);
    
    region_rqgeom(reg, flags, &geom, &ogeom);
    
    return extl_table_from_rectangle(&ogeom);
}


void region_managed_rqgeom(WRegion *mgr, WRegion *reg,
                           int flags, const WRectangle *geom,
                           WRectangle *geomret)
{
    CALL_DYN(region_managed_rqgeom, mgr,
             (mgr, reg, flags, geom, geomret));
}


void region_rqgeom_clientwin(WRegion *mgr, WClientWin *cwin,
                             int flags, const WRectangle *geom)
{
    CALL_DYN(region_rqgeom_clientwin, mgr, (mgr, cwin, flags, geom));
}


void region_managed_rqgeom_allow(WRegion *mgr, WRegion *reg,
                                 int flags, const WRectangle *geom,
                                 WRectangle *geomret)
{
    if(geomret!=NULL)
        *geomret=*geom;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fit(reg, geom, REGION_FIT_EXACT);
}


void region_managed_rqgeom_unallow(WRegion *mgr, WRegion *reg,
                                   int flags, const WRectangle *geom,
                                   WRectangle *geomret)
{
    if(geomret!=NULL)
        *geomret=REGION_GEOM(reg);
}


void region_size_hints(WRegion *reg, XSizeHints *hints_ret)
{
    hints_ret->flags=0;
    {
        CALL_DYN(region_size_hints, reg, (reg, hints_ret));
    }
    if(!(hints_ret->flags&PMinSize)){
        hints_ret->min_width=1;
        hints_ret->min_height=1;
    }
    if(!(hints_ret->flags&PBaseSize)){
        hints_ret->base_width=0;
        hints_ret->base_height=0;
    }
    if(!(hints_ret->flags&PMaxSize)){
        hints_ret->max_width=INT_MAX;
        hints_ret->max_height=INT_MAX;
    }
    /*hints_ret->flags|=(PMinSize|PBaseSize|PMaxSize);*/
}


int region_orientation(WRegion *reg)
{
    int ret=REGION_ORIENTATION_NONE;
    
    CALL_DYN_RET(ret, int, region_orientation, reg, (reg));
    
    return ret;
}


/*EXTL_DOC
 * Returns size hints for \var{reg}. The returned table always contains the
 * fields \code{min_?}, \code{base_?} and sometimes the fields \code{max_?},
 * \code{base_?} and \code{inc_?}, where \code{?}=\code{w}, \code{h}.
 */
EXTL_SAFE
EXTL_EXPORT_AS(WRegion, size_hints)
ExtlTab region_size_hints_extl(WRegion *reg)
{
    XSizeHints hints;
    ExtlTab tab;
    
    region_size_hints(reg, &hints);
    
    tab=extl_create_table();
    
    /* Base size is always guaranteed to be set. */
    extl_table_sets_i(tab, "base_w", hints.base_width);
    extl_table_sets_i(tab, "base_h", hints.base_height);
    /* Minimum size is always guaranteed to be set. */
    extl_table_sets_i(tab, "min_w", hints.min_width);
    extl_table_sets_i(tab, "min_h", hints.min_height);
    if(hints.flags&PMaxSize){
        extl_table_sets_i(tab, "max_w", hints.max_width);
        extl_table_sets_i(tab, "max_h", hints.max_height);
    }
    if(hints.flags&PBaseSize){
        extl_table_sets_i(tab, "base_w", hints.base_width);
        extl_table_sets_i(tab, "base_h", hints.base_height);
    }
    if(hints.flags&PResizeInc){
        extl_table_sets_i(tab, "inc_w", hints.width_inc);
        extl_table_sets_i(tab, "inc_h", hints.height_inc);
    }
    
    return tab;
}

/*}}}*/


/*{{{ Restore size, maximize, shade */


static void correct_frame_size(WFrame *frame, int *w, int *h)
{
    XSizeHints hints;
    int wdiff, hdiff;
    
    region_size_hints((WRegion*)frame, &hints);
    xsizehints_correct(&hints, w, h, TRUE);
}


static bool rqh(WFrame *frame, int y, int h)
{
    WRectangle geom, rgeom;
    int dummy_w;
    
    geom.x=REGION_GEOM(frame).x;
    geom.w=REGION_GEOM(frame).w;
    geom.y=y;
    geom.h=h;
    
    dummy_w=geom.w;
    correct_frame_size(frame, &dummy_w, &(geom.h));
    
    region_rqgeom((WRegion*)frame, REGION_RQGEOM_VERT_ONLY, &geom, &rgeom);
    
    return (abs(rgeom.y-REGION_GEOM(frame).y)>1 ||
            abs(rgeom.h-REGION_GEOM(frame).h)>1);
}


/*EXTL_DOC
 * Attempt to toggle vertical maximisation of \var{frame}.
 */
EXTL_EXPORT_MEMBER
void frame_maximize_vert(WFrame *frame)
{
    WRegion *mp=region_manager((WRegion*)frame);
    int oy, oh;
    
    if(frame->flags&FRAME_SHADED || frame->flags&FRAME_MAXED_VERT){
        if(frame->flags&FRAME_SAVED_VERT)
            rqh(frame, frame->saved_y, frame->saved_h);
        frame->flags&=~(FRAME_MAXED_VERT|FRAME_SAVED_VERT);
        return;
    }

    if(mp==NULL)
        return;
    
    oy=REGION_GEOM(frame).y;
    oh=REGION_GEOM(frame).h;
    
    rqh(frame, 0, REGION_GEOM(mp).h);

    frame->flags|=(FRAME_MAXED_VERT|FRAME_SAVED_VERT);
    frame->saved_y=oy;
    frame->saved_h=oh;
}

static bool rqw(WFrame *frame, int x, int w)
{
    WRectangle geom, rgeom;
    int dummy_h;

    geom.x=x;
    geom.w=w;
    geom.y=REGION_GEOM(frame).y;
    geom.h=REGION_GEOM(frame).h;
    
    dummy_h=geom.h;
    correct_frame_size(frame, &(geom.w), &dummy_h);
    
    region_rqgeom((WRegion*)frame, REGION_RQGEOM_HORIZ_ONLY, &geom, &rgeom);
    
    return (abs(rgeom.x-REGION_GEOM(frame).x)>1 ||
            abs(rgeom.w-REGION_GEOM(frame).w)>1);
}


/*EXTL_DOC
 * Attempt to toggle horizontal maximisation of \var{frame}.
 */
EXTL_EXPORT_MEMBER
void frame_maximize_horiz(WFrame *frame)
{
    WRegion *mp=region_manager((WRegion*)frame);
    int ox, ow;
    
    if(frame->flags&FRAME_MIN_HORIZ || frame->flags&FRAME_MAXED_HORIZ){
        if(frame->flags&FRAME_SAVED_HORIZ)
            rqw(frame, frame->saved_x, frame->saved_w);
        frame->flags&=~(FRAME_MAXED_HORIZ|FRAME_SAVED_HORIZ);
        return;
    }

    if(mp==NULL)
        return;
    
    ox=REGION_GEOM(frame).x;
    ow=REGION_GEOM(frame).w;
    
    rqw(frame, 0, REGION_GEOM(mp).w);
        
    frame->flags|=(FRAME_MAXED_HORIZ|FRAME_SAVED_HORIZ);
    frame->saved_x=ox;
    frame->saved_w=ow;
}



/*}}}*/


/*{{{ Misc. */


uint region_min_h(WRegion *reg)
{
    XSizeHints hints;
    region_size_hints(reg, &hints);
    return hints.min_height;
}


uint region_min_w(WRegion *reg)
{
    XSizeHints hints;
    region_size_hints(reg, &hints);
    return hints.min_width;
}


void region_convert_root_geom(WRegion *reg, WRectangle *geom)
{
    int rx, ry;
    if(reg!=NULL){
        region_rootpos(reg, &rx, &ry);
        geom->x-=rx;
        geom->y-=ry;
    }
}

/*}}}*/

