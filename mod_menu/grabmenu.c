/*
 * ion/mod_menu/grabmenu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/pointer.h>
#include <ioncore/grab.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/key.h>
#include "menu.h"
#include "mkmenu.h"


static bool grabmenu_handler(WRegion *reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    WMenu *menu=(WMenu*)reg;
    
    if(ev->type==KeyRelease){
        if(ioncore_unmod(ev->state, ev->keycode)==0){
            menu_finish(menu);
            return TRUE;
        }
        return FALSE;
    }
    
    if(reg==NULL)
        return FALSE;
    
    if(menu->gm_state==ev->state && ev->keycode==menu->gm_kcb)
        menu_select_next(menu);
    
    return FALSE;
}


/*--lowlevel routine not to be called by the user--*/
EXTL_EXPORT
WMenu *mod_menu_do_grabmenu(WMPlex *mplex, ExtlFn handler, ExtlTab tab,
                            ExtlTab param)
{
    WMenuCreateParams fnp;
    WMPlexAttachParams par;
    WMenu *menu;
    XKeyEvent *ev;
    
    ev=ioncore_current_key_event();
    
    if(ev==NULL)
        return NULL;
    
    fnp.handler=handler;
    fnp.tab=tab;
    fnp.pmenu_mode=FALSE;
    fnp.submenu_mode=FALSE;
    fnp.big_mode=extl_table_is_bool_set(param, "big");
    fnp.initial=0;
    extl_table_gets_i(param, "initial", &(fnp.initial));

    par.flags=(MPLEX_ATTACH_SWITCHTO|
               MPLEX_ATTACH_UNNUMBERED|
               MPLEX_ATTACH_SIZEPOLICY);
    par.szplcy=SIZEPOLICY_FULL_BOUNDS;

    menu=(WMenu*)mplex_do_attach_new(mplex, &par,
                                     (WRegionCreateFn*)create_menu,
                                     (void*)&fnp); 
    
    if(menu==NULL)
        return FALSE;
 
    menu->gm_kcb=ev->keycode;
    menu->gm_state=ev->state;
    
    ioncore_grab_establish((WRegion*)menu, grabmenu_handler, NULL, 0);
    
    return menu;
}

