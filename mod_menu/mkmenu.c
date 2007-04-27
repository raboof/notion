/*
 * ion/mod_menu/mkmenu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <limits.h>
#include <ioncore/common.h>
#include <ioncore/pointer.h>
#include <ioncore/stacking.h>
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
                        ExtlTab param)
{
    WMenuCreateParams fnp;
    WMPlexAttachParams par;

    fnp.handler=handler;
    fnp.tab=tab;
    fnp.pmenu_mode=FALSE;
    fnp.submenu_mode=FALSE;
    fnp.big_mode=extl_table_is_bool_set(param, "big");
    fnp.initial=0;
    extl_table_gets_i(param, "initial", &(fnp.initial));
    fnp.refg.x=0;
    fnp.refg.y=0;
    fnp.refg.w=0;
    fnp.refg.h=0;
    
    par.flags=(MPLEX_ATTACH_SWITCHTO|
               MPLEX_ATTACH_LEVEL|
               MPLEX_ATTACH_UNNUMBERED|
               MPLEX_ATTACH_SIZEPOLICY);
    par.szplcy=SIZEPOLICY_FULL_BOUNDS;
    par.level=STACKING_LEVEL_MODAL1+1;
    
    return (WMenu*)mplex_do_attach_new(mplex, &par,
                                       (WRegionCreateFn*)create_menu,
                                       (void*)&fnp); 
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
    fnp.refg.x=ev->xbutton.x_root-REGION_GEOM(scr).x;
    fnp.refg.y=ev->xbutton.y_root-REGION_GEOM(scr).y;
    fnp.refg.w=0;
    fnp.refg.h=0;
    
    fp.mode=REGION_FIT_BOUNDS;
    fp.g.x=REGION_GEOM(where).x;
    fp.g.y=REGION_GEOM(where).y;
    fp.g.w=REGION_GEOM(where).w;
    fp.g.h=REGION_GEOM(where).h;
    
    menu=create_menu((WWindow*)scr, &fp, &fnp);
    
    if(menu==NULL)
        return NULL;

    region_restack((WRegion*)menu, None, Above);
    
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

