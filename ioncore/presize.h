/*
 * ion/ioncore/presize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_PRESIZE_H
#define ION_IONCORE_PRESIZE_H

#include "common.h"
#include "window.h"

extern void window_p_resize_prepare(WWindow *wwin, XButtonEvent *ev);
extern void window_p_resize(WWindow *wwin);
extern void window_p_move(WWindow *wwin);

#endif /* ION_IONCORE_PRESIZE_H */
