/*
 * ion/ionws/ionws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_IONWS_H
#define ION_IONWS_IONWS_H

#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/extl.h>

INTROBJ(WIonWS);

#include "split.h"

DECLOBJ(WIonWS){
	WGenWS genws;
	WObj *split_tree;
	WRegion *managed_list;
};


extern WIonWS *create_ionws(WWindow *parent, const WRectangle *bounds, 
							bool ci);
extern WIonWS *create_ionws_simple(WWindow *parent, 
								   const WRectangle *bounds);
extern WRegion *ionws_load(WWindow *par, const WRectangle *geom, 
						   ExtlTab tab);

#endif /* ION_IONWS_IONWS_H */
