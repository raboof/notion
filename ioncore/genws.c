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


/*{{{ Class implementation */


IMPLOBJ(WGenWS, WRegion, genws_deinit, NULL);


/*}}}*/

