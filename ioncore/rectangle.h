/*
 * ion/ioncore/rectangle.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_RECTANGLE_H
#define ION_IONCORE_RECTANGLE_H

#include <stdio.h>
#include "common.h"
#include <libextl/extl.h>

INTRSTRUCT(WRectangle);

DECLSTRUCT(WRectangle){
    int x, y;
    int w, h;
};

extern bool rectangle_contains(const WRectangle *g, int x, int y);
extern void rectangle_constrain(WRectangle *g, const WRectangle *bounds);

extern void rectange_debugprint(const WRectangle *g, const char *n);

#endif /* ION_IONCORE_RECTANGLE_H */

