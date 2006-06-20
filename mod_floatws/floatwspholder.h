/*
 * ion/mod_floatws/floatwspholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_FLOATWS_FLOATWSPHOLDER_H
#define ION_MOD_FLOATWS_FLOATWSPHOLDER_H

#include <ioncore/common.h>
#include <ioncore/pholder.h>
#include "floatws.h"

INTRCLASS(WFloatWSPHolder);

DECLCLASS(WFloatWSPHolder){
    WPHolder ph;
    Watch floatws_watch;
    WRectangle geom;
    uint level;
    WSizePolicy szplcy;
};

extern WFloatWSPHolder *create_floatwspholder(WFloatWS *floatws, 
                                              const WStacking *st);

extern bool floatwspholder_init(WFloatWSPHolder *ph, WFloatWS *floatws,
                                const WStacking *st);

extern void floatwspholder_deinit(WFloatWSPHolder *ph);

extern bool floatwspholder_do_goto(WFloatWSPHolder *ph);

extern WRegion *floatwspholder_do_target(WFloatWSPHolder *ph);

extern bool floatwspholder_do_attach(WFloatWSPHolder *ph, 
                                     WRegionAttachHandler *hnd,
                                     void *hnd_param,
                                     int flags);

extern WFloatWSPHolder *floatws_managed_get_pholder(WFloatWS *floatws, 
                                                    WRegion *mgd);

#endif /* ION_MOD_FLOATWS_FLOATWSPHOLDER_H */
