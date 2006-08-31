/*
 * ion/ioncore/grouprescueph.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GROUPRESCUEPH_H
#define ION_IONCORE_GROUPRESCUEPH_H

#include <ioncore/common.h>
#include <ioncore/pholder.h>
#include "group-ws.h"
#include "grouppholder.h"

INTRCLASS(WGroupRescuePH);

DECLCLASS(WGroupRescuePH){
    WGroupPHolder gph;
    Watch frame_watch;
};

extern WGroupRescuePH *create_grouprescueph(WGroup *grp,
                                            const WStacking *either_st,
                                            const WGroupAttachParams *or_param);

extern bool grouprescueph_init(WGroupRescuePH *ph, WGroup *grp,
                               const WStacking *either_st,
                               const WGroupAttachParams *or_param);

extern void grouprescueph_deinit(WGroupRescuePH *ph);

extern bool grouprescueph_do_goto(WGroupRescuePH *ph);

extern WRegion *grouprescueph_do_target(WGroupRescuePH *ph);

extern bool grouprescueph_do_attach(WGroupRescuePH *ph, int flags,
                                    WRegionAttachData *data);

extern WGroupRescuePH *groupws_get_rescue_pholder_for(WGroupWS *group, 
                                                      WRegion *forwhat);

#endif /* ION_IONCORE_GROUPRESCUEPH_H */
