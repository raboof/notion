/*
 * ion/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/screen.h>
#include <wmcore/clientwin.h>
#include <wmcore/attach.h>
#include <wmcore/wsreg.h>
#include "placement.h"
#include "frame.h"
#include "splitframe.h"
#include "winprops.h"


static WRegion *find_suitable_frame(WIonWS *ws)
{
	WRegion *r=ionws_find_current(ws);
	
	if(r!=NULL && region_supports_add_managed(r))
		return r;
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r){
		if(region_supports_add_managed(r))
			return r;
		
	}
	
	return NULL;
}


bool ionws_add_clientwin(WIonWS *ws, WClientWin *cwin,
						 const XWindowAttributes *attr,
						 int init_state, WWinProp *props)
{
	WRegion *target=NULL;
	bool geomset;

	if(props!=NULL && props->transient_mode==TRANSIENT_MODE_CURRENT){
		target=find_suitable_frame(ws);
		if(target!=NULL && WTHING_IS(target, WFrame)){
			if(((WFrame*)target)->current_sub!=NULL &&
			   WTHING_IS(((WFrame*)target)->current_sub, WClientWin)){
				return region_add_managed(((WFrame*)target)->current_sub,
										  (WRegion*)cwin, 0);
			}
		}
	}else{
#ifdef CF_PLACEMENT_GEOM
		geomset=(cwin->size_hints.win_gravity!=ForgetGravity &&
				 (attr->x>CF_STUBBORN_TRESH &&
				  attr->y>CF_STUBBORN_TRESH));
		if(geomset && (!props || !props->stubborn))
			target=(WRegion*)find_frame_at(ws, attr->x, attr->y);

		if(target==NULL)
#endif
			target=find_suitable_frame(ws);
	}
	
	if(target==NULL){
		warn("Ooops... could not find a frame to add client to.");
		return FALSE;
	}
	
	assert(SCREEN_OF(target)==SCREEN_OF(cwin));
	
	return finish_add_clientwin(target, cwin, init_state, props);
}


bool ionws_add_transient(WIonWS *ws, WClientWin *tfor, WClientWin *cwin,
						 const XWindowAttributes *attr,
						 int init_state, WWinProp *props)
{
	return region_add_managed((WRegion*)tfor, (WRegion*)cwin, 0);
}
