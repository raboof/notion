/*
 * ion/mod_autows/autoframe.h
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_AUTOWS_AUTOFRAME_H
#define ION_MOD_AUTOWS_AUTOFRAME_H

#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRCLASS(WAutoFrame);

DECLCLASS(WAutoFrame){
    WFrame frame;
    WFitParams last_fp;
};

extern WAutoFrame *create_autoframe(WWindow *parent, const WFitParams *fp);
extern WRegion *autoframe_load(WWindow *par, const WFitParams *fp,
                               ExtlTab tab);

#endif /* ION_MOD_AUTOWS_AUTOFRAME_H */
