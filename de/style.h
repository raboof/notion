/*
 * ion/de/style.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_DE_STYLE_H
#define ION_DE_STYLE_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRSTRUCT(DEBorder);
INTRSTRUCT(DEStyle);

#include "font.h"
#include "colour.h"

enum{
    DEBORDER_INLAID=0,    /* -\xxxxxx/- */
    DEBORDER_RIDGE,       /* /-\xxxx/-\ */
    DEBORDER_ELEVATED,    /* /-xxxxxx-\ */
    DEBORDER_GROOVE       /* \_/xxxx\_/ */
};


enum{
    DEALIGN_LEFT=0,
    DEALIGN_RIGHT=1,
    DEALIGN_CENTER=2
};


DECLSTRUCT(DEBorder){
    uint sh, hl, pad;
    uint style;
};


DECLSTRUCT(DEStyle){
    char *style;
    int usecount;
    bool is_fallback;
    
    WRootWin *rootwin;
    
    DEStyle *based_on;
    
    GC normal_gc;    
    
    DEBorder border;
    bool cgrp_alloced;
    DEColourGroup cgrp;
    int n_extra_cgrps;
    DEColourGroup *extra_cgrps;
    GrTransparency transparency_mode;
    DEFont *font;
    int textalign;
    uint spacing;
    
    ExtlTab data_table;

    /* Only initialised if used as a DETabBrush */
    bool tabbrush_data_ok;
    GC stipple_gc;
    GC copy_gc;
    
    Pixmap tag_pixmap;
    int tag_pixmap_w;
    int tag_pixmap_h;
    
    DEStyle *next, *prev;
};


extern bool destyle_init(DEStyle *style, WRootWin *rootwin, const char *name);
extern void destyle_deinit(DEStyle *style);
extern DEStyle *de_create_style(WRootWin *rootwin, const char *name);
extern void destyle_unref(DEStyle *style);

extern void destyle_create_tab_gcs(DEStyle *style);

extern void de_reset();
extern void de_deinit_styles();

extern DEStyle *de_get_style(WRootWin *rootwin, const char *name);


#endif /* ION_DE_STYLE_H */
