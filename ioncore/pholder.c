/*
 * ion/ioncore/pholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include "common.h"
#include "pholder.h"


bool pholder_init(WPHolder *ph)
{
    return TRUE;
}


void pholder_deinit(WPHolder *ph)
{
}


bool pholder_attach(WPHolder *ph, 
                    WRegionAttachHandler *hnd, void *hnd_param)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_attach, ph, (ph, hnd, hnd_param));
    return ret;
}


bool pholder_stale(WPHolder *ph)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_stale, ph, (ph));
    return ret;
}


WPHolder *region_managed_get_pholder(WRegion *reg, WRegion *mgd)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, region_managed_get_pholder, 
                 reg, (reg, mgd));
    return ret;
}


IMPLCLASS(WPHolder, Obj, pholder_deinit, NULL);

