/*
 * ion/frame-pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_FRAME_POINTER_H
#define ION_FRAME_POINTER_H

#include <wmcore/common.h>
#include <wmcore/pointer.h>
#include "frame.h"

enum{
	FRAME_AREA_NONE,
	FRAME_AREA_BORDER,
	FRAME_AREA_TAB,
	FRAME_AREA_EMPTY_TAB,
	FRAME_AREA_CLIENT
};

extern void p_resize_setup(WFrame *frame);
extern void p_tabdrag_setup(WFrame *frame);

extern int frame_press(WFrame *frame, XButtonEvent *ev, WThing **thing_ret);

#endif /* ION_FRAME_POINTER_H */
