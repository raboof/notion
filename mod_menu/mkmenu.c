/*
 * ion/mod_menu/mkmenu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <ioncore/common.h>
#include <ioncore/pointer.h>
#include <ioncore/grab.h>
#include <libextl/extl.h>
#include "menu.h"
#include "mkmenu.h"


/*--lowlevel routine not to be called by the user--EXTL_DOC
 * Display a menu inside multiplexer. The \var{handler} parameter
 * is a function that gets the selected menu entry as argument and
 * should call it with proper parameters. The table \var{tab} is a
 * list of menu entries of the form \code{\{name = ???, [ submenu_fn = ??? ]\}}.
 * The function \var{submenu_fn} return a similar submenu definition 
 * when called.
 * 
 * Do not use this function directly. Use  \fnref{mod_menu.menu} and
 * \fnref{mod_menu.bigmenu}.
 */
EXTL_EXPORT
WMenu *mod_menu_do_menu(WMPlex *mplex, ExtlFn handler, ExtlTab tab, 
                        bool big_mode, int initial)
{
    WMenuCreateParams fnp;

    fnp.handler=handler;
    fnp.tab=tab;
    fnp.pmenu_mode=FALSE;
    fnp.submenu_mode=FALSE;
    fnp.big_mode=big_mode;
    fnp.initial=initial;
    
    return (WMenu*)mplex_attach_hnd(mplex,
                                    (WRegionAttachHandler*)create_menu,
                                    (void*)&fnp, 
                                    MPLEX_ATTACH_L2|MPLEX_ATTACH_SWITCHTO);
}


/*--lowlevel routine not to be called by the user--EXTL_DOC
 * Display a pop-up menu inside window \var{where}. This function
 * can only be called from a mouse/pointing device button press handler
 * and the menu will be placed below the point where the press occured.
 * The \var{handler} and \var{tab} parameters are similar to those of
 * \fnref{menu_menu}.
 * 
 * Do not use this function directly. Use \fnref{mod_menu.pmenu}.
 */
EXTL_EXPORT
WMenu *mod_menu_do_pmenu(WWindow *where, ExtlFn handler, ExtlTab tab)
{
    WScreen *scr;
    WMenuCreateParams fnp;
    XEvent *ev=ioncore_current_pointer_event();
    WMenu *menu;
    WFitParams fp;
    
    if(ev==NULL || ev->type!=ButtonPress)
        return NULL;

    scr=region_screen_of((WRegion*)where);

    if(scr==NULL)
        return NULL;
    
    fnp.handler=handler;
    fnp.tab=tab;
    fnp.pmenu_mode=TRUE;
    fnp.big_mode=FALSE;
    fnp.submenu_mode=FALSE;
    fnp.initial=0;
    fnp.ref_x=ev->xbutton.x_root-REGION_GEOM(scr).x;
    fnp.ref_y=ev->xbutton.y_root-REGION_GEOM(scr).y;
    
    fp.mode=REGION_FIT_BOUNDS;
    fp.g.x=0;
    fp.g.y=0;
    fp.g.w=REGION_GEOM(where).w;
    fp.g.h=REGION_GEOM(where).h;
    
    menu=create_menu((WWindow*)scr, &fp, &fnp);
    
    if(menu==NULL)
        return NULL;

    region_raise((WRegion*)menu);
    
    if(!ioncore_set_drag_handlers((WRegion*)menu,
                            NULL,
                            (WMotionHandler*)menu_motion,
                            (WButtonHandler*)menu_release,
                            NULL, 
                            (GrabKilledHandler*)menu_cancel)){
        destroy_obj((Obj*)menu);
        return NULL;
    }
    
    region_map((WRegion*)menu);
    
    return menu;
}

