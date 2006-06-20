/*
 * ion/ioncore/grouppholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GROUPPHOLDER_H
#define ION_IONCORE_GROUPPHOLDER_H

#include <ioncore/common.h>
#include <ioncore/pholder.h>
#include "group.h"

INTRCLASS(WGroupPHolder);

DECLCLASS(WGroupPHolder){
    WPHolder ph;
    Watch group_watch;
    WRectangle geom;
    uint level;
    WSizePolicy szplcy;
};

extern WGroupPHolder *create_grouppholder(WGroup *group, 
                                          const WStacking *st);

extern bool grouppholder_init(WGroupPHolder *ph, WGroup *group,
                              const WStacking *st);

extern void grouppholder_deinit(WGroupPHolder *ph);

extern bool grouppholder_do_goto(WGroupPHolder *ph);

extern WRegion *grouppholder_do_target(WGroupPHolder *ph);

extern bool grouppholder_do_attach(WGroupPHolder *ph, 
                                   WRegionAttachHandler *hnd,
                                   void *hnd_param,
                                   int flags);

extern WGroupPHolder *group_managed_get_pholder(WGroup *group, 
                                                WRegion *mgd);

#endif /* ION_IONCORE_GROUPPHOLDER_H */
