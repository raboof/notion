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

extern void screen_notify(WScreen *scr, const char *notstr);
extern void screen_unnotify(WScreen *scr);
extern void screen_windowinfo(WScreen *scr, const char *name);
extern void screen_nowindowinfo(WScreen *scr);

#endif /* ION_IONCORE_SCREEN_NOTIFY_H */
