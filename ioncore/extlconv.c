/*
 * ion/ioncore/extlconv.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "extl.h"
#include "extlconv.h"


/*{{{ Geometries */


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


/*}}}*/


/*{{{ Region lists */


ExtlTab managed_list_to_table(WRegion *list, bool (*filter)(WRegion *r))
{
	ExtlTab tab=extl_create_table();
	int i=1;
	WRegion *r;
	
	FOR_ALL_MANAGED_ON_LIST(list, r){
		if(filter==NULL || filter(r)){
			if(extl_table_seti_o(tab, i, (WObj*)r))
				i++;
		}
	}
	
	return tab;
}


bool extl_table_is_bool_set(ExtlTab tab, const char *entry)
{
	bool b;
	
	if(extl_table_gets_b(tab, entry, &b))
		return b;
	return FALSE;
}


/*}}}*/
