/*
 * ion/ioncore/genws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "objp.h"
#include "region.h"
#include "genws.h"
#include "names.h"


/*{{{ Create/destroy */


void init_genws(WGenWS *ws, WWindow *parent, WRectangle geom)
{
	init_region(&(ws->reg), (WRegion*)parent, geom);
}


void deinit_genws(WGenWS *ws)
{
	deinit_region(&(ws->reg));
}


/*}}}*/


/*{{{ Names */


WGenWS *lookup_workspace(const char *name)
{
	return (WGenWS*)do_lookup_region(name, &OBJDESCR(WGenWS));
}


int complete_workspace(char *nam, char ***cp_ret, char **beg, void *unused)
{
	return do_complete_region(nam, cp_ret, beg, &OBJDESCR(WGenWS));
}


bool goto_workspace_name(const char *str)
{
	WGenWS *ws=lookup_workspace(str);
	
	if(ws==NULL)
		return FALSE;

	goto_region((WRegion*)ws);
	
	return TRUE;
}


/*}}}*/


/*{{{ Class implementation */


IMPLOBJ(WGenWS, WRegion, deinit_genws, NULL);


/*}}}*/

