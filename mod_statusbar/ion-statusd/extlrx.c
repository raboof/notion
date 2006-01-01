/*
 * ion/mod_statusbar/ion-statusd/extlrx.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2006.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libextl/extl.h>
#include <libtu/output.h>
#include <libtu/locale.h>


/*{{{ libtu */


/*EXTL_DOC
 * Issue a warning. How the message is displayed depends on the current
 * warning handler.
 */
EXTL_EXPORT
void statusd_warn(const char *str)
{
    warn("%s", str);
}


EXTL_EXPORT
const char *statusd_gettext(const char *s)
{
    if(s==NULL)
        return NULL;
    else
        return TR(s);
}


/*}}}*/

