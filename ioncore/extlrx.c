/*
 * ion/ioncore/extlrx.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include <libmainloop/signal.h>

#include "common.h"


/*{{{ libmainloop (timers) */

EXTL_CLASS(WTimer, Obj);


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
EXTL_EXPORT_MEMBER
bool timer_is_set(WTimer *timer);


/*EXTL_DOC
 * Set timer handler and timeout in milliseconds, and start timing.
 */
EXTL_EXPORT_AS(WTimer, set)
void timer_set_extl(WTimer *timer, uint msecs, ExtlFn fn);


/*}}}*/


/*{{{ libtu */


/*EXTL_DOC
 * Issue a warning. How the message is displayed depends on the current
 * warning handler.
 */
EXTL_EXPORT
void ioncore_warn(const char *str)
{
    warn("%s", str);
}


EXTL_EXPORT
const char *ioncore_gettext(const char *s)
{
    if(s==NULL)
        return NULL;
    else
        return TR(s);
}


/*}}}*/

