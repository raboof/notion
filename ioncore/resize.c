/* 
 * ion/ioncore/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>

#include "common.h"
#include "global.h"
#include "resize.h"
#include "gr.h"
#include "sizehint.h"
#include "event.h"
#include "cursor.h"
#include <libtu/objp.h>
#include "extl.h"
#include "extlconv.h"
#include "grab.h"
#include "framep.h"
#include "infowin.h"
#include "defer.h"
#include "region-iter.h"
#include <libtu/minmax.h>


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
    WRectangle g;
    WInfoWin *infowin;
    
    g.x=0;
    g.y=0;
    g.w=1;
    g.h=1;
    
    infowin=create_infowin(parent, &g, "moveres_display");
    
    if(infowin==NULL)
        return NULL;
    
    grbrush_get_border_widths(INFOWIN_BRUSH(infowin), &bdw);
    grbrush_get_font_extents(INFOWIN_BRUSH(infowin), &fnte);
    
    /* Create move/resize position/size display window */
    g.w=3;
    g.w+=chars_for_num(REGION_GEOM(parent).w);
    g.w+=chars_for_num(REGION_GEOM(parent).h);
    g.w*=max_width(INFOWIN_BRUSH(infowin), "0123456789x+");     
    g.w+=bdw.left+bdw.right;
    g.h=fnte.max_height+bdw.top+bdw.bottom;;
    
    g.x=cx-g.w/2;
    g.y=cy-g.h/2;
    
    region_fit((WRegion*)infowin, &g);
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
        
        w=(mode->geom.w-mode->origgeom.w)+mode->relw;
        h=(mode->geom.h-mode->origgeom.h)+mode->relh;
        
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
    
    parent=REGION_PARENT_CHK(reg, WWindow);
    
    if(parent==NULL)
        return FALSE;
    
    tmpmode=mode;
    
    mode->snap_enabled=FALSE;
    region_resize_hints(reg, &mode->hints, &mode->relw, &mode->relh);
    
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
        }else if(REGION_PARENT(mgr)==(WRegion*)parent){
            mode->snap_enabled=TRUE;
        }
    }
    
    if(!mode->hints.flags&PMinSize || mode->hints.min_width<1)
        mode->hints.min_width=1;
    if(!mode->hints.flags&PMinSize || mode->hints.min_height<1)
        mode->hints.min_height=1;
    
    /* Set up info window */
    {
        int x=mode->parent_rx+mode->geom.x+mode->geom.w/2;
        int y=mode->parent_ry+mode->geom.y+mode->geom.h/2;
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
    int w, h;
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
    
    w=mode->relw-realdx1+realdx2;
    h=mode->relh-realdy1+realdy2;
    
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
        geom.w=geom.w-mode->relw+w;
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
        geom.h=geom.h-mode->relh+h;
        if(mode->dy1==0 || realdy1!=mode->dy1)
            geom.y+=realdy1;
        else
            geom.y+=mode->origgeom.h-geom.h;
    }
    
    if(XOR_RESIZE)
        moveres_draw_rubberband(mode);
    
    if(mode->reg!=NULL)
        region_request_geom(mode->reg, mode->rqflags, &geom, &mode->geom);
    
    if(!mode->resize_cumulative){
        mode->dx1=0;
        mode->dx2=0;
        mode->dy1=0;
        mode->dy2=0;
        mode->relw=(mode->geom.w-mode->origgeom.w)+mode->relw;
        mode->relh=(mode->geom.h-mode->origgeom.h)+mode->relh;
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
            region_request_geom(reg, mode->rqflags&~REGION_RQGEOM_TRYONLY,
                                &g2, &mode->geom);
        }
        XUngrabServer(ioncore_g.dpy);
    }
    if(apply)
        set_saved(mode, reg);
    
    if(mode->infowin!=NULL){
        ioncore_defer_destroy((Obj*)mode->infowin);
        mode->infowin=NULL;
    }
    destroy_obj((Obj*)mode);
    
    return TRUE;
}


/*}}}*/


/*{{{ Request and other dynfuns */


