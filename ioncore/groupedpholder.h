/*
 * ion/ioncore/groupedpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GROUPEDPHOLDER_H
#define ION_IONCORE_GROUPEDPHOLDER_H

#include "common.h"
#include "pholder.h"
#include "attach.h"

INTRCLASS(WGroupedPHolder);

DECLCLASS(WGroupedPHolder){
    WPHolder ph;
    WPHolder *cont;
};

extern WGroupedPHolder *create_groupedpholder(WPHolder *cont);

extern bool groupedpholder_init(WGroupedPHolder *ph, WPHolder *cont);

extern void groupedpholder_deinit(WGroupedPHolder *ph);

extern bool groupedpholder_do_goto(WGroupedPHolder *ph);

extern WRegion *groupedpholder_do_target(WGroupedPHolder *ph);

extern bool groupedpholder_stale(WGroupedPHolder *ph);

extern WRegion *groupedpholder_do_attach(WGroupedPHolder *ph, int flags,
                                         WRegionAttachData *data);

#endif /* ION_IONCORE_GROUPEDPHOLDER_H */
