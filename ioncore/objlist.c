/*
 * ion/ioncore/objlist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "objlist.h"


static WObjList *iter_next=NULL;


static void free_node(WObjList **objlist, WObjList *node)
{
	UNLINK_ITEM(*objlist, node, next, prev);
	free(node);
}

	
void watch_handler(WWatch *watch, WObj *obj)
{
	WObjList *node=(WObjList*)watch;
	WObjList **list=node->list;
	
	if(iter_next==node)
		iter_next=node->next;
	
	free_node(list, node);
}



bool objlist_insert(WObjList **objlist, WObj *obj)
{
	WObjList *node;
	
	if(obj==NULL)
		return FALSE;
	
	node=ALLOC(WObjList);
	
	if(node==NULL)
		return FALSE;
		
	watch_init(&(node->watch));
	watch_setup(&(node->watch), obj, watch_handler);
	node->list=objlist;
	
	LINK_ITEM_FIRST(*objlist, node, next, prev);
	
	return TRUE;
}


void objlist_remove(WObjList **objlist, WObj *obj)
{
	WObjList *node=*objlist;
	
	while(node!=NULL){
		if(node->watch.obj==obj){
			watch_reset(&(node->watch));
			free_node(objlist, node);
			return;
		}
		node=node->next;
	}
}


void objlist_clear(WObjList **objlist)
{
	while(*objlist!=NULL){
		watch_reset(&((*objlist)->watch));
		free_node(objlist, *objlist);
	}
}


/* Warning: not thread-safe */


WObj *objlist_init_iter(WObjList *objlist)
{
	if(objlist==NULL){
		iter_next=NULL;
		return NULL;
	}
	
	iter_next=objlist->next;
	return objlist->watch.obj;
}


WObj *objlist_iter()
{
	WObjList *ret;
	
	if(iter_next==NULL)
		return NULL;
	
	ret=iter_next;
	iter_next=iter_next->next;
	
	return ret->watch.obj;
}
