/*
 * ion/mod_menu/grabmenu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/pointer.h>
#include <ioncore/grab.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/key.h>
#include <ioncore/stacking.h>
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

    if(ev->keycode==menu->gm_kcb){
        if(menu->gm_state==ev->state)
            menu_select_next(menu);
        else if((menu->gm_state|ShiftMask)==ev->state)
            menu_select_prev(menu);
        else if(menu->gm_state==AnyModifier)
            menu_select_next(menu);
    }

    return FALSE;
}


static void grabkilled_handler(WRegion *reg)
{
    destroy_obj((Obj*)reg);
}


/*--lowlevel routine not to be called by the user--*/
EXTL_EXPORT
WMenu *mod_menu_do_grabmenu(WMPlex *mplex, ExtlFn handler, ExtlTab tab,
                            ExtlTab param)
{
    WMenuCreateParams fnp;
    WMPlexAttachParams par=MPLEXATTACHPARAMS_INIT;
    WMenu *menu;
    uint state, kcb;
    bool sub;

    if(!ioncore_current_key(&kcb, &state, &sub))
        return NULL;

    if(state==0){
        WMenu *menu=mod_menu_do_menu(mplex, handler, tab, param);
        /*
        if(menu!=NULL && cycle!=extl_fn_none()){
            uint kcb, state;

            menu->cycle_bindmap=region_add_cycle_bindmap((WRegion*)menu,
                                                         kcb, state, ???,
                                                         ???);
        }*/
        return menu;
    }

    fnp.handler=handler;
    fnp.tab=tab;
    fnp.pmenu_mode=FALSE;
    fnp.submenu_mode=FALSE;
    fnp.big_mode=extl_table_is_bool_set(param, "big");
    fnp.initial=0;
    extl_table_gets_i(param, "initial", &(fnp.initial));

    par.flags=(MPLEX_ATTACH_SWITCHTO|
               MPLEX_ATTACH_LEVEL|
               MPLEX_ATTACH_UNNUMBERED|
               MPLEX_ATTACH_SIZEPOLICY);
    extl_table_gets_sizepolicy(param, &par.szplcy, SIZEPOLICY_FULL_BOUNDS);
    par.level=STACKING_LEVEL_MODAL1+1;

    menu=(WMenu*)mplex_do_attach_new(mplex, &par,
                                     (WRegionCreateFn*)create_menu,
                                     (void*)&fnp);

    if(menu==NULL)
        return NULL;

    menu->gm_kcb=kcb;
    menu->gm_state=state;

    ioncore_grab_establish((WRegion*)menu, grabmenu_handler,
                           grabkilled_handler, 0, GRAB_DEFAULT_FLAGS);

    return menu;
}

