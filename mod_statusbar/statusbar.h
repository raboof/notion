/*
 * ion/ioncore/statusbar.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_STATUSBAR_H
#define ION_IONCORE_STATUSBAR_H

#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/infowin.h>

INTRCLASS(WStatusBar);

DECLCLASS(WStatusBar){
    WInfoWin iwin;
};

extern bool statusbar_init(WStatusBar *p, WWindow *parent, 
                           const WFitParams *fp);
extern WStatusBar *create_statusbar(WWindow *parent, const WFitParams *fp);
extern void statusbar_deinit(WStatusBar *p);

extern WRegion *statusbar_load(WWindow *par, const WFitParams *fp, 
                               ExtlTab tab);

#endif /* ION_IONCORE_STATUSBAR_H */
