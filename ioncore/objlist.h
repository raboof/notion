/*
 * ion/ioncore/objlist.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_OBJLIST_H
#define ION_IONCORE_OBJLIST_H

#include "common.h"
#include "obj.h"


INTRSTRUCT(WObjList);


DECLSTRUCT(WObjList){
	WWatch watch; /* Must be kept at head of structure */
	WObjList *next, *prev;
	WObjList **list;
};


#define ITERATE_OBJLIST(TYPE, VAR, LIST)     \
	for((VAR)=(TYPE)objlist_init_iter(LIST); \
		(VAR)!=NULL;                         \
		(VAR)=(TYPE)objlist_iter())


bool objlist_insert(WObjList **objlist, WObj *obj);
void objlist_remove(WObjList **objlist, WObj *obj);
void objlist_clear(WObjList **objlist);
WObj *objlist_init_iter(WObjList *objlist);
WObj *objlist_iter();

#endif /* ION_IONCORE_OBJLIST_H */
