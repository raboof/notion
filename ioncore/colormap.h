/*
 * ion/ioncore/colormap.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_COLORMAP_H
#define ION_IONCORE_COLORMAP_H

#include "common.h"
#include "clientwin.h"

extern void install_cmap(WRootWin *scr, Colormap cmap);
extern void handle_cwin_cmap(WClientWin *cwin, const XColormapEvent *ev);
extern void handle_all_cmaps(const XColormapEvent *ev);
extern void get_colormaps(WClientWin *cwin);
extern void clear_colormaps(WClientWin *cwin);
extern void install_cwin_cmap(WClientWin *cwin);

#endif /* ION_IONCORE_COLORMAP_H */
