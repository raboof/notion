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
#include "extl.h"

INTRSTRUCT(WRectangle);

DECLSTRUCT(WRectangle){
	int x, y;
	int w, h;
};

extern bool rectangle_contains(const WRectangle *g, int x, int y);

extern void rectange_debugprint(const WRectangle *g, const char *n);
extern void rectangle_writecode(const WRectangle *geom, FILE *file);

#endif /* ION_IONCORE_RECTANGLE_H */

