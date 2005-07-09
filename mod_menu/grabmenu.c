/*
 * ion/mod_menu/grabmenu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/pointer.h>
#include <ioncore/grab.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <libextl/extl.h>
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
    
    if(menu->gm_mod!=AnyModifier && menu->gm_mod!=ev->state)
        return FALSE;
    
    if(ev->keycode!=XKeysymToKeycode(ioncore_g.dpy, menu->gm_ksb))
        return FALSE;
       
    menu_select_next(menu);
    
    return FALSE;
}


/*--lowlevel routine not to be called by the user--*/
EXTL_EXPORT
WMenu *mod_menu_do_grabmenu(WMPlex *mplex, ExtlFn handler, ExtlTab tab,
                            ExtlTab param)
{
    WMenuCreateParams fnp;
    uint mod=0, ksb=0;
    WMenu *menu;
    char *key=NULL;
    
    if(!extl_table_gets_s(param, "key", &key))
        return NULL;
    
    if(!ioncore_parse_keybut(key, &mod, &ksb, FALSE, TRUE)){
        free(key);
        return NULL;
    }

    free(key);

    fnp.handler=handler;
    fnp.tab=tab;
    fnp.pmenu_mode=FALSE;
    fnp.submenu_mode=FALSE;
    fnp.big_mode=extl_table_is_bool_set(param, "big");
    fnp.initial=0;
    extl_table_gets_i(param, "initial", &(fnp.initial));

    menu=(WMenu*)mplex_attach_hnd(mplex,
                                  (WRegionAttachHandler*)create_menu,
                                  (void*)&fnp, 
                                  MPLEX_ATTACH_L2|MPLEX_ATTACH_SWITCHTO);
    
    if(menu==NULL)
        return FALSE;
 
    menu->gm_ksb=ksb;
    menu->gm_mod=mod;
    
    ioncore_grab_establish((WRegion*)menu, grabmenu_handler, NULL, 0);
    
    return menu;
}

