/*
 * ion/ioncore/geom.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "geom.h"
#include "extl.h"


bool extltab_to_geom(ExtlTab tab, WRectangle *geomret)
{
	if(!extl_table_gets_i(tab, "x", &(geomret->x)) ||
	   !extl_table_gets_i(tab, "y", &(geomret->y)) ||
	   !extl_table_gets_i(tab, "w", &(geomret->w)) ||
	   !extl_table_gets_i(tab, "h", &(geomret->h)))
	   return FALSE;
	
	return TRUE;
}


ExtlTab geom_to_extltab(WRectangle geom)
{
	ExtlTab tab=extl_create_table();
	
	extl_table_sets_i(tab, "x", geom.x);
	extl_table_sets_i(tab, "y", geom.y);
	extl_table_sets_i(tab, "w", geom.w);
	extl_table_sets_i(tab, "h", geom.h);
	
	return tab;
}


void pgeom(const char *n, WRectangle g)
{
	fprintf(stderr, "%s %d, %d; %d, %d\n", n, g.x, g.y, g.w, g.h);
}

