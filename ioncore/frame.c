/*
 * ion/ioncore/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/obj.h>
#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/stringstore.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "window.h"
#include "global.h"
#include "rootwin.h"
#include "focus.h"
#include "event.h"
#include "attach.h"
#include "resize.h"
#include "tags.h"
#include "names.h"
#include "saveload.h"
#include "framep.h"
#include "frame-pointer.h"
#include "frame-draw.h"
#include "sizehint.h"
#include "extlconv.h"
#include "mplex.h"
#include "bindmaps.h"
#include "regbind.h"
#include "gr.h"
#include "genws.h"
#include "activity.h"
#include "llist.h"


extern bool frame_set_background(WFrame *frame, bool set_always);
extern void frame_initialise_gr(WFrame *frame);

static bool frame_initialise_titles(WFrame *frame);
static void frame_free_titles(WFrame *frame);


WHook *frame_managed_changed_hook=NULL;


/*{{{ Destroy/create frame */


bool frame_init(WFrame *frame, WWindow *parent, const WFitParams *fp,
                const char *style)
{
    WRectangle mg;

    if(style==NULL)
        style="frame";
    
    frame->flags=0;
    frame->saved_w=0;
    frame->saved_h=0;
    frame->saved_x=0;
    frame->saved_y=0;
    frame->tab_dragged_idx=-1;
    frame->titles=NULL;
    frame->titles_n=0;
    frame->bar_h=0;
    frame->tr_mode=GR_TRANSPARENCY_DEFAULT;
    frame->brush=NULL;
    frame->bar_brush=NULL;
    frame->style=stringstore_alloc(style);
    
    if(frame->style==STRINGID_NONE)
        return FALSE;

    if(!mplex_init((WMPlex*)frame, parent, fp)){
        stringstore_free(frame->style);
        return FALSE;
    }
    
    frame_initialise_gr(frame);
    frame_initialise_titles(frame);
    
    region_add_bindmap((WRegion*)frame, ioncore_frame_bindmap);

    mplex_managed_geom((WMPlex*)frame, &mg);
    
    if(mg.h<=1)
        frame->flags|=FRAME_SHADED;
    
    ((WRegion*)frame)->flags|=REGION_PLEASE_WARP;
    
    return TRUE;
}


WFrame *create_frame(WWindow *parent, const WFitParams *fp, const char *style)
{
    CREATEOBJ_IMPL(WFrame, frame, (p, parent, fp, style));
}


void frame_deinit(WFrame *frame)
{
    frame_free_titles(frame);
    frame_release_brushes(frame);
    stringstore_free(frame->style);
    mplex_deinit((WMPlex*)frame);
}


/*}}}*/


/*{{{ Tabs */


int frame_tab_at_x(const WFrame *frame, int x)
{
    WRectangle bg;
    int tab, tx;
    
    frame_bar_geom(frame, &bg);
    
    if(x>=bg.x+bg.w || x<bg.x)
        return -1;
    
    tx=bg.x;

    for(tab=0; tab<FRAME_MCOUNT(frame); tab++){
        tx+=frame_nth_tab_w(frame, tab);
        if(x<tx)
            break;
    }
    
    return tab;
}


int frame_nth_tab_x(const WFrame *frame, int n)
{
    uint x=0;
    int i;
    
    for(i=0; i<n; i++)
        x+=frame_nth_tab_w(frame, i);
    
    return x;
}


static int frame_nth_tab_w_iw(const WFrame *frame, int n, bool inner)
{
    WRectangle bg;
    GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
    int m=FRAME_MCOUNT(frame);
    uint w;
    
    frame_bar_geom(frame, &bg);

    if(m==0)
        m=1;

    if(frame->bar_brush!=NULL)
        grbrush_get_border_widths(frame->bar_brush, &bdw);
    
    /* Remove borders */
    w=bg.w-bdw.left-bdw.right-(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)*(m-1);
    
    if(w<=0)
        return 0;
    
    /* Get n:th tab's portion of free area */
    w=(((n+1)*w)/m-(n*w)/m);
    
    /* Add n:th tab's borders back */
    if(!inner){
        w+=(n==0 ? bdw.left : bdw.tb_ileft);
        w+=(n==m-1 ? bdw.right : bdw.tb_iright+bdw.spacing);
    }
            
    return w;
}


