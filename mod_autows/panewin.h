/*
 * ion/autows/panewin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_AUTOWS_PANEWIN_H
#define ION_AUTOWS_PANEWIN_H

#include <ioncore/common.h>
#include <ioncore/gr.h>

INTRCLASS(WPaneWin);

DECLCLASS(WPaneWin){
    WWindow wwin;
    GrBrush *brush;
    GrBorderLine bline;
    GrBorderWidths bdw;
};

extern bool panewin_init(WPaneWin *pwin, 
                         WWindow *parent, const WFitParams *fp);
extern WPaneWin *create_panewin(WWindow *parent, const WFitParams *fp);

#endif /* ION_AUTOWS_PANEWIN_H */
