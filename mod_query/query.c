/*
 * ion/mod_query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <unistd.h>
#include <string.h>

#include <libextl/extl.h>
#include "query.h"
#include "wedln.h"


/*--lowlevel routine not to be called by the user--EXTL_DOC
 * Show a query window in \var{mplex} with prompt \var{prompt}, initial
 * contents \var{dflt}. The function \var{handler} is called with
 * the entered string as the sole argument when \fnref{WEdln.finish}
 * is called. The function \var{completor} is called with the created
 * \type{WEdln} is first argument and the string to complete is the
 * second argument when \fnref{WEdln.complete} is called.
 */
EXTL_EXPORT
WEdln *mod_query_do_query(WMPlex *mplex, const char *prompt, const char *dflt,
                          ExtlFn handler, ExtlFn completor)
{
    WRectangle geom;
    WEdlnCreateParams fnp;
    WMPlexAttachParams par;

    fnp.prompt=prompt;
    fnp.dflt=dflt;
    fnp.handler=handler;
    fnp.completor=completor;
    
    par.flags=(MPLEX_ATTACH_SWITCHTO|
               MPLEX_ATTACH_L2|
               MPLEX_ATTACH_L2_SEMIMODAL|
               MPLEX_ATTACH_SIZEPOLICY);
    par.szplcy=MPLEX_SIZEPOLICY_FULL_BOUNDS;

    return (WEdln*)mplex_do_attach(mplex,
                                   (WRegionAttachHandler*)create_wedln,
                                   (void*)&fnp, 
                                   &par);
}