int frame_nth_tab_w(const WFrame *frame, int n)
{
    return frame_nth_tab_w_iw(frame, n, FALSE);
}


int frame_nth_tab_iw(const WFrame *frame, int n)
{
    return frame_nth_tab_w_iw(frame, n, TRUE);
}



static void update_attr(WFrame *frame, int i, WRegion *reg)
{
    int flags=0;
    static char *attrs[]={
        "unselected-not_tagged-not_dragged-no_activity",
        "selected-not_tagged-not_dragged-no_activity",
        "unselected-tagged-not_dragged-no_activity",
        "selected-tagged-not_dragged-no_activity",
        "unselected-not_tagged-dragged-no_activity",
        "selected-not_tagged-dragged-no_activity",
        "unselected-tagged-dragged-no_activity",
        "selected-tagged-dragged-no_activity",
        "unselected-not_tagged-not_dragged-activity",
        "selected-not_tagged-not_dragged-activity",
        "unselected-tagged-not_dragged-activity",
        "selected-tagged-not_dragged-activity",
        "unselected-not_tagged-dragged-activity",
        "selected-not_tagged-dragged-activity",
        "unselected-tagged-dragged-activity",
        "selected-tagged-dragged-activity"
    };

    if(i>=frame->titles_n){
        /* Might happen when deinitialising */
        return;
    }
    
    if(reg==FRAME_CURRENT(frame))
        flags|=0x01;
    if(reg!=NULL && reg->flags&REGION_TAGGED)
        flags|=0x02;
    if(i==frame->tab_dragged_idx)
        flags|=0x04;
    if(reg!=NULL && region_is_activity_r(reg))
        flags|=0x08;
    
    frame->titles[i].attr=attrs[flags];
}


void frame_update_attr_nth(WFrame *frame, int i)
{
    WRegion *reg;
    
    if(i<0 || i>=frame->titles_n)
        return;

    update_attr(frame, i, mplex_lnth((WMPlex*)frame, 1, i));
}


static void update_attrs(WFrame *frame)
{
    int i=0;
    WRegion *sub;
    WLListIterTmp tmp;
    
    FRAME_L1_FOR_ALL(sub, frame, tmp){
        update_attr(frame, i, sub);
        i++;
    }
}


static void frame_free_titles(WFrame *frame)
{
    int i;
    
    if(frame->titles!=NULL){
        for(i=0; i<frame->titles_n; i++){
            if(frame->titles[i].text)
                free(frame->titles[i].text);
        }
        free(frame->titles);
        frame->titles=NULL;
    }
    frame->titles_n=0;
}


static void do_init_title(WFrame *frame, int i, WRegion *sub)
{
    frame->titles[i].text=NULL;
    frame->titles[i].iw=frame_nth_tab_iw(frame, i);
    update_attr(frame, i, sub);
}

    
static bool frame_initialise_titles(WFrame *frame)
{
    int i, n=FRAME_MCOUNT(frame);
    
    frame_free_titles(frame);

    if(n==0)
        n=1;
    
    frame->titles=ALLOC_N(GrTextElem, n);
    if(frame->titles==NULL)
        return FALSE;
    frame->titles_n=n;
    
    if(FRAME_MCOUNT(frame)==0){
        do_init_title(frame, 0, NULL);
    }else{
        WLListIterTmp tmp;
        WRegion *sub;
        i=0;
        FRAME_L1_FOR_ALL(sub, frame, tmp){
            do_init_title(frame, i, sub);
            i++;
        }
    }
    
    frame_recalc_bar(frame);

    return TRUE;
}


/*}}}*/


/*{{{ Resize and reparent */


