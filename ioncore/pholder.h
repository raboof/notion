/*
 * ion/ioncore/pholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_PHOLDER_H
#define ION_IONCORE_PHOLDER_H

#include "common.h"
#include "attach.h"

/* Note: PHolders should be destroyed by their acquirer. */

DECLCLASS(WPHolder){
    Obj obj;
};

extern bool pholder_init(WPHolder *ph);
extern void pholder_deinit(WPHolder *ph);

DYNFUN bool pholder_stale(WPHolder *ph);

DYNFUN bool pholder_attach(WPHolder *ph, 
                           WRegionAttachHandler *hnd, void *hnd_param);


DYNFUN WPHolder *region_managed_get_pholder(WRegion *reg, WRegion *mgd);

#endif /* ION_IONCORE_PHOLDER_H */
