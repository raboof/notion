/*
 * ion/ioncore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_DEFER_H
#define ION_IONCORE_DEFER_H

#include "common.h"

typedef void WDeferredAction(WObj*);

extern void ioncore_execute_deferred();
extern void ioncore_execute_deferred_on_list(void **list);

extern bool ioncore_defer_action(WObj *obj, WDeferredAction *action);
extern bool ioncore_defer_action_on_list(WObj *obj, WDeferredAction *action,
                                         void **list);
extern bool ioncore_defer_destroy(WObj *obj);

#endif /* ION_IONCORE_DEFER_H */
