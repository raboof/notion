/*
 * ion/floatws/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