bool frame_fitrep(WFrame *frame, WWindow *par, const WFitParams *fp)
{
    WRectangle old_geom, mg;
    bool wchg=(REGION_GEOM(frame).w!=fp->g.w);
    bool hchg=(REGION_GEOM(frame).h!=fp->g.h);
    
    old_geom=REGION_GEOM(frame);
    
    window_do_fitrep(&(frame->mplex.win), par, &(fp->g));

    mplex_managed_geom((WMPlex*)frame, &mg);
    
    if(hchg){
        if(mg.h<=1){
            frame->flags|=(FRAME_SHADED|FRAME_SAVED_VERT);
            frame->saved_y=old_geom.y;
            frame->saved_h=old_geom.h;
        }else{
            frame->flags&=~FRAME_SHADED;
        }
        frame->flags&=~FRAME_MAXED_VERT;
    }
    
    if(wchg){
        if(mg.w<=1){
            frame->flags|=(FRAME_MIN_HORIZ|FRAME_SAVED_HORIZ);
            frame->saved_x=old_geom.x;
            frame->saved_w=old_geom.w;
        }else{
            frame->flags&=~FRAME_MIN_HORIZ;
        }
        frame->flags&=~FRAME_MAXED_HORIZ;
    }

    if(wchg || hchg){
        mplex_fit_managed((WMPlex*)frame);
        mplex_size_changed((WMPlex*)frame, wchg, hchg);
    }
    
    return TRUE;
}


void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret)
{
    WRectangle subgeom;
    WLListIterTmp tmp;
    WRegion *sub;
    int woff, hoff;
    
    mplex_managed_geom((WMPlex*)frame, &subgeom);
    
    woff=maxof(REGION_GEOM(frame).w-subgeom.w, 0);
    hoff=maxof(REGION_GEOM(frame).h-subgeom.h, 0);

    if(FRAME_CURRENT(frame)!=NULL){
        region_size_hints(FRAME_CURRENT(frame), hints_ret);
        if(!(frame->flags&FRAME_SZH_USEMINMAX))
            hints_ret->flags&=PResizeInc|PBaseSize;
    }else{
        hints_ret->flags=0;
    }
    
    FRAME_L1_FOR_ALL(sub, frame, tmp){
        xsizehints_adjust_for(hints_ret, sub);
    }
    
    if(!(hints_ret->flags&PBaseSize)){
        hints_ret->base_width=0;
        hints_ret->base_height=0;
        hints_ret->flags|=PBaseSize;
    }

    if(!(hints_ret->flags&PMinSize)){
        hints_ret->min_width=0;
        hints_ret->min_height=0;
        hints_ret->flags|=PMinSize;
    }
    
    hints_ret->base_width+=woff;
    hints_ret->base_height+=hoff;
    hints_ret->max_width+=woff;
    hints_ret->max_height+=hoff;
    hints_ret->min_width+=woff;
    hints_ret->min_height+=hoff;
}


/*}}}*/


/*{{{ Focus  */


void frame_inactivated(WFrame *frame)
{
    window_draw((WWindow*)frame, FALSE);
}


void frame_activated(WFrame *frame)
{
    window_draw((WWindow*)frame, FALSE);
}


/*}}}*/


/*{{{ Client window rqgeom */


