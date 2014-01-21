/*
 * ion/ioncore/extlrx.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */


#include "common.h"


/*{{{ libtu */


/*EXTL_DOC
 * Issue a warning. How the message is displayed depends on the current
 * warning handler.
 */
EXTL_SAFE
EXTL_UNTRACED
EXTL_EXPORT
void ioncore_warn(const char *str)
{
    warn("%s", str);
}


/*EXTL_DOC
 * Similar to \fnref{ioncore.warn}, but also print Lua stack trace.
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_warn_traced(const char *str)
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

