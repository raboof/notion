/*
 * ion/mod_mgmtmode/mgmtmode.c
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/region.h>
#include <ioncore/rootwin.h>
#include <ioncore/binding.h>
#include <ioncore/grab.h>
#include "mgmtmode.h"
#include "main.h"


static WMgmtMode *mgmt_mode=NULL;


static void cancel_mgmt(WRegion *reg);


/*{{{ WMgmtMode */


static bool mgmtmode_init(WMgmtMode *mode, WRegion *reg)
{
    watch_init(&(mode->selw));
    watch_setup(&(mode->selw), (Obj*)reg, NULL);
    return TRUE;
}


static WMgmtMode *create_mgmtmode(WRegion *reg)
{
    CREATEOBJ_IMPL(WMgmtMode, mgmtmode, (p, reg));
}


static void mgmtmode_deinit(WMgmtMode *mode)
{
    if(mgmt_mode==mode)
       mgmt_mode=NULL;
    
    watch_reset(&(mode->selw));
}


/*EXTL_DOC
 * Select management mode target.
 */
EXTL_EXPORT_MEMBER
void mgmtmode_select(WMgmtMode *mode, WRegion *reg)
{
    watch_setup(&(mode->selw), (Obj*)reg, NULL);
}


/*EXTL_DOC
 * Return management mode target.
 */
EXTL_EXPORT_MEMBER
WRegion *mgmtmode_selected(WMgmtMode *mode)
{
    return (WRegion*)(mode->selw.obj);
}


/*EXTL_DOC
 * End management mode.
 */
EXTL_EXPORT_MEMBER
void mgmtmode_finish(WMgmtMode *mode)
{
    if(mgmt_mode==mode)
        cancel_mgmt(NULL);
}
    

IMPLCLASS(WMgmtMode, Obj, mgmtmode_deinit, NULL);


/*}}}*/


/*{{{ Rubberband */


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


static void mgmtmode_draw(WMgmtMode *mode)
{
    WRegion *reg=mgmtmode_selected(mode);
    
    if(reg!=NULL){
        WRootWin *rw=region_rootwin_of(reg);
        WRectangle g=REGION_GEOM(reg);
        int rx=0, ry=0;
        
        region_rootpos(reg, &rx, &ry);
        
        g.x=rx;
        g.y=ry;
        
        draw_rubberbox(rw, &g);
    }
}


static void mgmtmode_erase(WMgmtMode *mode)
{
    mgmtmode_draw(mode);
}


/*}}}*/


/*{{{ The mode */


static bool mgmt_handler(WRegion *reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    WBinding *binding=NULL;
    WMgmtMode *mode;
    
    if(ev->type==KeyRelease)
        return FALSE;
    
    if(reg==NULL)
        return FALSE;
    
    mode=mgmt_mode;
    
    if(mode==NULL)
        return FALSE;
    
    binding=bindmap_lookup_binding(mod_mgmtmode_bindmap, 
                                   BINDING_KEYPRESS,
                                   ev->state, ev->keycode);
    
    if(!binding)
        return FALSE;
    
    if(binding!=NULL){
        mgmtmode_erase(mode);
        extl_call(binding->func, "o", NULL, mode);
        if(mgmt_mode!=NULL)
            mgmtmode_draw(mgmt_mode);
    }
    
    return (mgmt_mode==NULL);
}


static void cancel_mgmt(WRegion *reg)
{
    if(mgmt_mode!=NULL){
        mgmtmode_erase(mgmt_mode);
        destroy_obj((Obj*)mgmt_mode);
    }
    ioncore_grab_remove(mgmt_handler);
}


/*EXTL_DOC
 * Begin management mode.
 */
EXTL_EXPORT
WMgmtMode *mod_mgmtmode_begin(WRegion *reg)
{
    if(mgmt_mode!=NULL)
        return NULL;
    
    mgmt_mode=create_mgmtmode(reg);

    if(mgmt_mode==NULL)
        return NULL;
    
    ioncore_grab_establish((WRegion*)region_rootwin_of(reg), 
                           mgmt_handler,
                           (GrabKilledHandler*)cancel_mgmt, 0);
    
    mgmtmode_draw(mgmt_mode);
    
    return mgmt_mode;
}


/*}}}*/
