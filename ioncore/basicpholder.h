/*
 * ion/ioncore/basicpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_BASICPHOLDER_H
#define ION_IONCORE_BASICPHOLDER_H

#include "common.h"
#include "pholder.h"
#include "attach.h"


typedef WRegion *WBasicPHolderHandler(WRegion *reg, int flags,
                                      WRegionAttachData *data);

INTRCLASS(WBasicPHolder);

DECLCLASS(WBasicPHolder){
    WPHolder ph;
    Watch reg_watch;
    WBasicPHolderHandler* hnd;
};

extern WBasicPHolder *create_basicpholder(WRegion *reg,
                                          WBasicPHolderHandler *hnd);

extern bool basicpholder_init(WBasicPHolder *ph, WRegion *reg,
                              WBasicPHolderHandler *hnd);

extern void basicpholder_deinit(WBasicPHolder *ph);

extern bool basicpholder_do_goto(WBasicPHolder *ph);

extern WRegion *basicpholder_do_target(WBasicPHolder *ph);

extern WRegion *basicpholder_do_attach(WBasicPHolder *ph, int flags,
                                       WRegionAttachData *data);

#endif /* ION_IONCORE_BASICPHOLDER_H */
