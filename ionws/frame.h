/*
 * ion/frame.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_FRAME_H
#define ION_FRAME_H

#include <wmcore/common.h>
#include <wmcore/window.h>
#include <wmcore/screen.h>
#include <wmcore/attach.h>

INTROBJ(WFrame)

#define FRAME_X(FRAME) (REGION_GEOM(FRAME).x)
#define FRAME_Y(FRAME) (REGION_GEOM(FRAME).y)
#define FRAME_W(FRAME) (REGION_GEOM(FRAME).w)
#define FRAME_H(FRAME) (REGION_GEOM(FRAME).h)
#define FRAME_WIN(FRAME) ((FRAME)->win.win)
#define FRAME_DRAW(FRAME) ((FRAME)->win.draw)

#define FRAME_CLIENT_WOFF(SCR) ((SCR)->grdata.client_off.w)
#define FRAME_CLIENT_HOFF(SCR) ((SCR)->grdata.client_off.h)

#define FRAME_NO_SAVED_WH -1

#define FRAME_TAB_DRAGGED 0x0001

DECLOBJ(WFrame){
	WWindow win;
	int flags;
	int target_id;
	int tab_w;
	int saved_w, saved_h;
	int saved_x, saved_y;
	
	int managed_count;
	WRegion *managed_list;
	WRegion *current_sub;
	WRegion *current_input;
	WRegion *tab_pressed_sub;
};


extern WFrame *create_frame(WRegion *parent, WRectangle geom,
							int id, int flags);

extern void frame_bar_geom(const WFrame *frame, WRectangle *geom);
extern void frame_sub_geom(const WFrame *frame, WRectangle *geom);

extern void frame_recalc_bar(WFrame *frame);
extern void draw_frame(const WFrame *frame, bool complete);
extern void draw_frame_bar(const WFrame *frame, bool complete);

extern WRegion *frame_current_input(WFrame *frame);
extern WRegion *frame_attach_input_new(WFrame *frame, WRegionCreateFn *fn,
									   void *fnp);

extern void frame_switch_nth(WFrame *frame, uint n);
extern void frame_switch_next(WFrame *frame);
extern void frame_switch_prev(WFrame *frame);

extern void frame_attach_sub(WFrame *frame, WRegion *sub, int flags);
/*extern void frame_detach_sub(WFrame *frame, WRegion *sub);*/
extern void frame_move_managed(WFrame *dest, WFrame *src);
extern void frame_fit_managed(WFrame *frame);

extern void activate_frame(WFrame *frame);
extern void deactivate_frame(WFrame *frame);

extern WFrame *find_frame_of(Window win);
extern void frame_attach_tagged(WFrame *frame);

extern void frame_move_current_tab_right(WFrame *frame);
extern void frame_move_current_tab_left(WFrame *frame);

extern WRegion *frame_nth_managed(WFrame *frame, uint n);

extern int frame_nth_tab_w(const WFrame *frame, int n);
extern int frame_nth_tab_x(const WFrame *frame, int n);
extern int frame_tab_at_x(const WFrame *frame, int x);

#endif /* ION_FRAME_H */
