/*
 * ion/mod_floatws/floatwsrescueph.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_FLOATWS_FLOATWSRESCUEPH_H
#define ION_MOD_FLOATWS_FLOATWSRESCUEPH_H

#include <ioncore/common.h>
#include <ioncore/pholder.h>
#include "floatws.h"

INTRCLASS(WFloatWSRescuePH);

DECLCLASS(WFloatWSRescuePH){
    WPHolder ph;
    bool pos_ok;
    bool inner_geom;
    WRectangle geom;
    Watch floatws_watch;
    Watch frame_watch;
};

extern WFloatWSRescuePH *create_floatwsrescueph(WFloatWS *floatws, 
                                                WRegion *contents,
                                                WRegion *or_this);

extern bool floatwsrescueph_init(WFloatWSRescuePH *ph, WFloatWS *floatws,
                                 WRegion *contents, WRegion *or_this);

extern void floatwsrescueph_deinit(WFloatWSRescuePH *ph);

extern bool floatwsrescueph_stale(WFloatWSRescuePH *ph);

extern bool floatwsrescueph_do_goto(WFloatWSRescuePH *ph);

extern bool floatwsrescueph_do_attach(WFloatWSRescuePH *ph, 
                                      WRegionAttachHandler *hnd,
                                      void *hnd_param);

extern WFloatWSRescuePH *floatws_get_rescue_pholder_for(WFloatWS *floatws, 
                                                        WRegion *forwhat);

#endif /* ION_MOD_FLOATWS_FLOATWSRESCUEPH_H */