void region_request_geom(WRegion *reg, int flags, const WRectangle *geom,
                         WRectangle *geomret)
{
    bool tryonly=(flags&REGION_RQGEOM_TRYONLY);
    
    if(REGION_MANAGER(reg)!=NULL){
        region_request_managed_geom(REGION_MANAGER(reg), reg, flags, geom,
                                    geomret);
    }else{
        if(geomret!=NULL)
            *geomret=REGION_GEOM(reg);
        if(!tryonly)
            region_fit(reg, geom);
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
EXTL_EXPORT_AS(WRegion, request_geom)
ExtlTab region_request_geom_extl(WRegion *reg, ExtlTab g)
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
    
    region_request_geom(reg, flags, &geom, &ogeom);
    
    return extl_table_from_rectangle(&ogeom);
}


void region_request_managed_geom(WRegion *mgr, WRegion *reg,
                                 int flags, const WRectangle *geom,
                                 WRectangle *geomret)
{
    CALL_DYN(region_request_managed_geom, mgr,
             (mgr, reg, flags, geom, geomret));
}


void region_request_clientwin_geom(WRegion *mgr, WClientWin *cwin,
                                   int flags, const WRectangle *geom)
{
    CALL_DYN(region_request_clientwin_geom, mgr, (mgr, cwin, flags, geom));
}


void region_request_managed_geom_allow(WRegion *mgr, WRegion *reg,
                                       int flags, const WRectangle *geom,
                                       WRectangle *geomret)
{
    if(geomret!=NULL)
        *geomret=*geom;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fit(reg, geom);
}


void region_request_managed_geom_unallow(WRegion *mgr, WRegion *reg,
                                         int flags, const WRectangle *geom,
                                         WRectangle *geomret)
{
    if(geomret!=NULL)
        *geomret=REGION_GEOM(reg);
}


void region_resize_hints(WRegion *reg, XSizeHints *hints_ret,
                         uint *relw_ret, uint *relh_ret)
{
    hints_ret->flags=0;
    hints_ret->min_width=1;
    hints_ret->min_height=1;
    if(relw_ret!=NULL)
        *relw_ret=REGION_GEOM(reg).w;
    if(relh_ret!=NULL)
        *relh_ret=REGION_GEOM(reg).h;
    {
        CALL_DYN(region_resize_hints, reg, (reg, hints_ret, relw_ret, relh_ret));
    }
}


/*}}}*/


/*{{{ Restore size, maximize, shade */


void frame_restore_size(WFrame *frame, bool horiz, bool vert)
{
    WRectangle geom;
    int rqf=REGION_RQGEOM_WEAK_ALL;

    geom=REGION_GEOM(frame);
    
    if(vert && frame->flags&FRAME_SAVED_VERT){
        geom.y=frame->saved_y;
        geom.h=frame->saved_h;
        rqf&=~(REGION_RQGEOM_WEAK_Y|REGION_RQGEOM_WEAK_H);
    }

    if(horiz && frame->flags&FRAME_SAVED_HORIZ){
        geom.x=frame->saved_x;
        geom.w=frame->saved_w;
        rqf&=~(REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_W);
    }
    
    if((rqf&REGION_RQGEOM_WEAK_ALL)!=REGION_RQGEOM_WEAK_ALL)
        region_request_geom((WRegion*)frame, rqf, &geom, NULL);
}


static void correct_frame_size(WFrame *frame, int *w, int *h)
{
    XSizeHints hints;
    uint relw, relh;
    int wdiff, hdiff;
    
    region_resize_hints((WRegion*)frame, &hints, &relw, &relh);
    
    wdiff=REGION_GEOM(frame).w-relw;
    hdiff=REGION_GEOM(frame).h-relh;
    *w-=wdiff;
    *h-=hdiff;
    xsizehints_correct(&hints, w, h, TRUE);
    *w+=wdiff;
    *h+=hdiff;
}


static bool trymaxv(WFrame *frame, WRegion *mgr, int tryonlyflag)
{
    WRectangle geom=REGION_GEOM(frame), rgeom;
    geom.y=0;
    geom.h=REGION_GEOM(mgr).h;
    
    {
        int dummy_w=geom.w;
        correct_frame_size(frame, &dummy_w, &(geom.h));
    }
    
    region_request_geom((WRegion*)frame, 
                        tryonlyflag|REGION_RQGEOM_VERT_ONLY, 
                        &geom, &rgeom);
    return (abs(rgeom.y-REGION_GEOM(frame).y)>1 ||
            abs(rgeom.h-REGION_GEOM(frame).h)>1);
}


/*EXTL_DOC
 * Attempt to maximize \var{frame} vertically.
 */
EXTL_EXPORT_MEMBER
void frame_maximize_vert(WFrame *frame)
{
    WRegion *mgr=REGION_MANAGER(frame);
    
    if(frame->flags&FRAME_SHADED){
        frame_do_toggle_shade(frame, 0 /* not used */);
        return;
    }
        
    if(mgr==NULL)
        return;

    if(!trymaxv(frame, mgr, REGION_RQGEOM_TRYONLY)){
        /* Could not maximize further, restore */
        frame_restore_size(frame, FALSE, TRUE);
        return;
    }

    trymaxv(frame, mgr, 0);
}


static bool trymaxh(WFrame *frame, WRegion *mgr, int tryonlyflag)
{
    WRectangle geom=REGION_GEOM(frame), rgeom;
    geom.x=0;
    geom.w=REGION_GEOM(mgr).w;
    
    {
        int dummy_h=geom.h;
        correct_frame_size(frame, &(geom.w), &dummy_h);
    }
    
    region_request_geom((WRegion*)frame, 
                        tryonlyflag|REGION_RQGEOM_HORIZ_ONLY, 
                        &geom, &rgeom);
    return (abs(rgeom.x-REGION_GEOM(frame).x)>1 ||
            abs(rgeom.w-REGION_GEOM(frame).w)>1);
}
                   
/*EXTL_DOC
 * Attempt to maximize \var{frame} horizontally.
 */
EXTL_EXPORT_MEMBER
void frame_maximize_horiz(WFrame *frame)
{
    WRegion *mgr=REGION_MANAGER(frame);
    
    if(mgr==NULL)
        return;

    if(!trymaxh(frame, mgr, REGION_RQGEOM_TRYONLY)){
        /* Could not maximize further, restore */
        frame_restore_size(frame, TRUE, FALSE);
        return;
    }
    
    trymaxh(frame, mgr, 0);
}


void frame_do_toggle_shade(WFrame *frame, int shaded_h)
{
    WRectangle geom=REGION_GEOM(frame);

    if(frame->flags&FRAME_SHADED){
        if(!(frame->flags&FRAME_SAVED_VERT))
            return;
        geom.h=frame->saved_h;
    }else{
        if(frame->flags&FRAME_TAB_HIDE)
            return;
        geom.h=shaded_h;
    }
    
    region_request_geom((WRegion*)frame, REGION_RQGEOM_H_ONLY,
                        &geom, NULL);
}


/*EXTL_DOC
 * Is \var{frame} shaded?
 */
EXTL_EXPORT_MEMBER
bool frame_is_shaded(WFrame *frame)
{
    return ((frame->flags&FRAME_SHADED)!=0);
}


/*}}}*/


/*{{{ Misc. */


#define REG_MIN_WH(REG, WHL)                                 \
    XSizeHints hints;                                        \
    uint relwidth, relheight;                                \
    region_resize_hints(reg, &hints, &relwidth, &relheight); \
    return (hints.flags&PMinSize ? hints.min_##WHL : 1);
    
    
uint region_min_h(WRegion *reg)
{
    XSizeHints hints;
    uint relw, relh;
    region_resize_hints(reg, &hints, &relw, &relh);
    return (hints.flags&PMinSize ? hints.min_height : 1)
        +REGION_GEOM(reg).h-relh;
}


uint region_min_w(WRegion *reg)
{
    XSizeHints hints;
    uint relw, relh;
    region_resize_hints(reg, &hints, &relw, &relh);
    return (hints.flags&PMinSize ? hints.min_width : 1)
        +REGION_GEOM(reg).w-relw;
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

