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
#include <ioncore/wsreg.h>
#include <ioncore/winprops.h>
#include "placement.h"
#include "ionframe.h"
#include "splitframe.h"
#include "ionws.h"


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
		if(target!=NULL && WOBJ_IS(target, WGenFrame)){
			if(((WGenFrame*)target)->current_sub!=NULL &&
			   WOBJ_IS(((WGenFrame*)target)->current_sub, WClientWin)){
				return region_add_managed(((WGenFrame*)target)->current_sub,
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
