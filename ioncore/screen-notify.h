/*
 * ion/ioncore/screen-notify.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_SCREEN_NOTIFY_H
#define ION_IONCORE_SCREEN_NOTIFY_H

#include "common.h"
#include "region.h"
#include "screen.h"

void screen_unnotify(WScreen *scr);
void screen_nowindowinfo(WScreen *scr);

void screen_managed_notify(WScreen *scr, WRegion *reg, WRegionNotify how);

void screen_update_infowin(WScreen *scr);
void screen_update_notifywin(WScreen *scr);

extern void ioncore_screen_activity_notify(WRegion *reg, WRegionNotify how);

#endif /* ION_IONCORE_SCREEN_NOTIFY_H */
