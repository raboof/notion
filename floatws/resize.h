/*
 * ion/floatws/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef FLOAT_FLOATWS_RESIZE_H
#define FLOAT_FLOATWS_RESIZE_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/resize.h>
#include <ioncore/genframe.h>
#include "floatframe.h"

extern void floatframe_begin_resize(WFloatFrame *frame);
extern void floatframe_grow_vert(WFloatFrame *frame);
extern void floatframe_shrink_vert(WFloatFrame *frame);
extern void floatframe_grow_horiz(WFloatFrame *frame);
extern void floatframe_shrink_horiz(WFloatFrame *frame);
extern void floatframe_end_resize(WFloatFrame *frame);
extern void floatframe_cancel_resize(WFloatFrame *frame);

#endif /* FLOAT_FLOATWS_RESIZE_H */
