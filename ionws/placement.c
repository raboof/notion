/*
 * ion/ionws/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/screen.h>
#include <ioncore/clientwin.h>
#include <ioncore/attach.h>
#include <ioncore/manage.h>
#include <ioncore/extl.h>
#include "placement.h"
#include "ionframe.h"
#include "splitframe.h"
#include "ionws.h"


static WRegion *find_suitable_frame(WIonWS *ws)
{
	WRegion *r=ionws_current(ws);
	
	if(r!=NULL && region_supports_add_managed(r))
		return r;
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r){
		if(region_supports_add_managed(r))
			return r;
		
	}
	
	return NULL;
}


bool ionws_add_clientwin(WIonWS *ws, WClientWin *cwin,
						 const WAttachParams *param)
{
	WRegion *target=NULL;
	bool uspos;
	
	if(clientwin_get_transient_mode(cwin)==TRANSIENT_MODE_CURRENT){
		target=find_suitable_frame(ws);
		if(target!=NULL && WOBJ_IS(target, WGenFrame)){
			if(((WGenFrame*)target)->current_sub!=NULL &&
			   WOBJ_IS(((WGenFrame*)target)->current_sub, WClientWin)){
				if(finish_add_clientwin(((WGenFrame*)target)->current_sub,
										cwin, param))
					return TRUE;
			}
		}
	}else{
		bool uspos=(param->flags&REGION_ATTACH_MAPRQ &&
					cwin->size_hints.flags&USPosition);

		if(target==NULL){
			extl_call_named("ionws_placement_method", 
							"oob", "o", ws, cwin, uspos, &target);
			if(target!=NULL && !WOBJ_IS(target, WRegion))
				target=NULL;
		}
		
		if(target==NULL)
			target=find_suitable_frame(ws);
	}
	
	if(target==NULL){
		warn("Ooops... could not find a frame to add client to.");
		return FALSE;
	}
	
	return finish_add_clientwin(target, cwin, param);
}

