/*
 * ion/menu/menu.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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

INTROBJ(WMenu);

DECLOBJ(WMenu){
	WWindow win;
	WRectangle max_geom;
	GrBrush *brush;
	GrBrush *entry_brush;
	
	uint n_entries, selected_entry;
	int max_entry_w, entry_h, entry_spacing;
	char **entry_titles;
	
	ExtlTab tab;
	ExtlFn handler;
};


INTRSTRUCT(WMenuCreateParams);

DECLSTRUCT(WMenuCreateParams){
	ExtlFn handler;
	ExtlTab tab;
};


extern WMenu *create_menu(WWindow *par, const WRectangle *geom,
						  const WMenuCreateParams *params);
extern bool menu_init(WMenu *menu, WWindow *par, const WRectangle *geom,
					  const WMenuCreateParams *params);
extern void menu_deinit(WMenu *menu);

extern void menu_fit(WMenu *menu, const WRectangle *geom);
extern void menu_cancel(WMenu *menu);
extern void menu_draw_config_updated(WMenu *menu);

/* Or how? */
DYNFUN const char *menu_style(WMenu *menu);
DYNFUN const char *menu_entry_style(WMenu *menu);

#endif /* ION_MENU_MENU_H */
