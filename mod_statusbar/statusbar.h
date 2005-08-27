/*
 * ion/mod_statusbar/statusbar.h
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


#define STATUSBAR_NX_STR "?"


typedef enum{
    WSBELEM_ALIGN_LEFT=0,
    WSBELEM_ALIGN_CENTER=1,
    WSBELEM_ALIGN_RIGHT=2
} WSBElemAlign;


typedef enum{
    WSBELEM_NONE=0,
    WSBELEM_TEXT=1,
    WSBELEM_METER=2,
    WSBELEM_STRETCH=3,
    WSBELEM_FILLER=4
} WSBElemType;
  

INTRSTRUCT(WSBElem);

DECLSTRUCT(WSBElem){
    int type;
    int align;
    int stretch;
    int text_w;
    char *text;
    char *meter;
    int max_w;
    char *tmpl;
    char *attr;
    int zeropad;
};

INTRCLASS(WStatusBar);

DECLCLASS(WStatusBar){
    WWindow wwin;
    GrBrush *brush;
    WSBElem *elems;
    int nelems;
    int natural_w, natural_h;
    int filleridx;
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
