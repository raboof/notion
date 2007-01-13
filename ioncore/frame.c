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
#include <libtu/map.h>
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
#include "activity.h"
#include "llist.h"
#include "framedpholder.h"
#include "return.h"


extern bool frame_set_background(WFrame *frame, bool set_always);
extern void frame_initialise_gr(WFrame *frame);

static bool frame_initialise_titles(WFrame *frame);
static void frame_free_titles(WFrame *frame);

static void frame_add_mode_bindmaps(WFrame *frame);


WHook *frame_managed_changed_hook=NULL;

#define IS_FLOATING_MODE(FRAME) \
    ((FRAME)->mode==FRAME_MODE_FLOATING || (FRAME)->mode==FRAME_MODE_TRANSIENT)
#define FORWARD_CWIN_RQGEOM(FRAME) IS_FLOATING_MODE(FRAME)
#define USE_MINMAX(FRAME) IS_FLOATING_MODE(FRAME)
#define DEST_EMPTY(FRAME) IS_FLOATING_MODE(FRAME)


/*{{{ Destroy/create frame */


bool frame_init(WFrame *frame, WWindow *parent, const WFitParams *fp,
                WFrameMode mode)
{
    WRectangle mg;

    frame->flags=0;
    frame->saved_w=0;
    frame->saved_h=0;
    frame->saved_x=0;
    frame->saved_y=0;
    frame->tab_dragged_idx=-1;
    frame->titles=NULL;
    frame->titles_n=0;
    frame->bar_h=0;
    frame->bar_w=fp->g.w;
    frame->tr_mode=GR_TRANSPARENCY_DEFAULT;
    frame->brush=NULL;
    frame->bar_brush=NULL;
    frame->mode=mode;
    frame->tab_min_w=0;
    frame->bar_max_width_q=1.0;
    
    if(!mplex_init((WMPlex*)frame, parent, fp))
        return FALSE;
    
    frame_initialise_gr(frame);
    frame_initialise_titles(frame);
    
    region_add_bindmap((WRegion*)frame, ioncore_frame_bindmap);
    region_add_bindmap((WRegion*)frame, ioncore_mplex_bindmap);
    
    frame_add_mode_bindmaps(frame);
    
    mplex_managed_geom((WMPlex*)frame, &mg);
    
    if(mg.h<=1)
        frame->flags|=FRAME_SHADED;
    
    ((WRegion*)frame)->flags|=REGION_PLEASE_WARP;
    
    return TRUE;
}


WFrame *create_frame(WWindow *parent, const WFitParams *fp, WFrameMode mode)
{
    CREATEOBJ_IMPL(WFrame, frame, (p, parent, fp, mode));
}


void frame_deinit(WFrame *frame)
{
    frame_free_titles(frame);
    frame_release_brushes(frame);
    mplex_deinit((WMPlex*)frame);
}


/*}}}*/


/*{{{ Mode switching */


static void frame_add_mode_bindmaps(WFrame *frame)
{
    WFrameMode mode=frame->mode;
    
    if(mode==FRAME_MODE_TILED || mode==FRAME_MODE_TILED_ALT){
	region_add_bindmap((WRegion*)frame, ioncore_mplex_toplevel_bindmap);
	region_add_bindmap((WRegion*)frame, ioncore_frame_toplevel_bindmap);
        region_add_bindmap((WRegion*)frame, ioncore_frame_tiled_bindmap);
    }else if(mode==FRAME_MODE_FLOATING){
	region_add_bindmap((WRegion*)frame, ioncore_mplex_toplevel_bindmap);
	region_add_bindmap((WRegion*)frame, ioncore_frame_toplevel_bindmap);
        region_add_bindmap((WRegion*)frame, ioncore_frame_floating_bindmap);
    }else if(mode==FRAME_MODE_TRANSIENT){
        region_add_bindmap((WRegion*)frame, ioncore_frame_transient_bindmap);
    }
}


