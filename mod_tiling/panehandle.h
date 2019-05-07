/*
 * ion/mod_tiling/panehandle.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
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
