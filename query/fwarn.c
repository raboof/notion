/*
 * ion/query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/focus.h>
#include <ioncore/genframe.h>
#include <ioncore/objp.h>
#include "wmessage.h"
#include "fwarn.h"


/*EXTL_DOC
 * Display an error message box in the frame \var{frame}.
 */
EXTL_EXPORT
WMessage *query_fwarn(WMPlex *mplex, const char *p)
{
	char *p2;
	WMessage *wmsg;
	
	if(p==NULL)
		return NULL;
	
	p2=scat("-Error- ", p);
	
	if(p2==NULL){
		warn_err();
		return NULL;
	}
	
	wmsg=query_message(mplex, p2);
	
	free(p2);
	
	return wmsg;
}

/*EXTL_DOC
 * Display a mesage in the frame \var{frame}.
 */
EXTL_EXPORT
WMessage *query_message(WMPlex *mplex, const char *p)
{
	if(p==NULL || mplex_current_input(mplex)!=NULL)
		return NULL;

	return (WMessage*)mplex_add_input(mplex, (WRegionAddFn*)create_wmsg,
									  (void*)p);
}

