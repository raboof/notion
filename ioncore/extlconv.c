/*
 * ion/ioncore/extlconv.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "extl.h"
#include "extlconv.h"
#include "region-iter.h"


/*{{{ Region list */


ExtlTab managed_list_to_table(WRegion *list, bool (*filter)(WRegion *r))
{
	ExtlTab tab=extl_create_table();
	int i=1;
	WRegion *r;
	
	FOR_ALL_MANAGED_ON_LIST(list, r){
		if(filter==NULL || filter(r)){
			extl_table_seti_o(tab, i, (Obj*)r);
			i++;
		}
	}
	
	return tab;
}


/*}}}*/


/*{{{ Booleans */


bool extl_table_is_bool_set(ExtlTab tab, const char *entry)
{
	bool b;
	
	if(extl_table_gets_b(tab, entry, &b))
		return b;
	return FALSE;
}


/*}}}*/


/*{{{ Rectangles */


bool extl_table_to_rectangle(ExtlTab tab, WRectangle *rectret)
{
	if(!extl_table_gets_i(tab, "x", &(rectret->x)) ||
	   !extl_table_gets_i(tab, "y", &(rectret->y)) ||
	   !extl_table_gets_i(tab, "w", &(rectret->w)) ||
	   !extl_table_gets_i(tab, "h", &(rectret->h)))
	   return FALSE;
	
	return TRUE;
}


ExtlTab extl_table_from_rectangle(const WRectangle *rect)
{
	ExtlTab tab=extl_create_table();
	
	extl_table_sets_i(tab, "x", rect->x);
	extl_table_sets_i(tab, "y", rect->y);
	extl_table_sets_i(tab, "w", rect->w);
	extl_table_sets_i(tab, "h", rect->h);
	
	return tab;
}


void extl_table_sets_rectangle(ExtlTab tab, const char *nam, 
                               const WRectangle *rect)
{
	ExtlTab g=extl_table_from_rectangle(rect);
	extl_table_sets_t(tab, nam, g);
	extl_unref_table(g);
}


bool extl_table_gets_rectangle(ExtlTab tab, const char *nam, 
                               WRectangle *rect)
{
	ExtlTab g;
	bool ok;
	
	if(!extl_table_gets_t(tab, nam, &g))
		return FALSE;
	
	ok=extl_table_to_rectangle(g, rect);
	
	extl_unref_table(g);
	
	return ok;
}


/*}}}*/

