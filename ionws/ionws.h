/*
 * ion/ionws/ionws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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


extern WIonWS *create_ionws(WWindow *parent, WRectangle bounds, bool ci);
extern WIonWS *create_ionws_simple(WWindow *parent, WRectangle bounds);
extern WRegion *ionws_load(WWindow *par, WRectangle geom, ExtlTab tab);

#endif /* ION_IONWS_IONWS_H */
