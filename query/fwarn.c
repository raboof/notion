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
void query_fwarn(WGenFrame *frame, const char *p)
{
	char *p2;
	
	if(p==NULL)
		return;
	
	p2=scat("-Error- ", p);
	
	if(p2==NULL){
		warn_err();
		return;
	}
	
	query_message(frame, p2);
	
	free(p2);
}

/*EXTL_DOC
 * Display a mesage in the frame \var{frame}.
 */
EXTL_EXPORT
void query_message(WGenFrame *frame, const char *p)
{
	WMessage *wmsg;
	
	if(p==NULL || genframe_current_input(frame)!=NULL)
		return;

	wmsg=(WMessage*)genframe_add_input(frame, (WRegionAddFn*)create_wmsg,
									   (void*)p);
	
	region_map((WRegion*)wmsg);

	if(REGION_IS_ACTIVE(frame))
		set_focus((WRegion*)wmsg);
}

