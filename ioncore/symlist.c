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


static void free_node(WSymlist **symlist, WSymlist *node)
{
	UNLINK_ITEM(*symlist, node, next, prev);
	free(node);
}


bool symlist_insert(WSymlist **symlist, void *symbol)
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


void symlist_remove(WSymlist **symlist, void *symbol)
{
	WSymlist *node=*symlist;
	
	while(node!=NULL){
		if(node->symbol==symbol){
			free_node(symlist, node);
			return;
		}
		node=node->next;
	}
}


void symlist_clear(WSymlist **symlist)
{
	while(*symlist!=NULL)
		free_node(symlist, *symlist);
}


/* Warning: not thread-safe */


static WSymlist *iter_next=NULL;


void *symlist_init_iter(WSymlist *symlist)
{
	if(symlist==NULL){
		iter_next=NULL;
		return NULL;
	}
	
	iter_next=symlist->next;
	return symlist->symbol;
}


void *symlist_iter()
{
	WSymlist *ret;
	
	if(iter_next==NULL)
		return NULL;
	
	ret=iter_next;
	iter_next=iter_next->next;
	
	return ret->symbol;
}

