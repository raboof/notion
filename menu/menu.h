/*
 * ion/menu/menu.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MENU_MENU_H
#define ION_MENU_MENU_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>

INTRCLASS(WMenu);
INTRSTRUCT(WMenuEntry);

#define WMENUENTRY_SUBMENU 0x0001

DECLSTRUCT(WMenuEntry){
	char *title;
	int flags;
};

DECLCLASS(WMenu){
	WWindow win;
	GrBrush *brush;
	GrBrush *entry_brush;

	WRectangle max_geom;
	
	bool pmenu_mode;
	bool big_mode;
	int n_entries, selected_entry;
	int first_entry, vis_entries;
	int max_entry_w, entry_h, entry_spacing;
	WMenuEntry *entries;
	
	WMenu *submenu;
	
	ExtlTab tab;
	ExtlFn handler;
};


INTRSTRUCT(WMenuCreateParams);

DECLSTRUCT(WMenuCreateParams){
	ExtlFn handler;
	ExtlTab tab;
	bool pmenu_mode;
	bool submenu_mode;
	bool big_mode;
	int ref_x, ref_y;
};


extern WMenu *create_menu(WWindow *par, const WRectangle *geom,
						  const WMenuCreateParams *params);
extern bool menu_init(WMenu *menu, WWindow *par, const WRectangle *geom,
					  const WMenuCreateParams *params);
extern void menu_deinit(WMenu *menu);

extern void menu_fit(WMenu *menu, const WRectangle *geom);
extern void menu_cancel(WMenu *menu);
extern void menu_draw_config_updated(WMenu *menu);

extern int menu_entry_at_root(WMenu *menu, int root_x, int root_y);
extern void menu_release(WMenu *menu, XButtonEvent *ev);
extern void menu_motion(WMenu *menu, XMotionEvent *ev, int dx, int dy);
extern void menu_button(WMenu *menu, XButtonEvent *ev);
extern int menu_press(WMenu *menu, XButtonEvent *ev, WRegion **reg_ret);

extern void menumod_set_scroll_params(int delay, int amount);

#endif /* ION_MENU_MENU_H */
