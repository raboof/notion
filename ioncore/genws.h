/*
 * ion/ioncore/genws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GENWS_H
#define ION_IONCORE_GENWS_H

#include "region.h"
#include "window.h"
#include "extl.h"

INTROBJ(WGenWS);

DECLOBJ(WGenWS){
	WRegion reg;
};

extern void genws_init(WGenWS *ws, WWindow *parent, WRectangle geom);
extern void genws_deinit(WGenWS *ws);

#endif /* ION_IONCORE_GENWS_H */
