/*
 * wmcore/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_POINTER_H
#define WMCORE_POINTER_H

#include "common.h"
#include "thing.h"

typedef void WButtonHandler(WThing *thing, XButtonEvent *ev);
typedef void WMotionHandler(WThing *thing, XMotionEvent *ev, int dx, int dy);

INTRSTRUCT(WDragHandler)

DECLSTRUCT(WDragHandler){
	WMotionHandler *begin;
	WMotionHandler *motion;
	WButtonHandler *end;
};

extern bool handle_button_press(XButtonEvent *ev);
extern bool handle_button_release(XButtonEvent *ev);
extern void handle_pointer_motion(XMotionEvent *ev);

extern bool find_window_at(Window rootwin, int x, int y, Window *childret);

#endif /* WMCORE_POINTER_H */
