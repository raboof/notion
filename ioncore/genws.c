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


void genws_init(WGenWS *ws, WWindow *parent, WRectangle geom)
{
	region_init(&(ws->reg), (WRegion*)parent, geom);
}


void genws_deinit(WGenWS *ws)
{
	region_deinit(&(ws->reg));
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

	region_goto((WRegion*)ws);
	
	return TRUE;
}


/*}}}*/


/*{{{ Class implementation */


IMPLOBJ(WGenWS, WRegion, genws_deinit, NULL);


/*}}}*/

