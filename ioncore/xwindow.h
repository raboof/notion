/*
 * ion/ioncore/xwindow.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_XWINDOW_H
#define ION_IONCORE_XWINDOW_H

#include "common.h"
#include "rectangle.h"

#define XWINDOW_REGION_OF_T(WIN, TYPE) (TYPE*)xwindow_region_of_t(WIN, &CLASSDESCR(TYPE))
#define XWINDOW_REGION_OF(WIN) xwindow_region_of(WIN)

extern Window create_xwindow(WRootWin *rw, Window par,
                             const WRectangle *geom, const char *name);

extern WRegion *xwindow_region_of(Window win);
extern WRegion *xwindow_region_of_t(Window win, const ClassDescr *descr);

extern void xwindow_restack(Window win, Window other, int stack_mode);

extern void xwindow_do_set_focus(Window win);

extern void xwindow_set_cursor(Window win, int cursor);

extern void xwindow_get_sizehints(Window win, XSizeHints *hints);

extern bool xwindow_pointer_pos(Window rel, int *px, int *py);

#endif /* ION_IONCORE_XWINDOW_H */
