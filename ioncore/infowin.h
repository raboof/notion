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
};

#define INFOWIN_BRUSH(INFOWIN) ((INFOWIN)->brush)
#define INFOWIN_BUFFER(INFOWIN) ((INFOWIN)->buffer)

extern bool infowin_init(WInfoWin *p, WWindow *parent, 
						 const WRectangle *geom,
						 const char *style);
extern WInfoWin *create_infowin(WWindow *parent, const WRectangle *geom,
								const char *style);
extern bool infowin_set_attr2(WInfoWin *p, const char *attr1,
							  const char *attr2);

#endif /* ION_IONCORE_INFOWIN_H */
