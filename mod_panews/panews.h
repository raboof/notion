/*
 * ion/panews/panews.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_PANEWS_PANEWS_H
#define ION_PANEWS_PANEWS_H

#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <libextl/extl.h>
#include <ioncore/rectangle.h>

#include <mod_ionws/ionws.h>


INTRCLASS(WPaneWS);
DECLCLASS(WPaneWS){
    WIonWS ionws;
};


extern void panews_deinit(WPaneWS *ws);
extern bool panews_init(WPaneWS *ws, WWindow *parent, const WFitParams *fp,
                        bool cu);
extern WPaneWS *create_panews(WWindow *parent, const WFitParams *fp, bool cu);
extern WPaneWS *create_panews_simple(WWindow *parent, const WFitParams *fp);
extern WRegion *panews_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

/* Dynfun implementations */

extern bool panews_managed_may_destroy(WPaneWS *ws, WRegion *reg);
extern void panews_managed_remove(WPaneWS *ws, WRegion *reg);
extern void panews_managed_add(WPaneWS *ws, WRegion *reg);
extern WRegion *panews_managed_control_focus(WPaneWS *ws, WRegion *reg);

extern ExtlTab panews_get_configuration(WPaneWS *ws);
extern WRegion *panews_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

#endif /* ION_PANEWS_PANEWS_H */
