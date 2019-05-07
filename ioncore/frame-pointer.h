/*
 * ion/ioncore/frame-pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FRAME_POINTER_H
#define ION_IONCORE_FRAME_POINTER_H

#include "common.h"
#include "pointer.h"
#include "frame.h"

extern void frame_p_resize(WFrame *frame);
extern void frame_p_tabdrag(WFrame *frame);
extern void frame_p_move(WFrame *frame);
extern void frame_p_switch_tab(WFrame *frame);

extern int frame_press(WFrame *frame, XButtonEvent *ev,
                          WRegion **reg_ret);
extern void frame_release(WFrame *frame);

#endif /* ION_IONCORE_FRAME_POINTER_H */
