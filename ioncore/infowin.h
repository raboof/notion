/*
 * ion/ioncore/infowin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_INFOWIN_H
#define ION_IONCORE_INFOWIN_H

#include "common.h"
#include "window.h"
#include "gr.h"
#include "rectangle.h"

#define INFOWIN_BUFFER_LEN 256

DECLCLASS(WInfoWin){
    WWindow wwin;
    GrBrush *brush;
    char *buffer;
    char *attr;
    char *style;
    char *natural_w_tmpl;
    int natural_w, natural_h;
    bool drawn;
};

#define INFOWIN_BRUSH(INFOWIN) ((INFOWIN)->brush)
#define INFOWIN_BUFFER(INFOWIN) ((INFOWIN)->buffer)

extern bool infowin_init(WInfoWin *p, WWindow *parent, const WFitParams *fp,
                         const char *style);
extern WInfoWin *create_infowin(WWindow *parent, const WFitParams *fp,
                                const char *style);

extern void infowin_deinit(WInfoWin *p);

extern void infowin_set_text(WInfoWin *p, const char *s);
extern bool infowin_set_attr2(WInfoWin *p, const char *a1, const char *a2);
extern void infowin_set_natural_w(WInfoWin *p, const char *str);

extern WRegion *infowin_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

extern void infowin_updategr(WInfoWin *p);
extern void infowin_size_hints(WInfoWin *p, XSizeHints *h);

#endif /* ION_IONCORE_INFOWIN_H */
