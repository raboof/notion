/*
 * ion/ioncore/genframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GENFRAME_H
#define ION_IONCORE_GENFRAME_H

#include "common.h"
#include "window.h"
#include "rootwin.h"
#include "attach.h"

INTROBJ(WGenFrame);

#define WGENFRAME_NO_SAVED_WH -1
	
#define WGENFRAME_TAB_DRAGGED 0x0001
#define WGENFRAME_TRANSPARENT 0x0002
#define WGENFRAME_TAB_HIDE    0x0004


DECLOBJ(WGenFrame){
	WWindow win;
	int flags;
	int saved_w, saved_h;
	int saved_x, saved_y;
	
	int tab_spacing;
	
	int managed_count;
	WRegion *managed_list;
	WRegion *current_sub;
	WRegion *current_input;
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

/* Attach */
extern void genframe_move_managed(WGenFrame *dest, WGenFrame *src);
extern void genframe_attach_tagged(WGenFrame *genframe);
extern WRegion *genframe_add_input(WGenFrame *genframe, WRegionAddFn *fn,
								   void *fnp);
extern void genframe_remove_managed(WGenFrame *genframe, WRegion *reg);

/* Switch */
extern bool genframe_display_managed(WGenFrame *genframe, WRegion *sub);
extern void genframe_switch_nth(WGenFrame *genframe, uint n);
extern void genframe_switch_next(WGenFrame *genframe);
extern void genframe_switch_prev(WGenFrame *genframe);

/* Focus */
extern void genframe_focus(WGenFrame *genframe, bool warp);
extern void genframe_activated(WGenFrame *genframe);
extern void genframe_inactivated(WGenFrame *genframe);

/* Tabs */
extern int genframe_nth_tab_w(const WGenFrame *genframe, int n);
extern int genframe_nth_tab_x(const WGenFrame *genframe, int n);
extern int genframe_tab_at_x(const WGenFrame *genframe, int x);
extern void genframe_move_current_tab_right(WGenFrame *genframe);
extern void genframe_move_current_tab_left(WGenFrame *genframe);
extern void genframe_toggle_tab(WGenFrame *genframe);

/* Misc */
extern WRegion *genframe_nth_managed(WGenFrame *genframe, uint n);
extern WRegion *genframe_current_input(WGenFrame *genframe);
extern void genframe_fit_managed(WGenFrame *genframe);
extern void genframe_draw_config_updated(WGenFrame *genframe);
extern void genframe_toggle_sub_tag(WGenFrame *genframe);

/* Dynfuns */
DYNFUN void genframe_recalc_bar(WGenFrame *genframe, bool draw);
DYNFUN void genframe_draw_bar(const WGenFrame *genframe, bool complete);
DYNFUN void genframe_managed_geom(const WGenFrame *genframe, WRectangle *geom);
DYNFUN void genframe_bar_geom(const WGenFrame *genframe, WRectangle *geom);
DYNFUN void genframe_border_inner_geom(const WGenFrame *genframe,
									   WRectangle *geom);
DYNFUN void genframe_size_changed(WGenFrame *genframe, bool wchg, bool hchg);

extern void genframe_draw_bar_default(const WGenFrame *genframe, bool complete);


#endif /* ION_IONCORE_GENFRAME_H */
