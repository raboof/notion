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

typedef void DeferredAction(WObj*);

extern bool defer_action(WObj *obj, DeferredAction *action);
extern bool defer_action_on_list(WObj *obj, DeferredAction *action,
								 void **list);
extern bool defer_destroy(WObj *obj);
extern void execute_deferred();
extern void execute_deferred_on_list(void **list);

#endif /* ION_IONCORE_DEFER_H */
