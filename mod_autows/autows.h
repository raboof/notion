/*
 * ion/autows/autows.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_AUTOWS_AUTOWS_H
#define ION_AUTOWS_AUTOWS_H

#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

#include <mod_ionws/ionws.h>


INTRCLASS(WAutoWS);
DECLCLASS(WAutoWS){
    WIonWS ionws;
};


extern void autows_deinit(WAutoWS *ws);
extern bool autows_init(WAutoWS *ws, WWindow *parent, const WFitParams *fp);
extern WAutoWS *create_autows(WWindow *parent, const WFitParams *fp);
extern WRegion *autows_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

/* Dynfun implementations */

extern bool autows_managed_may_destroy(WAutoWS *ws, WRegion *reg);
extern void autows_managed_remove(WAutoWS *ws, WRegion *reg);
extern void autows_managed_add(WAutoWS *ws, WRegion *reg);

extern ExtlTab autows_get_configuration(WAutoWS *ws);
extern WRegion *autows_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

#endif /* ION_AUTOWS_AUTOWS_H */
