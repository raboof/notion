/*
 * ion/ioncore/genframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GENFRAME_H
#define ION_IONCORE_GENFRAME_H

#include "common.h"
#include "window.h"
#include "attach.h"
#include "mplex.h"

INTROBJ(WGenFrame);

#define WGENFRAME_TAB_DRAGGED 0x0001
#define WGENFRAME_TRANSPARENT 0x0002
#define WGENFRAME_TAB_HIDE    0x0004
#define WGENFRAME_SAVED_VERT  0x0008
#define WGENFRAME_SAVED_HORIZ 0x0010
#define WGENFRAME_SHADED	  0x0020
#define WGENFRAME_SETSHADED	  0x0040
	
DECLOBJ(WGenFrame){
	WMPlex mplex;
	
	int flags;
	int saved_w, saved_h;
	int saved_x, saved_y;
	
	int tab_spacing;
	WRegion *tab_pressed_sub;
};


/* Create/destroy */
extern WGenFrame *create_genframe(WWindow *parent, WRectangle geom);
extern bool genframe_init(WGenFrame *genframe, WWindow *parent,
						  WRectangle geom);
extern void genframe_deinit(WGenFrame *genframe);

/* Resize and reparent */
extern bool genframe_reparent(WGenFrame *genframe, WWindow *parent,
							  WRectangle geom);
extern void genframe_fit(WGenFrame *genframe, WRectangle geom);
extern void genframe_resize_hints(WGenFrame *genframe, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret);

/* Focus */
extern void genframe_activated(WGenFrame *genframe);
extern void genframe_inactivated(WGenFrame *genframe);

/* Tabs */
extern int genframe_nth_tab_w(const WGenFrame *genframe, int n);
extern int genframe_nth_tab_x(const WGenFrame *genframe, int n);
extern int genframe_tab_at_x(const WGenFrame *genframe, int x);
extern void genframe_toggle_tab(WGenFrame *genframe);

/* Misc */
extern void genframe_draw_config_updated(WGenFrame *genframe);
extern void genframe_managed_geom(const WGenFrame *genframe, 
								  WRectangle *geom);

/* Dynfuns */
DYNFUN void genframe_recalc_bar(WGenFrame *genframe);
DYNFUN void genframe_draw_bar(const WGenFrame *genframe, bool complete);
DYNFUN void genframe_bar_geom(const WGenFrame *genframe, WRectangle *geom);
DYNFUN void genframe_border_inner_geom(const WGenFrame *genframe,
									   WRectangle *geom);
extern void genframe_draw_bar_default(const WGenFrame *genframe, 
									  bool complete);


#endif /* ION_IONCORE_GENFRAME_H */
