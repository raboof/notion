/*
 * ion/ioncore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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
