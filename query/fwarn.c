/*
 * ion/query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/focus.h>
#include <ioncore/frame.h>
#include <libtu/objp.h>
#include "wmessage.h"
#include "fwarn.h"


/*EXTL_DOC
 * Display an error message box in the multiplexer \var{mplex}.
 */
EXTL_EXPORT
WMessage *mod_query_warn(WMPlex *mplex, const char *p)
{
    char *p2;
    WMessage *wmsg;
    
    if(p==NULL)
        return NULL;
    
    p2=scat("Error:\n", p);
    
    if(p2==NULL){
        warn_err();
        return NULL;
    }
    
    wmsg=mod_query_message(mplex, p2);
    
    free(p2);
    
    return wmsg;
}

/*EXTL_DOC
 * Display a message in the \var{mplex}.
 */
EXTL_EXPORT
WMessage *mod_query_message(WMPlex *mplex, const char *p)
{
    if(p==NULL)
        return NULL;

    return (WMessage*)mplex_attach_hnd(mplex, 
                                       (WRegionAttachHandler*)create_wmsg,
                                       (void*)p,
                                       MPLEX_ATTACH_L2|MPLEX_ATTACH_SWITCHTO);
}

