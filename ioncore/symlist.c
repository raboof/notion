/*
 * wmcore/symlist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "symlist.h"


bool add_to_symlist(WSymlist **symlist, void *symbol)
{
	WSymlist *node;
	
	node=ALLOC(WSymlist);
	
	if(node==NULL)
		return FALSE;
		
	node->symbol=symbol;
	
	LINK_ITEM(*symlist, node, next, prev);
	
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


void *iter_symlist(WSymlist *symlist)
{
	/* Warning: not thread-safe */
	static WSymlist *next=NULL;
	void *ret;
	
	if(symlist!=NULL)
		next=symlist;
	
	if(next==NULL)
		return NULL;
	
	ret=next->symbol;
	next=next->next;
	
	return ret;
}

