/*
 * ion/panews/panehandle.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_PANEWS_PANEHANDLE_H
#define ION_PANEWS_PANEHANDLE_H

#include <ioncore/common.h>
#include <ioncore/gr.h>

INTRCLASS(WPaneHandle);

#include "splitfloat.h"

DECLCLASS(WPaneHandle){
    WWindow wwin;
    GrBrush *brush;
    GrBorderLine bline;
    GrBorderWidths bdw;
    WSplitFloat *splitfloat;
};

extern bool panehandle_init(WPaneHandle *pwin, 
                            WWindow *parent, const WFitParams *fp);
extern WPaneHandle *create_panehandle(WWindow *parent, const WFitParams *fp);

#endif /* ION_PANEWS_PANEHANDLE_H */
