/*
 * ion/ioncore/xic.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_XIC_H
#define ION_IONCORE_XIC_H

#include "common.h"
#include "window.h"

extern XIC xwindow_create_xic(Window win);

extern bool window_create_xic(WWindow *wwin);

extern void ioncore_init_xim(void);

#endif /* ION_IONCORE_XIC_H */
