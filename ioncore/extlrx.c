/*
 * ion/ioncore/extlrx.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include <libmainloop/signal.h>
#include <libmainloop/defer.h>
#include <libmainloop/hooks.h>

#include "common.h"



/* TODO: Get rid of this kludge */



/*{{{ libmainloop/WTimer */


EXTL_CLASS(WTimer, Obj)


/*EXTL_DOC
 * Create a new timer.
 */
EXTL_EXPORT_AS(ioncore, create_timer)
WTimer *create_timer_extl_owned();


/*EXTL_DOC
 * Reset timer (clear handler and stop timing).
 */
EXTL_EXPORT_MEMBER
void timer_reset(WTimer *timer);


/*EXTL_DOC
 * Is timer active?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool timer_is_set(WTimer *timer);


/*EXTL_DOC
 * Set timer handler and timeout in milliseconds, and start timing.
 */
EXTL_EXPORT_AS(WTimer, set)
void timer_set_extl(WTimer *timer, uint msecs, ExtlFn fn);


/*}}}*/


/*{{{ libmainloop/hooks */


EXTL_CLASS(WHook, Obj)

    
/*EXTL_DOC
 * Find named hook \var{name}.
 */
EXTL_SAFE
EXTL_EXPORT_AS(ioncore, get_hook)
WHook *mainloop_get_hook(const char *name);


/*EXTL_DOC
 * Is \var{fn} hooked to hook \var{hk}?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool hook_listed(WHook *hk, ExtlFn efn);


/*EXTL_DOC
 * Add \var{efn} to the list of functions to be called when the
 * hook \var{hk} is triggered.
 */
EXTL_EXPORT_AS(WHook, add)
bool hook_add_extl(WHook *hk, ExtlFn efn);


/*EXTL_DOC
 * Remove \var{efn} from the list of functions to be called when the 
 * hook \var{hk} is triggered.
 */
EXTL_EXPORT_AS(WHook, remove)
bool hook_remove_extl(WHook *hk, ExtlFn efn);


/*}}}*/


/*{{{ libmainloop/defer */


/*EXTL_DOC
 * Defer action until return to program main loop.
 */
EXTL_SAFE /* special case - this function does modify state */
EXTL_EXPORT_AS(ioncore, defer)
bool mainloop_defer_extl(ExtlFn fn);


/*}}}*/


/*{{{ libtu */


/*EXTL_DOC
 * Issue a warning. How the message is displayed depends on the current
 * warning handler.
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_warn(const char *str)
{
    warn("%s", str);
}


EXTL_SAFE
EXTL_EXPORT
const char *ioncore_gettext(const char *s)
{
    if(s==NULL)
        return NULL;
    else
        return TR(s);
}


/*}}}*/

