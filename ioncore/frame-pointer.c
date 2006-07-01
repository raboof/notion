/*
 * ion/ioncore/frame-pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>

#include "common.h"
#include "global.h"
#include "pointer.h"
#include "cursor.h"
#include "focus.h"
#include "attach.h"
#include "resize.h"
#include "grab.h"
#include "frame.h"
#include "framep.h"
#include "frame-pointer.h"
#include "frame-draw.h"
#include "bindmaps.h"
#include "infowin.h"
#include "rectangle.h"
#include "xwindow.h"
#include "names.h"
#include "presize.h"
#include "llist.h"


static int p_tab_x=0, p_tab_y=0, p_tabnum=-1;
static WInfoWin *tabdrag_infowin=NULL;


/*{{{ Frame press */


static WRegion *sub_at_tab(WFrame *frame)
{
    return mplex_lnth((WMPlex*)frame, 1, p_tabnum);
}


int frame_press(WFrame *frame, XButtonEvent *ev, WRegion **reg_ret)
{
    WRegion *sub=NULL;
    WRectangle g;
    
    p_tabnum=-1;

    window_p_resize_prepare((WWindow*)frame, ev);
    
    /* Check tab */
    
    frame_bar_geom(frame, &g);
    
    /* Borders act like tabs at top of the parent region */
    if(REGION_GEOM(frame).y==0){
        g.h+=g.y;
        g.y=0;
    }

    if(rectangle_contains(&g, ev->x, ev->y)){
        p_tabnum=frame_tab_at_x(frame, ev->x);

        region_rootpos((WRegion*)frame, &p_tab_x, &p_tab_y);
        p_tab_x+=frame_nth_tab_x(frame, p_tabnum);
        p_tab_y+=g.y;
        
        sub=mplex_lnth(&(frame->mplex), 1, p_tabnum);

        if(reg_ret!=NULL)
            *reg_ret=sub;
        
        return FRAME_AREA_TAB;
    }else{
        WLListIterTmp tmp;
        FRAME_L1_FOR_ALL(sub, frame, tmp){
            p_tabnum++;
            if(sub==FRAME_CURRENT(frame))
                break;
        }
        
        if(sub!=NULL){
            p_tab_x=ev->x_root-frame_nth_tab_w(frame, p_tabnum)/2;
            p_tab_y=ev->y_root-frame->bar_h/2;
        }else{
            p_tabnum=-1;
        }
    }
    

    /* Check border */
    
    frame_border_inner_geom(frame, &g);
    
    if(rectangle_contains(&g, ev->x, ev->y))
        return FRAME_AREA_CLIENT;
    
    return FRAME_AREA_BORDER;
}


/*}}}*/


/*{{{ Tab drag */


static ExtlExportedFn *tabdrag_safe_fns[]={
    (ExtlExportedFn*)&mplex_switch_nth,
    (ExtlExportedFn*)&mplex_switch_next,
    (ExtlExportedFn*)&mplex_switch_prev,
    NULL
};

static ExtlSafelist tabdrag_safelist=EXTL_SAFELIST_INIT(tabdrag_safe_fns);


#define BUTTONS_MASK \
    (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)


static bool tabdrag_kbd_handler(WRegion *reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    WBinding *binding=NULL;
    WBindmap **bindptr;
    
    if(ev->type==KeyRelease)
        return FALSE;
    
    assert(reg!=NULL);

    binding=bindmap_lookup_binding(ioncore_rootwin_bindmap, BINDING_KEYPRESS,
                                   ev->state&~BUTTONS_MASK, ev->keycode);
    
    if(binding!=NULL && binding->func!=extl_fn_none()){
        extl_protect(&tabdrag_safelist);
        extl_call(binding->func, "o", NULL, region_screen_of(reg));
        extl_unprotect(&tabdrag_safelist);
    }
    
    return FALSE;
}


static void setup_dragwin(WFrame *frame, uint tab)
{
    WRectangle g;
    WRootWin *rw;
    WFitParams fp;
    const char *style;
    char *tab_style;
    
    assert(tabdrag_infowin==NULL);
    
    rw=region_rootwin_of((WRegion*)frame);
    
    fp.mode=REGION_FIT_EXACT;
    fp.g.x=p_tab_x;
    fp.g.y=p_tab_y;
    fp.g.w=frame_nth_tab_w(frame, tab);
    fp.g.h=frame->bar_h;
    
    assert(frame->style!=STRINGID_NONE);
    
    style=stringstore_get(frame->style);
    
    assert(style!=NULL);
    
    tab_style=scat("tab-", style);
    
    if(tab_style==NULL)
        return;
    
    tabdrag_infowin=create_infowin((WWindow*)rw, &fp, tab_style);
    
    free(tab_style);
    
    if(tabdrag_infowin==NULL)
        return;
    
    infowin_set_attr2(tabdrag_infowin, (REGION_IS_ACTIVE(frame) 
                                        ? "active" : "inactive"),
                      frame->titles[tab].attr);
    
    if(frame->titles[tab].text!=NULL){
        char *buf=INFOWIN_BUFFER(tabdrag_infowin);
        strncpy(buf, frame->titles[tab].text, INFOWIN_BUFFER_LEN-1);
        buf[INFOWIN_BUFFER_LEN-1]='\0';
    }
}


