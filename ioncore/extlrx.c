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


#include "common.h"


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

