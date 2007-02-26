/*
 * ion/ioncore/screen-notify.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SCREEN_NOTIFY_H
#define ION_IONCORE_SCREEN_NOTIFY_H

#include "common.h"
#include "region.h"
#include "screen.h"


void screen_managed_notify(WScreen *scr, WRegion *reg, WRegionNotify how);

void screen_update_infowin(WScreen *scr);
void screen_update_notifywin(WScreen *scr);

extern void ioncore_screen_activity_notify(WRegion *reg, WRegionNotify how);

#endif /* ION_IONCORE_SCREEN_NOTIFY_H */
