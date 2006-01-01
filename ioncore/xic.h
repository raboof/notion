/*
 * ion/ioncore/xic.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_XIC_H
#define ION_IONCORE_XIC_H

#include "common.h"
#include "window.h"

extern XIC xwindow_create_xic(Window win);

extern bool window_create_xic(WWindow *wwin);

extern void ioncore_init_xim(void);

#endif /* ION_IONCORE_XIC_H */
