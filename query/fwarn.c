/*
 * ion/query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */


#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/focus.h>
#include <ioncore/genframe.h>
#include "wmessage.h"
#include <ioncore/objp.h>


void fwarn(WGenFrame *frame, const char *p)
{
	WMessage *wmsg;
	char *p2;
	
	if(p==NULL || genframe_current_input(frame)!=NULL)
		return;
	
	p2=scat("-Error- ", p);
	
	if(p2!=NULL){
		wmsg=(WMessage*)genframe_add_input(frame, (WRegionAddFn*)create_wmsg,
										   (void*)p2);
		free(p2);
	}else{
		wmsg=(WMessage*)genframe_add_input(frame, (WRegionAddFn*)create_wmsg,
										   (void*)p);
	}
	
	region_map((WRegion*)wmsg);

	if(REGION_IS_ACTIVE(frame)){
		set_focus((WRegion*)wmsg);
	}
}


void fwarn_free(WGenFrame *frame, char *p)
{
	if(p!=NULL){
		fwarn(frame, p);
		free(p);
	}
}
