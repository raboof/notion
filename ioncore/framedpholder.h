/*
 * ion/ioncore/framedpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAMEDPHOLDER_H
#define ION_IONCORE_FRAMEDPHOLDER_H

#include "common.h"
#include "pholder.h"
#include "attach.h"

INTRCLASS(WFramedPHolder);
INTRSTRUCT(WFramedParam);


#define FRAMEDPARAM_INIT {0, 0, {0, 0, 0, 0}, NULL}


DECLSTRUCT(WFramedParam){
    uint inner_geom_gravity_set:1;
    int gravity;
    WRectangle inner_geom;
    WRegionSimpleCreateFn *mkframe;
};


DECLCLASS(WFramedPHolder){
    WPHolder ph;
    WPHolder *cont;
    WFramedParam param;
};


extern WFramedPHolder *create_framedpholder(WPHolder *cont,
                                            const WFramedParam *param);

extern bool framedpholder_init(WFramedPHolder *ph, WPHolder *cont,
                               const WFramedParam *param);

extern void framedpholder_deinit(WFramedPHolder *ph);

extern bool framedpholder_do_goto(WFramedPHolder *ph);

extern WRegion *framedpholder_do_target(WFramedPHolder *ph);

extern bool framedpholder_do_attach(WFramedPHolder *ph, int flags,
                                     WRegionAttachData *data);

extern WRegion *region_attach_framed(WRegion *reg, WFramedParam *param,
                                     WRegionAttachFn *fn, void *fn_param,
                                     WRegionAttachData *data);

#endif /* ION_IONCORE_FRAMEDPHOLDER_H */
