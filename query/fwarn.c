/*
 * ion/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */


#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/focus.h>
#include <src/frame.h>
#include "wmessage.h"
#include <wmcore/objp.h>


void fwarn(WFrame *frame, const char *p)
{
	WMessage *wmsg;
	char *p2;
	
	if(p==NULL || frame_current_input(frame)!=NULL)
		return;
	
	p2=scat("-Error- ", p);
	
	if(p2!=NULL){
		wmsg=(WMessage*)frame_attach_input_new(frame,
								(WRegionCreateFn*)create_wmsg, (void*)p2);
		free(p2);
	}else{
		wmsg=(WMessage*)frame_attach_input_new(frame,
								(WRegionCreateFn*)create_wmsg, (void*)p);
	}
	
	map_region((WRegion*)wmsg);

	if(REGION_IS_ACTIVE(frame)){
		set_focus((WRegion*)wmsg);
	}
}


void fwarn_free(WFrame *frame, char *p)
{
	if(p!=NULL){
		fwarn(frame, p);
		free(p);
	}
}
