/*
 * ion/ioncore/statusbar.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_STATUSBAR_STATUSBAR_H
#define ION_MOD_STATUSBAR_STATUSBAR_H

#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/gr.h>

INTRCLASS(WStatusBar);

DECLCLASS(WStatusBar){
    WWindow wwin;
    GrBrush *brush;
    GrTextElem *strings;
    int nstrings;
    char *natural_w_tmpl;
    int natural_w, natural_h;
};

extern bool statusbar_init(WStatusBar *p, WWindow *parent, 
                           const WFitParams *fp);
extern WStatusBar *create_statusbar(WWindow *parent, const WFitParams *fp);
extern void statusbar_deinit(WStatusBar *p);

extern WRegion *statusbar_load(WWindow *par, const WFitParams *fp, 
                               ExtlTab tab);

extern void statusbar_set_natural_w(WStatusBar *p, const char *str);
extern void statusbar_size_hints(WStatusBar *p, XSizeHints *h);
extern void statusbar_updategr(WStatusBar *p);
extern void statusbar_set_contents(WStatusBar *sb, ExtlTab t);

#endif /* ION_MOD_STATUSBAR_STATUSBAR_H */
