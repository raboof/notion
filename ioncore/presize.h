/*
 * ion/ioncore/presize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_PRESIZE_H
#define ION_IONCORE_PRESIZE_H

#include "common.h"
#include "window.h"

extern void window_p_resize_prepare(WWindow *wwin, XButtonEvent *ev);
extern void window_p_resize(WWindow *wwin);
extern void window_p_move(WWindow *wwin);

#endif /* ION_IONCORE_PRESIZE_H */
