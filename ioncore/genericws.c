/*
 * wmcore/genericws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "objp.h"
#include "region.h"
#include "genericws.h"
#include "names.h"


IMPLOBJ(WGenericWS, WRegion, deinit_genericws, NULL, NULL)


/*{{{ Create/destroy */


void init_genericws(WGenericWS *ws, WRegion *parent, WRectangle geom)
{
	init_region(&(ws->reg), parent, geom);
}


void deinit_genericws(WGenericWS *ws)
{
	deinit_region(&(ws->reg));
}


/*}}}*/


/*{{{ Names */


WGenericWS *lookup_workspace(const char *name)
{
	return (WGenericWS*)do_lookup_region(name, &OBJDESCR(WGenericWS));
}


int complete_workspace(char *nam, char ***cp_ret, char **beg, void *unused)
{
	return do_complete_region(nam, cp_ret, beg, &OBJDESCR(WGenericWS));
}


bool goto_workspace_name(const char *str)
{
	WGenericWS *ws=lookup_workspace(str);
	
	if(ws==NULL)
		return FALSE;

	goto_region((WRegion*)ws);
	
	return TRUE;
}


/*}}}*/

