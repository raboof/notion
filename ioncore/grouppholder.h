/*
 * ion/ioncore/grouppholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GROUPPHOLDER_H
#define ION_IONCORE_GROUPPHOLDER_H

#include <ioncore/common.h>
#include <ioncore/pholder.h>
#include "group.h"


DECLCLASS(WGroupPHolder){
    WPHolder ph;
    WGroup *group;
    Watch stack_above_watch;
    WGroupAttachParams param;
    WGroupPHolder *next, *prev;
    WPHolder *recreate_pholder;
};

extern WGroupPHolder *create_grouppholder(WGroup *group,
                                          const WStacking *either_st,
                                          const WGroupAttachParams *or_param);

extern bool grouppholder_init(WGroupPHolder *ph,
                              WGroup *group,
                              const WStacking *either_st,
                              const WGroupAttachParams *or_param);

extern void grouppholder_deinit(WGroupPHolder *ph);

extern bool grouppholder_do_goto(WGroupPHolder *ph);

extern WRegion *grouppholder_do_target(WGroupPHolder *ph);

extern WRegion *grouppholder_do_attach(WGroupPHolder *ph, int flags,
                                       WRegionAttachData *data);

extern WGroupPHolder *group_managed_get_pholder(WGroup *group,
                                                WRegion *mgd);

extern void grouppholder_do_unlink(WGroupPHolder *ph);
extern void grouppholder_do_link(WGroupPHolder *ph, WGroup *group,
                                 WRegion *stack_above);

#endif /* ION_IONCORE_GROUPPHOLDER_H */
