/*
 * ion/ioncore/groupwsrescueph.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GROUPWSRESCUEPH_H
#define ION_IONCORE_GROUPWSRESCUEPH_H

#include <ioncore/common.h>
#include <ioncore/pholder.h>
#include "group-ws.h"

INTRCLASS(WGroupWSRescuePH);

DECLCLASS(WGroupWSRescuePH){
    WPHolder ph;
    bool pos_ok;
    bool inner_geom;
    WRectangle geom;
    Watch groupws_watch;
    Watch frame_watch;
    int gravity;
    Watch stack_above_watch;
};

extern WGroupWSRescuePH *create_groupwsrescueph(WGroupWS *groupws,
                                                const WRectangle *geom, 
                                                bool pos_ok, bool inner_geom, 
                                                int gravity);

extern bool groupwsrescueph_init(WGroupWSRescuePH *ph, WGroupWS *ws,
                                 const WRectangle *geom, 
                                 bool pos_ok, bool inner_geom, int gravity);

extern void groupwsrescueph_deinit(WGroupWSRescuePH *ph);

extern bool groupwsrescueph_do_goto(WGroupWSRescuePH *ph);

extern WRegion *groupwsrescueph_do_target(WGroupWSRescuePH *ph);

extern bool groupwsrescueph_do_attach(WGroupWSRescuePH *ph, 
                                      WRegionAttachHandler *hnd,
                                      void *hnd_param, int flags);

extern WGroupWSRescuePH *groupws_get_rescue_pholder_for(WGroupWS *groupws, 
                                                        WRegion *forwhat);

#endif /* ION_IONCORE_GROUPWSRESCUEPH_H */
