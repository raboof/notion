/*
 * ion/ioncore/basicpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_BASICPHOLDER_H
#define ION_IONCORE_BASICPHOLDER_H

#include "common.h"
#include "pholder.h"
#include "attach.h"

INTRCLASS(WBasicPHolder);

DECLCLASS(WBasicPHolder){
    WPHolder ph;
    Watch reg_watch;
    WRegionDoAttachFnSimple *hnd;
};

extern WBasicPHolder *create_basicpholder(WRegion *reg,
                                          WRegionDoAttachFnSimple *hnd);

extern bool basicpholder_init(WBasicPHolder *ph, WRegion *reg,
                              WRegionDoAttachFnSimple *hnd);

extern void basicpholder_deinit(WBasicPHolder *ph);

extern bool basicpholder_do_goto(WBasicPHolder *ph);

extern WRegion *basicpholder_do_target(WBasicPHolder *ph);

extern bool basicpholder_do_attach(WBasicPHolder *ph, 
                                   WRegionAttachHandler *hnd,
                                   void *hnd_param);

#endif /* ION_IONCORE_BASICPHOLDER_H */
