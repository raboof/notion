/*
 * ion/ioncore/symlist.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SYMLIST_H
#define ION_IONCORE_SYMLIST_H

#include "common.h"


INTRSTRUCT(WSymlist);


DECLSTRUCT(WSymlist){
	void *symbol;
	WSymlist *next, *prev;
};


#define ITERATE_SYMLIST(TYPE, VAR, LIST)     \
	for((VAR)=(TYPE)symlist_init_iter(LIST); \
		(VAR)!=NULL;                         \
		(VAR)=(TYPE)symlist_iter())


bool symlist_insert(WSymlist **symlist, void *symbol);
void symlist_remove(WSymlist **symlist, void *symbol);
void symlist_clear(WSymlist **symlist);
void *symlist_init_iter(WSymlist *symlist);
void *symlist_iter();

#endif /* ION_IONCORE_SYMLIST_H */