void frame_set_mode(WFrame *frame, WFrameMode mode)
{
    if(frame->mode==mode)
        return;
    
    frame_clear_shape(frame);
    
    frame_release_brushes(frame);
    
    region_remove_bindmap((WRegion*)frame, ioncore_mplex_toplevel_bindmap);
    region_remove_bindmap((WRegion*)frame, ioncore_frame_toplevel_bindmap);
    region_remove_bindmap((WRegion*)frame, ioncore_frame_tiled_bindmap);
    region_remove_bindmap((WRegion*)frame, ioncore_frame_floating_bindmap);
    region_remove_bindmap((WRegion*)frame, ioncore_frame_transient_bindmap);
    
    frame->mode=mode;

    frame_add_mode_bindmaps(frame);
    
    frame_initialise_gr(frame);
    
    mplex_fit_managed(&frame->mplex);
    frame_recalc_bar(frame);
    frame_set_background(frame, TRUE);
}


WFrameMode frame_mode(WFrame *frame)
{
    return frame->mode;
}


StringIntMap frame_modes[]={
    {"tiled", FRAME_MODE_TILED},
    {"tiled-alt", FRAME_MODE_TILED_ALT},
    {"floating", FRAME_MODE_FLOATING},
    {"transient", FRAME_MODE_TRANSIENT},
    END_STRINGINTMAP
};


/*EXTL_DOC
 * Get frame mode.
 */
EXTL_EXPORT_AS(WFrame, mode)
const char *frame_mode_extl(WFrame *frame)
{
    return stringintmap_key(frame_modes, frame->mode, NULL);
}


/*EXTL_DOC
 * Set frame mode.
 */
EXTL_EXPORT_AS(WFrame, set_mode)
bool frame_set_mode_extl(WFrame *frame, const char *modestr)
{
    WFrameMode mode;
    int idx;
    
    idx=stringintmap_ndx(frame_modes, modestr);
    if(idx<0)
	return FALSE;
    
    frame_set_mode(frame, frame_modes[idx].value);
    
    return TRUE;
}


/*}}}*/


/*{{{ Tabs */


int frame_tab_at_x(WFrame *frame, int x)
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


int frame_nth_tab_x(WFrame *frame, int n)
{
    uint x=0;
    int i;
    
    for(i=0; i<n; i++)
        x+=frame_nth_tab_w(frame, i);
    
    return x;
}


static int frame_nth_tab_w_iw(WFrame *frame, int n, bool inner)
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


int frame_nth_tab_w(WFrame *frame, int n)
{
    return frame_nth_tab_w_iw(frame, n, FALSE);
}


int frame_nth_tab_iw(WFrame *frame, int n)
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

    update_attr(frame, i, mplex_mx_nth((WMPlex*)frame, i));
}


