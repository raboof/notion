/*
 * ion/ionws/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_RESIZE_H
#define ION_IONWS_RESIZE_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/resize.h>
#include <ioncore/frame.h>
#include "ionframe.h"

extern void ionframe_begin_resize(WIonFrame *frame);
extern void ionframe_end_resize(WIonFrame *frame);
extern void ionframe_cancel_resize(WIonFrame *frame);
extern void ionframe_do_resize(WIonFrame *frame, int left, int right,
							   int top, int bottom);

#endif /* ION_IONWS_RESIZE_H */
