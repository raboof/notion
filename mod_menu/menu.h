/*
 * ion/mod_menu/menu.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_MENU_MENU_H
#define ION_MOD_MENU_MENU_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>
#include <ioncore/binding.h>

INTRCLASS(WMenu);
INTRSTRUCT(WMenuEntry);

#define WMENUENTRY_SUBMENU 0x0001

DECLSTRUCT(WMenuEntry){
    char *title;
    int flags;
    GrStyleSpec attr;
};

DECLCLASS(WMenu){
    WWindow win;
    GrBrush *brush;
    GrBrush *entry_brush;

    WFitParams last_fp;
    
    bool pmenu_mode;
    bool big_mode;
    int n_entries, selected_entry;
    int first_entry, vis_entries;
    int max_entry_w, entry_h, entry_spacing;
    WMenuEntry *entries;
    
    WMenu *submenu;
    
    ExtlTab tab;
    ExtlFn handler;
    
    char *typeahead;
    
    uint gm_kcb, gm_state;
    
    /*WBindmap *cycle_bindmap;*/
};


INTRSTRUCT(WMenuCreateParams);

DECLSTRUCT(WMenuCreateParams){
    ExtlFn handler;
    ExtlTab tab;
    bool pmenu_mode;
    bool submenu_mode;
    bool big_mode;
    int initial;
    WRectangle refg;
};


extern WMenu *create_menu(WWindow *par, const WFitParams *fp,
                          const WMenuCreateParams *params);
extern bool menu_init(WMenu *menu, WWindow *par, const WFitParams *fp,
                      const WMenuCreateParams *params);
extern void menu_deinit(WMenu *menu);

extern bool menu_fitrep(WMenu *menu, WWindow *par, const WFitParams *fp);

extern void menu_finish(WMenu *menu);
extern void menu_cancel(WMenu *menu);
extern bool menu_rqclose(WMenu *menu);
extern void menu_updategr(WMenu *menu);

extern int menu_entry_at_root(WMenu *menu, int root_x, int root_y);
extern void menu_release(WMenu *menu, XButtonEvent *ev);
extern void menu_motion(WMenu *menu, XMotionEvent *ev, int dx, int dy);
extern void menu_button(WMenu *menu, XButtonEvent *ev);
extern int menu_press(WMenu *menu, XButtonEvent *ev, WRegion **reg_ret);

extern void mod_menu_set_scroll_params(int delay, int amount);

extern void menu_typeahead_clear(WMenu *menu);

extern void menu_select_nth(WMenu *menu, int n);
extern void menu_select_prev(WMenu *menu);
extern void menu_select_next(WMenu *menu);

#endif /* ION_MOD_MENU_MENU_H */
