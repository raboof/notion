/*
 * ion/ioncore/genws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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

