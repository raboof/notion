/*
 * ion/libmainloop/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_LIBMAINLOOP_DEFER_H
#define ION_LIBMAINLOOP_DEFER_H

#include <libtu/types.h>
#include <libtu/obj.h>
#include <libextl/extl.h>

INTRSTRUCT(WDeferred);

typedef void WDeferredAction(Obj*);

extern void mainloop_execute_deferred();
extern void mainloop_execute_deferred_on_list(WDeferred **list);

extern bool mainloop_defer_action(Obj *obj, WDeferredAction *action);
extern bool mainloop_defer_action_on_list(Obj *obj, WDeferredAction *action,
                                          WDeferred **list);

extern bool mainloop_defer_destroy(Obj *obj);

extern bool mainloop_defer_extl(ExtlFn fn);
extern bool mainloop_defer_extl_on_list(ExtlFn fn, WDeferred **list);

#endif /* ION_LIBMAINLOOP_DEFER_H */
