/*
 * ion/ionws/ionframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_IONFRAME_H
#define ION_IONWS_IONFRAME_H

#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRCLASS(WIonFrame);

DECLCLASS(WIonFrame){
    WFrame frame;
};

extern WIonFrame* create_ionframe(WWindow *parent, const WFitParams *fp);
extern WRegion *ionframe_load(WWindow *par, const WFitParams *fp,
                              ExtlTab tab);

#endif /* ION_IONWS_IONFRAME_H */
