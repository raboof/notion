/*
 * ion/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/screen.h>
#include <wmcore/property.h>
#include <wmcore/clientwin.h>
#include <wmcore/targetid.h>
#include <wmcore/attach.h>
#include "placement.h"
#include "frame.h"
#include "winprops.h"
#include "splitframe.h"



static WRegion *find_suitable_frame(WWorkspace *ws)
{
	WRegion *r=REGION_ACTIVE_SUB(ws);
	
	if(r!=NULL && region_supports_attach(r)) /* WTHING_IS(r, WFrame)) */
		return r;
	
	return (WRegion*)FIRST_THING(ws, WFrame);
}


bool workspace_add_clientwin(WWorkspace *ws, WClientWin *cwin,
							 const XWindowAttributes *attr,
							 int init_state, WWinProp *props)
{
	WRegion *target=NULL;
	bool geomset;

#ifdef CF_PLACEMENT_GEOM
	geomset=(cwin->size_hints.win_gravity!=ForgetGravity &&
			 (attr->x>CF_STUBBORN_TRESH &&
			  attr->y>CF_STUBBORN_TRESH));
	if(geomset && (!props || !props->stubborn))
		target=(WRegion*)find_frame_at(ws, attr->x, attr->y);

	if(target==NULL)
#endif
		target=find_suitable_frame(ws);

	
	if(target==NULL){
		warn("Ooops... could not find a frame to attach client to.");
		return FALSE;
	}
	
	assert(SCREEN_OF(target)==SCREEN_OF(cwin));
	
	return finish_add_clientwin(target, cwin, init_state, props);
}


bool workspace_add_transient(WRegion *reg, WClientWin *tfor,
							 WClientWin *cwin,
							 const XWindowAttributes *attr,
							 int init_state, WWinProp *props)
{
	return clientwin_attach_sub(tfor, cwin, 0);
}