static void p_tabdrag_motion(WFrame *frame, XMotionEvent *ev,
                             int dx, int dy)
{
    WRootWin *rootwin=region_rootwin_of((WRegion*)frame);

    p_tab_x+=dx;
    p_tab_y+=dy;
    
    if(tabdrag_infowin!=NULL){
        WRectangle g;
        g.x=p_tab_x;
        g.y=p_tab_y;
        g.w=REGION_GEOM(tabdrag_infowin).w;
        g.h=REGION_GEOM(tabdrag_infowin).h;
        region_fit((WRegion*)tabdrag_infowin, &g, REGION_FIT_EXACT);
    }
}


static void p_tabdrag_begin(WFrame *frame, XMotionEvent *ev,
                            int dx, int dy)
{
    WRootWin *rootwin=region_rootwin_of((WRegion*)frame);

    if(p_tabnum<0)
        return;
    
    ioncore_change_grab_cursor(IONCORE_CURSOR_DRAG);
        
    setup_dragwin(frame, p_tabnum);
    
    frame->tab_dragged_idx=p_tabnum;
    frame_update_attr_nth(frame, p_tabnum);

    frame_draw_bar(frame, FALSE);
    
    p_tabdrag_motion(frame, ev, dx, dy);
    
    if(tabdrag_infowin!=NULL)
        window_map((WWindow*)tabdrag_infowin);
}


static WRegion *fnd(Window root, int x, int y)
{
    Window win=root;
    int dstx, dsty;
    WRegion *reg=NULL;
    WWindow *w=NULL;
    WScreen *scr;
    
    FOR_ALL_SCREENS(scr){
        if(region_root_of((WRegion*)scr)==root &&
           rectangle_contains(&REGION_GEOM(scr), x, y)){
            break;
        }
    }
    
    w=(WWindow*)scr;
    
    while(w!=NULL){
        if(HAS_DYN(w, region_handle_drop))
            reg=(WRegion*)w;
        
        if(!XTranslateCoordinates(ioncore_g.dpy, root, w->win,
                                  x, y, &dstx, &dsty, &win)){
            break;
        }
        
        w=XWINDOW_REGION_OF_T(win, WWindow);
        /*x=dstx;
        y=dsty;*/
    }

    return reg;
}


static bool drop_ok(WRegion *mgr, WRegion *reg)
{
    WRegion *reg2=mgr;
    for(reg2=mgr; reg2!=NULL; reg2=region_manager(reg2)){
        if(reg2==reg)
            goto err;
    }
    
    for(reg2=REGION_PARENT_REG(mgr); reg2!=NULL; reg2=REGION_PARENT_REG(reg2)){
        if(reg2==reg)
            goto err;
    }
    
    return TRUE;
    
err:
    warn(TR("Attempt to make region %s manage its ancestor %s."),
         region_name(mgr), region_name(reg));
    return FALSE;
}


static void tabdrag_deinit(WFrame *frame)
{
    int idx=frame->tab_dragged_idx;
    frame->tab_dragged_idx=-1;
    frame_update_attr_nth(frame, idx);
    
    if(tabdrag_infowin!=NULL){
        destroy_obj((Obj*)tabdrag_infowin);
        tabdrag_infowin=NULL;
    }
}


static void tabdrag_killed(WFrame *frame)
{
    tabdrag_deinit(frame);
    if(!OBJ_IS_BEING_DESTROYED(frame))
        frame_draw_bar(frame, TRUE);
}


static void p_tabdrag_end(WFrame *frame, XButtonEvent *ev)
{
    WRegion *sub=NULL;
    WRegion *dropped_on;
    Window win=None;

    sub=sub_at_tab(frame);
    
    tabdrag_deinit(frame);
    
    /* Must be same root window */
    if(sub==NULL || ev->root!=region_root_of(sub))
        return;
    
    dropped_on=fnd(ev->root, ev->x_root, ev->y_root);

    if(dropped_on==NULL || dropped_on==(WRegion*)frame || 
       dropped_on==sub || !drop_ok(dropped_on, sub)){
        frame_draw_bar(frame, TRUE);
        return;
    }
    
    if(region_handle_drop(dropped_on, p_tab_x, p_tab_y, sub))
        region_goto(dropped_on);
    else
        frame_draw_bar(frame, TRUE);
}


/*EXTL_DOC
 * Start dragging the tab that the user pressed on with the pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action with area ''tab''.
 */
EXTL_EXPORT_MEMBER
void frame_p_tabdrag(WFrame *frame)
{
    if(p_tabnum<0)
        return;
    
    ioncore_set_drag_handlers((WRegion*)frame,
                        (WMotionHandler*)p_tabdrag_begin,
                        (WMotionHandler*)p_tabdrag_motion,
                        (WButtonHandler*)p_tabdrag_end,
                        tabdrag_kbd_handler,
                        (GrabKilledHandler*)tabdrag_killed);
}


/*}}}*/


/*{{{ switch_tab */


/*EXTL_DOC
 * Display the region corresponding to the tab that the user pressed on.
 * This function should only be used by binding it to a mouse action.
 */
EXTL_EXPORT_MEMBER
void frame_p_switch_tab(WFrame *frame)
{
    WRegion *sub;
    
    if(ioncore_pointer_grab_region()!=(WRegion*)frame)
        return;
    
    sub=sub_at_tab(frame);
    
    if(sub!=NULL){
        bool mcf=region_may_control_focus((WRegion*)frame);
        region_goto_flags(sub, (mcf 
                                ? REGION_GOTO_FOCUS|REGION_GOTO_NOWARP 
                                : 0));
    }
}


/*}}}*/