/* geom parameter==client requested geometry minus border crap */
static void frame_rqgeom_clientwin(WFrame *frame, WClientWin *cwin,
                                   int rqflags, const WRectangle *geom_)
{
    int gravity=NorthWestGravity;
    XSizeHints hints;
    WRectangle off;
    WRegion *par;
    WRectangle geom=*geom_;
    
    if(!(frame->flags&FRAME_FWD_CWIN_RQGEOM)){
        region_managed_rqgeom((WRegion*)frame, (WRegion*)cwin,
                              rqflags, geom_, NULL);
        return;
    }
    
    if(cwin->size_hints.flags&PWinGravity)
        gravity=cwin->size_hints.win_gravity;

    mplex_managed_geom(&frame->mplex, &off);
    off.x=-off.x;
    off.y=-off.y;
    off.w=REGION_GEOM(frame).w-off.w;
    off.h=REGION_GEOM(frame).h-off.h;

    geom.w=maxof(geom.w, 0);
    geom.h=maxof(geom.h, 0);
    geom.w+=off.w;
    geom.h+=off.h;
    
    region_size_hints((WRegion*)frame, &hints);
    xsizehints_correct(&hints, &(geom.w), &(geom.h), TRUE);
    
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


/*}}}*/


/*{{{ Misc. */


bool frame_set_tabbar(WFrame *frame, int sp)
{
    bool set=!(frame->flags&FRAME_TAB_HIDE);
    bool nset=libtu_do_setparam(sp, set);
    
    if(!nset && frame->flags&FRAME_SHADED)
        return set;
    
    if(XOR(nset, set)){
        frame->flags^=FRAME_TAB_HIDE;
        mplex_size_changed(&(frame->mplex), FALSE, TRUE);
        mplex_fit_managed(&(frame->mplex));
        XClearWindow(ioncore_g.dpy, frame->mplex.win.win);
        window_draw((WWindow*)frame, TRUE);
    }
    
    return !(frame->flags&FRAME_TAB_HIDE);
}


/*EXTL_DOC
 * Set tab-bar visibility according to the parameter \var{how} 
 * (set/unset/toggle). Resulting state is returned, which may not be
 * what was requested.
 */
EXTL_EXPORT_AS(WFrame, set_tabbar)
bool frame_set_tabbar_extl(WFrame *frame, const char *how)
{
    return frame_set_tabbar(frame, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{frame}'s tab-bar visible?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool frame_is_tabbar(WFrame *frame)
{
    return (frame->flags&FRAME_TAB_HIDE);
}


static void frame_do_toggle_shade(WFrame *frame, int shaded_h)
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
    
    region_rqgeom((WRegion*)frame, REGION_RQGEOM_H_ONLY, &geom, NULL);
}


bool frame_set_shaded(WFrame *frame, int sp)
{
    bool set=(frame->flags&FRAME_SHADED);
    bool nset=libtu_do_setparam(sp, set);
    GrBorderWidths bdw;
    int h=frame->bar_h;

    if(!XOR(nset, set))
        return nset;
    
    if(!(frame->flags&FRAME_BAR_OUTSIDE) && frame->brush!=NULL){
        grbrush_get_border_widths(frame->brush, &bdw);
        h+=bdw.top+bdw.bottom+2*bdw.spacing;
    }

    frame_do_toggle_shade((WFrame*)frame, h);
    
    return (frame->flags&FRAME_SHADED);
}


/*EXTL_DOC
 * Set shading state according to the parameter \var{how} 
 * (set/unset/toggle). Resulting state is returned, which may not be
 * what was requested.
 */
EXTL_EXPORT_AS(WFrame, set_shaded)
bool frame_set_shaded_extl(WFrame *frame, const char *how)
{
    return frame_set_shaded(frame, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{frame} shaded?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool frame_is_shaded(WFrame *frame)
{
    return ((frame->flags&FRAME_SHADED)!=0);
}


void frame_managed_notify(WFrame *frame, WRegion *sub)
{
    /* TODO: Should only draw/update the affected tab.*/
    update_attrs(frame);
    frame_recalc_bar(frame);
    frame_draw_bar(frame, FALSE);
}


static void frame_size_changed_default(WFrame *frame,
                                       bool wchg, bool hchg)
{
    if(wchg)
        frame_recalc_bar(frame);
    /* We should get a request from X to draw the frame... */
}


static void frame_managed_changed(WFrame *frame, int mode, bool sw,
                                  WRegion *reg)
{
    bool need_draw=TRUE;
    
    if(mode!=MPLEX_CHANGE_SWITCHONLY)
        frame_initialise_titles(frame);
    else
        update_attrs(frame);

    if(sw)
        need_draw=!frame_set_background(frame, FALSE);
    
    if(need_draw)
        frame_draw_bar(frame, mode!=MPLEX_CHANGE_SWITCHONLY);

    mplex_call_changed_hook((WMPlex*)frame,
                            frame_managed_changed_hook,
                            mode, sw, reg);
}


#define EMPTY_SHOULD_BE_DESTROYED(FRAME) \
    ((FRAME)->flags&FRAME_DEST_EMPTY && FRAME_MCOUNT(FRAME)==0 && \
     !OBJ_IS_BEING_DESTROYED(frame))


static void frame_destroy_empty(WFrame *frame)
{
    if(EMPTY_SHOULD_BE_DESTROYED(frame))
        destroy_obj((Obj*)frame);
}


void frame_managed_remove(WFrame *frame, WRegion *reg)
{
    mplex_managed_remove((WMPlex*)frame, reg);
    if(EMPTY_SHOULD_BE_DESTROYED(frame)){
        mainloop_defer_action((Obj*)frame, 
                              (WDeferredAction*)frame_destroy_empty);
    }
}


int frame_default_index(WFrame *frame)
{
    if(ioncore_g.frame_add_last)
        return LLIST_INDEX_LAST;
    else
        return LLIST_INDEX_AFTER_CURRENT;
}


/*}}}*/


/*{{{ Save/load */


ExtlTab frame_get_configuration(WFrame *frame)
{
    ExtlTab tab=mplex_get_configuration(&frame->mplex);
    
    extl_table_sets_i(tab, "flags", frame->flags);
    
    if(frame->flags&FRAME_SAVED_VERT){
        extl_table_sets_i(tab, "saved_y", frame->saved_y);
        extl_table_sets_i(tab, "saved_h", frame->saved_h);
    }
    if(frame->flags&FRAME_SAVED_HORIZ){
        extl_table_sets_i(tab, "saved_x", frame->saved_x);
        extl_table_sets_i(tab, "saved_w", frame->saved_w);
    }
    
    extl_table_sets_s(tab, "frame_style", stringstore_get(frame->style));
    
    return tab;
}



void frame_do_load(WFrame *frame, ExtlTab tab)
{
    int flags=0;
    int p=0, s=0;
    
    extl_table_gets_i(tab, "flags", &flags);
    
    if(flags&FRAME_TAB_HIDE){
        frame->flags&=~FRAME_SHADED;
        frame->flags|=FRAME_TAB_HIDE;
    }

    frame->flags|=flags&(FRAME_DEST_EMPTY|
                         FRAME_MAXED_VERT|
                         FRAME_MAXED_HORIZ|
                         FRAME_SZH_USEMINMAX);
    
    if(extl_table_gets_i(tab, "saved_x", &p) &&
       extl_table_gets_i(tab, "saved_w", &s)){
        frame->saved_x=p;
        frame->saved_w=s;
        frame->flags|=FRAME_SAVED_HORIZ;
    }

    if(extl_table_gets_i(tab, "saved_y", &p) &&
       extl_table_gets_i(tab, "saved_h", &s)){
        frame->saved_y=p;
        frame->saved_h=s;
        frame->flags|=FRAME_SAVED_VERT;
    }
    
    mplex_load_contents(&frame->mplex, tab);
}


WRegion *frame_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    char *style=NULL;
    WFrame *frame;
    
    extl_table_gets_s(tab, "frame_style", &style);
    
    frame=create_frame(par, fp, style);
    
    if(frame!=NULL)
        frame_do_load(frame, tab);
    
    if(style!=NULL)
        free(style);
    
    return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab frame_dynfuntab[]={
    {region_size_hints, frame_resize_hints},

    {mplex_managed_changed, frame_managed_changed},
    {mplex_size_changed, frame_size_changed_default},
    {region_managed_notify, frame_managed_notify},
    
    {region_activated, frame_activated},
    {region_inactivated, frame_inactivated},

    {(DynFun*)window_press, (DynFun*)frame_press},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)frame_get_configuration},

    {window_draw, 
     frame_draw_default},
    {frame_draw_bar, 
     frame_draw_bar_default},
    {frame_recalc_bar, 
     frame_recalc_bar_default},
    {frame_bar_geom, 
     frame_bar_geom_default},
    {frame_border_geom, 
     frame_border_geom_default},
    {frame_border_inner_geom, 
     frame_border_inner_geom_default},
    {mplex_managed_geom, 
     frame_managed_geom_default},

    {region_updategr, 
     frame_updategr},

    {(DynFun*)region_fitrep,
     (DynFun*)frame_fitrep},

    {region_rqgeom_clientwin, frame_rqgeom_clientwin},

    {region_managed_remove, frame_managed_remove},
    
    {(DynFun*)mplex_default_index,
     (DynFun*)frame_default_index},
    
    {frame_brushes_updated, 
     frame_brushes_updated_default},
    
    END_DYNFUNTAB
};
                                       

EXTL_EXPORT
IMPLCLASS(WFrame, WMPlex, frame_deinit, frame_dynfuntab);


/*}}}*/