static void update_attrs(WFrame *frame)
{
    int i=0;
    WRegion *sub;
    WLListIterTmp tmp;
    
    FRAME_MX_FOR_ALL(sub, frame, tmp){
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
        FRAME_MX_FOR_ALL(sub, frame, tmp){
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


void frame_size_hints(WFrame *frame, WSizeHints *hints_ret)
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
        if(!USE_MINMAX(frame)){
            hints_ret->max_set=0;
            hints_ret->min_set=0;
            hints_ret->base_set=0;
            hints_ret->aspect_set=0;
            hints_ret->no_constrain=FALSE;
            /*hints_ret->no_constrain=TRUE;*/
        }
    }else{
        sizehints_clear(hints_ret);
    }
    
    FRAME_MX_FOR_ALL(sub, frame, tmp){
        sizehints_adjust_for(hints_ret, sub);
    }
    
    if(!hints_ret->base_set){
        hints_ret->base_width=0;
        hints_ret->base_height=0;
        hints_ret->base_set=TRUE;
    }

    if(!hints_ret->min_set){
        hints_ret->min_width=0;
        hints_ret->min_height=0;
        hints_ret->min_set=TRUE;
    }
    
    hints_ret->base_width+=woff;
    hints_ret->base_height+=hoff;
    hints_ret->max_width+=woff;
    hints_ret->max_height+=hoff;
    hints_ret->min_width+=woff;
    hints_ret->min_height+=hoff;
    
    if(frame->barmode==FRAME_BAR_SHAPED){
        int f=frame->flags&(FRAME_SHADED|FRAME_SHADED_TOGGLE);
        
        if(f==FRAME_SHADED || f==FRAME_SHADED_TOGGLE){
            hints_ret->min_height=frame->bar_h;
            hints_ret->max_height=frame->bar_h;
            hints_ret->base_height=frame->bar_h;
            if(!hints_ret->max_set){
                hints_ret->max_width=INT_MAX;
                hints_ret->max_set=TRUE;
            }
        }
    }
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


static void frame_managed_rqgeom_absolute(WFrame *frame, WRegion *sub,
                                          const WRQGeomParams *rq,
                                          WRectangle *geomret)
{
    if(!FORWARD_CWIN_RQGEOM(frame)){
        region_managed_rqgeom_absolute_default((WRegion*)frame, sub, 
                                               rq, geomret);
    }else{
        WRQGeomParams rq2=RQGEOMPARAMS_INIT;
        int gravity=ForgetGravity;
        WRectangle off;
        WRegion *par;
        
        rq2.geom=rq->geom;
        rq2.flags=rq->flags&(REGION_RQGEOM_WEAK_ALL
                             |REGION_RQGEOM_TRYONLY
                             |REGION_RQGEOM_ABSOLUTE);

        if(rq->flags&REGION_RQGEOM_GRAVITY)
            gravity=rq->gravity;
    
        mplex_managed_geom(&frame->mplex, &off);
        off.x=-off.x;
        off.y=-off.y;
        off.w=REGION_GEOM(frame).w-off.w;
        off.h=REGION_GEOM(frame).h-off.h;

        rq2.geom.w=maxof(rq2.geom.w+off.w, 0);
        rq2.geom.h=maxof(rq2.geom.h+off.h, 0);
    
        /*region_size_hints_correct((WRegion*)frame, &(geom.w), &(geom.h), TRUE);*/
    
        /* If WEAK_? is set, then geom.(x|y) is root-relative as it was not 
         * requested by the client and clientwin_handle_configure_request has
         * no better guess. Otherwise the coordinates are those requested by 
         * the client (modulo borders/gravity) and we interpret them to be 
         * root-relative coordinates for this frame modulo gravity.
         */
        if(rq->flags&REGION_RQGEOM_WEAK_X)
            rq2.geom.x+=off.x;
        else
            rq2.geom.x+=xgravity_deltax(gravity, -off.x, off.x+off.w);
	
        if(rq->flags&REGION_RQGEOM_WEAK_Y)
            rq2.geom.y+=off.y;
        else
            rq2.geom.y+=xgravity_deltay(gravity, -off.y, off.y+off.h);
        
        region_rqgeom((WRegion*)frame, &rq2, geomret);
        
        if(geomret!=NULL){
            geomret->x-=off.x;
            geomret->y-=off.y;
            geomret->w-=off.w;
            geomret->h-=off.h;
        }
    }
}


/*}}}*/


/*{{{ Frame recreate pholder stuff */


static WFramedPHolder *frame_make_recreate_pholder(WFrame *frame)
{
    WPHolder *ph;
    WFramedPHolder *fph;
    WFramedParam fparam=FRAMEDPARAM_INIT;
    
    ph=region_make_return_pholder((WRegion*)frame);
    
    if(ph==NULL)
        return NULL;
        
    fparam.mode=frame->mode;
    
    fph=create_framedpholder(ph, &fparam);
    
    if(fph==NULL){
        destroy_obj((Obj*)ph);
        return NULL;
    }
    
    return fph;
}


static void mplex_flatten_phs(WMPlex *mplex)
{
    WLListNode *node;
    WLListIterTmp tmp;

    FOR_ALL_NODES_ON_LLIST(node, mplex->mx_list, tmp){
        WMPlexPHolder *last=(mplex->mx_phs==NULL ? NULL : mplex->mx_phs->prev);
        mplex_move_phs(mplex, node, last, NULL);
    }
}


static void frame_modify_pholders(WFrame *frame)
{
    WFramedPHolder *fph;
    WMPlexPHolder *phs, *ph;
    
    mplex_flatten_phs(&frame->mplex);
    
    if(frame->mplex.mx_phs==NULL)
        return;
    
    fph=frame_make_recreate_pholder(frame);
    
    if(fph==NULL)
        return;
    
    phs=frame->mplex.mx_phs;
    frame->mplex.mx_phs=NULL;
    
    phs->recreate_pholder=fph;
    
    for(ph=phs; ph!=NULL; ph=ph->next)
        watch_reset(&ph->mplex_watch);
}


/*}}}*/


/*{{{ Misc. */


bool frame_set_shaded(WFrame *frame, int sp)
{
    bool set=(frame->flags&FRAME_SHADED);
    bool nset=libtu_do_setparam(sp, set);
    WRQGeomParams rq=RQGEOMPARAMS_INIT;
    GrBorderWidths bdw;
    int h;

    if(!XOR(nset, set))
        return nset;

    rq.flags=REGION_RQGEOM_H_ONLY;
    rq.geom=REGION_GEOM(frame);
        
    if(!nset){
        if(!(frame->flags&FRAME_SAVED_VERT))
            return FALSE;
        rq.geom.h=frame->saved_h;
    }else{
        if(frame->barmode==FRAME_BAR_NONE){
            return FALSE;
        }else if(frame->barmode==FRAME_BAR_SHAPED){
            rq.geom.h=frame->bar_h;
        }else{
            WRectangle tmp;
            
            frame_border_inner_geom(frame, &tmp);
            
            rq.geom.h=rq.geom.h-tmp.h;
        }
    }
    
    frame->flags|=FRAME_SHADED_TOGGLE;
    
    region_rqgeom((WRegion*)frame, &rq, NULL);
    
    frame->flags&=~FRAME_SHADED_TOGGLE;
    
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


bool frame_set_numbers(WFrame *frame, int sp)
{
    bool set=frame->flags&FRAME_SHOW_NUMBERS;
    bool nset=libtu_do_setparam(sp, set);
    
    if(XOR(nset, set)){
        frame->flags^=FRAME_SHOW_NUMBERS;
        frame_recalc_bar(frame);
        frame_draw_bar(frame, TRUE);
    }
    
    return frame->flags&FRAME_SHOW_NUMBERS;
}


/*EXTL_DOC
 * Control whether tabs show numbers (set/unset/toggle). 
 * Resulting state is returned, which may not be what was 
 * requested.
 */
EXTL_EXPORT_AS(WFrame, set_numbers)
bool frame_set_numbers_extl(WFrame *frame, const char *how)
{
    return frame_set_numbers(frame, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Does \var{frame} show numbers for tabs?
 */
bool frame_is_numbers(WFrame *frame)
{
    return frame->flags&FRAME_SHOW_NUMBERS;
}


void frame_managed_notify(WFrame *frame, WRegion *sub, WRegionNotify how)
{
    update_attrs(frame);
    frame_recalc_bar(frame);
    frame_draw_bar(frame, FALSE);
}


static void frame_size_changed_default(WFrame *frame,
                                       bool wchg, bool hchg)
{
    int bar_w=frame->bar_w;
    
    if(wchg)
        frame_recalc_bar(frame);
    
    if(frame->barmode==FRAME_BAR_SHAPED &&
       ((!wchg && hchg) || (wchg && bar_w==frame->bar_w))){
        frame_set_shape(frame);
    }
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


#define EMPTY_AND_SHOULD_BE_DESTROYED(FRAME) \
    (DEST_EMPTY(frame) && FRAME_MCOUNT(FRAME)==0 && \
     !OBJ_IS_BEING_DESTROYED(frame))


static void frame_destroy_empty(WFrame *frame)
{
    if(EMPTY_AND_SHOULD_BE_DESTROYED(frame)){
        frame_modify_pholders(frame);
        destroy_obj((Obj*)frame);
    }
}


void frame_managed_remove(WFrame *frame, WRegion *reg)
{
    mplex_managed_remove((WMPlex*)frame, reg);
    if(EMPTY_AND_SHOULD_BE_DESTROYED(frame)){
        mainloop_defer_action((Obj*)frame, 
                              (WDeferredAction*)frame_destroy_empty);
    }
}


int frame_default_index(WFrame *frame)
{
    return ioncore_g.frame_default_index;
}


/*}}}*/


/*{{{ Save/load */


ExtlTab frame_get_configuration(WFrame *frame)
{
    ExtlTab tab=mplex_get_configuration(&frame->mplex);
    
    extl_table_sets_i(tab, "mode", frame->mode);

    if(frame->flags&FRAME_SAVED_VERT){
        extl_table_sets_i(tab, "saved_y", frame->saved_y);
        extl_table_sets_i(tab, "saved_h", frame->saved_h);
    }

    if(frame->flags&FRAME_SAVED_HORIZ){
        extl_table_sets_i(tab, "saved_x", frame->saved_x);
        extl_table_sets_i(tab, "saved_w", frame->saved_w);
    }
    
    return tab;
}



void frame_do_load(WFrame *frame, ExtlTab tab)
{
    int flags=0;
    int p=0, s=0;
    
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
    int mode=FRAME_MODE_UNKNOWN;
    WFrame *frame;
    
    if(!extl_table_gets_i(tab, "mode", &mode)){
        #warning "TODO: Remove backwards compatibility hack"
        char *style=NULL;
        if(extl_table_gets_s(tab, "frame_style", &style)){
            if(strcmp(style, "frame-tiled")==0)
                mode=FRAME_MODE_TILED;
            else if(strcmp(style, "frame-floating")==0)
                mode=FRAME_MODE_FLOATING;
            else if(strcmp(style, "frame-transientcontainer")==0)
                mode=FRAME_MODE_TRANSIENT;
            free(style);
        }
    }
    
    frame=create_frame(par, fp, mode);
    
    if(frame!=NULL)
        frame_do_load(frame, tab);
    
    return (WRegion*)frame;
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab frame_dynfuntab[]={
    {region_size_hints, frame_size_hints},

    {mplex_managed_changed, frame_managed_changed},
    {mplex_size_changed, frame_size_changed_default},
    {region_managed_notify, frame_managed_notify},
    
    {region_activated, frame_activated},
    {region_inactivated, frame_inactivated},

    {(DynFun*)window_press, (DynFun*)frame_press},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)frame_get_configuration},

    {window_draw, 
     frame_draw},
    
    {mplex_managed_geom, 
     frame_managed_geom},

    {region_updategr, 
     frame_updategr},

    {(DynFun*)region_fitrep,
     (DynFun*)frame_fitrep},

    {region_managed_rqgeom_absolute, 
     frame_managed_rqgeom_absolute},

    {region_managed_remove, frame_managed_remove},
    
    {(DynFun*)mplex_default_index,
     (DynFun*)frame_default_index},
    
    END_DYNFUNTAB
};
                                       

EXTL_EXPORT
IMPLCLASS(WFrame, WMPlex, frame_deinit, frame_dynfuntab);


/*}}}*/
