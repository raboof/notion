/*
 * wmcore/close.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "region.h"
#include "clientwin.h"
#include "objp.h"
#include "attach.h"


static void default_request_close(WRegion *reg)
{
	if(!region_rescue_clientwins(reg))
		return;
	destroy_thing((WThing*)reg);
}


void region_request_close(WRegion *reg)
{
	CALL_DYN(region_request_close, reg, (reg));
	if(funnotfound)
		default_request_close(reg);
}

/*
void close_propagate(WRegion *reg)
{
	WRegion *r;
	while(1){
		r=region_selected_sub(reg);
		if(r==NULL)
			break;
		reg=r;
	}
	
	region_request_close(reg);
}
*/

void close_sub(WRegion *reg)
{
	reg=region_selected_sub(reg);
	if(reg!=NULL)
		region_request_close(reg);
}

/*
void close_sub_propagate(WRegion *reg)
{
	reg=region_selected_sub(reg);
	if(reg!=NULL)
		close_propagate(reg);
}
*/

