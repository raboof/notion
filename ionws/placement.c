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


static bool finish_add_transient(WRegion *reg, WClientWin *cwin,
								 const WAttachParams *param)
{
	WAttachParams param2;

	param2.flags=0;
	
	assert(reg!=NULL);
	
	if(clientwin_get_switchto(cwin))
		param2.flags|=REGION_ATTACH_SWITCHTO;
	
	if(param->flags&REGION_ATTACH_INITSTATE &&
	   param->init_state==IconicState)
		param2.flags=0;
	
	return region_add_managed(reg, (WRegion*)cwin, &param2);
}



bool ionws_add_clientwin(WIonWS *ws, WClientWin *cwin,
						 const WAttachParams *param)
{
	WRegion *target=NULL;
	int tm;
	
	if(param->flags&REGION_ATTACH_TFOR){
		if(finish_add_transient((WRegion*)param->tfor, cwin, 
								param))
			return TRUE;
	}
	
	tm=clientwin_get_transient_mode(cwin);
	if(tm==TRANSIENT_MODE_CURRENT){
		target=find_suitable_frame(ws);
		if(target!=NULL && WOBJ_IS(target, WGenFrame)){
			if(((WGenFrame*)target)->current_sub!=NULL &&
			   WOBJ_IS(((WGenFrame*)target)->current_sub, WClientWin)){
				if(finish_add_transient(((WGenFrame*)target)->current_sub,
										cwin, param))
					return TRUE;
			}
		}
	}else{
#ifndef CF_IONWS_IGNORE_USER_POSITION
		if(param->flags&REGION_ATTACH_MAPRQ &&
		   cwin->size_hints.flags&USPosition){
			/* MAPRQ implies GEOMRQ */
			
			/* Maybe gravity should be taken into account so that user
			 * specified position -0-0 would put to the frame in the
			 * lower right corner. 
			 */
			
			target=(WRegion*)find_frame_at(ws, param->geomrq.x,
										   param->geomrq.y);
		}
		if(target==NULL)
#endif
			target=find_suitable_frame(ws);
	}
	
	if(target==NULL){
		warn("Ooops... could not find a frame to add client to.");
		return FALSE;
	}
	
	assert(SCREEN_OF(target)==SCREEN_OF(cwin));
	
	return finish_add_clientwin(target, cwin, param);
}

