/*
 * ion/ioncore/colormap.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_COLORMAP_H
#define ION_IONCORE_COLORMAP_H

#include "common.h"
#include "clientwin.h"

extern void ioncore_handle_colormap_notify(const XColormapEvent *ev);

extern void rootwin_install_colormap(WRootWin *scr, Colormap cmap);

extern void clientwin_install_colormap(WClientWin *cwin);
extern void clientwin_get_colormaps(WClientWin *cwin);
extern void clientwin_clear_colormaps(WClientWin *cwin);

extern void xwindow_unmanaged_selectinput(Window win, long mask);

#endif /* ION_IONCORE_COLORMAP_H */
