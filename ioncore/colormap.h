/*
 * ion/ioncore/colormap.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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

#endif /* ION_IONCORE_COLORMAP_H */
