/*
 * ion/ioncore/kbresize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_KBRESIZE_H
#define ION_IONCORE_KBRESIZE_H

#include "frame.h"

extern void frame_kbresize_units(WFrame *frame, int *wret, int *hret);
extern void frame_kbresize_begin(WFrame *frame);

extern void kbresize_end();
extern void kbresize_cancel();
extern void kbresize_move(int horizmul, int vertmul);
extern void kbresize_resize(int left, int right, int top, int bottom);
extern void kbresize_accel(int *wu, int *hu, int mode);

#endif /* ION_IONCORE_KBRESIZE_H */
