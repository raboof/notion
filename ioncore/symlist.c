/*
 * ion/ioncore/symlist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "symlist.h"


bool add_to_symlist(WSymlist **symlist, void *symbol)
{
	WSymlist *node;
	
	if(symbol==NULL)
		return FALSE;
	
	node=ALLOC(WSymlist);
	
	if(node==NULL)
		return FALSE;
		
	node->symbol=symbol;
	
	LINK_ITEM_FIRST(*symlist, node, next, prev);
	
	return TRUE;
}


void remove_from_symlist(WSymlist **symlist, void *symbol)
{
	WSymlist *node=*symlist;
	
	while(node!=NULL){
		if(node->symbol==symbol){
			UNLINK_ITEM(*symlist, node, next, prev);
			free(node);
			return;
		}
		node=node->next;
	}
}


/* Warning: not thread-safe */


static WSymlist *next=NULL;


void *iter_symlist_init(WSymlist *symlist)
{
	if(symlist==NULL){
		next=NULL;
		return NULL;
	}
	
	next=symlist->next;
	return symlist->symbol;
}


void *iter_symlist()
{
	WSymlist *ret;
	
	if(next==NULL)
		return NULL;
	
	ret=next;
	next=next->next;
	
	return ret->symbol;
}

