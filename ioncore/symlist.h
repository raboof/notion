/*
 * wmcore/symlist.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_SYMLIST_H
#define WMCORE_SYMLIST_H

#include "common.h"


INTRSTRUCT(WSymlist)


DECLSTRUCT(WSymlist){
	void *symbol;
	WSymlist *next, *prev;
};


#define ITERATE_SYMLIST(TYPE, VAR, LIST)     \
	for((VAR)=(TYPE)iter_symlist_init(LIST); \
		(VAR)!=NULL;                         \
		(VAR)=(TYPE)iter_symlist())


bool add_to_symlist(WSymlist **symlist, void *symbol);
void remove_from_symlist(WSymlist **symlist, void *symbol);
void *iter_symlist_init(WSymlist *symlist);
void *iter_symlist();

#endif /* WMCORE_SYMLIST_H */
