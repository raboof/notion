/*
 * ion/panews/unusedwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_PANEWS_UNUSEDWIN_H
#define ION_PANEWS_UNUSEDWIN_H

#include <ioncore/common.h>
#include <ioncore/gr.h>

INTRCLASS(WUnusedWin);

DECLCLASS(WUnusedWin){
    WWindow wwin;
    GrBrush *brush;
};

extern bool unusedwin_init(WUnusedWin *pwin, 
                           WWindow *parent, const WFitParams *fp);
extern WUnusedWin *create_unusedwin(WWindow *parent, const WFitParams *fp);

#endif /* ION_PANEWS_UNUSEDWIN_H */
