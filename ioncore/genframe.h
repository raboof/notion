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
#include "gr.h"
#include "extl.h"

INTROBJ(WGenFrame);

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
	
	int tab_dragged_idx;
	
	GrBrush *brush;
	GrBrush *bar_brush;
	GrTransparency tr_mode;
	int bar_h;
	GrTextElem *titles;
	int titles_n;
};


/* Create/destroy */
extern WGenFrame *create_genframe(WWindow *parent, const WRectangle *geom);
extern bool genframe_init(WGenFrame *genframe, WWindow *parent,
						  const WRectangle *geom);
extern void genframe_deinit(WGenFrame *genframe);

/* Resize and reparent */
extern bool genframe_reparent(WGenFrame *genframe, WWindow *parent,
							  const WRectangle *geom);
extern void genframe_fit(WGenFrame *genframe, const WRectangle *geom);
extern void genframe_resize_hints(WGenFrame *genframe, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret);

/* Focus */
extern void genframe_activated(WGenFrame *genframe);
extern void genframe_inactivated(WGenFrame *genframe);

/* Tabs */
extern int genframe_nth_tab_w(const WGenFrame *genframe, int n);
extern int genframe_nth_tab_iw(const WGenFrame *genframe, int n);
extern int genframe_nth_tab_x(const WGenFrame *genframe, int n);
extern int genframe_tab_at_x(const WGenFrame *genframe, int x);
extern void genframe_toggle_tab(WGenFrame *genframe);
extern void genframe_update_attr_nth(WGenFrame *genframe, int i);

/* Misc */
extern void genframe_draw_config_updated(WGenFrame *genframe);
extern void genframe_toggle_sub_tag(WGenFrame *genframe);
extern void genframe_border_geom(const WGenFrame *genframe, 
								 WRectangle *geom);
extern void genframe_border_inner_geom(const WGenFrame *genframe, 
									   WRectangle *geom);
extern void genframe_load_saved_geom(WGenFrame* genframe, ExtlTab tab);

/* Dynfuns */
DYNFUN const char *genframe_style(WGenFrame *genframe);
DYNFUN const char *genframe_tab_style(WGenFrame *genframe);
DYNFUN void genframe_recalc_bar(WGenFrame *genframe);
DYNFUN void genframe_draw_bar(const WGenFrame *genframe, bool complete);
DYNFUN void genframe_bar_geom(const WGenFrame *genframe, WRectangle *geom);
DYNFUN void genframe_border_inner_geom(const WGenFrame *genframe,
									   WRectangle *geom);
DYNFUN void genframe_brushes_updated(WGenFrame *genframe);


extern void genframe_draw_bar_default(const WGenFrame *genframe, 
									  bool complete);
extern void genframe_draw_default(const WGenFrame *genframe, bool complete);


#endif /* ION_IONCORE_GENFRAME_H */
