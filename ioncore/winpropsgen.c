/*
 * wmcore/winpropsgen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "property.h"
#include "winpropsgen.h"


WWinPropGen *find_winpropgen(WWinPropGen *list,
							 const char *wclass, const char *winstance)
{
	WWinPropGen *prop=list, *loosematch=NULL;
	int match, bestmatch=0;
	
	/* I assume there will not be that many winprops, so a naive algorithm
	 * and data structure should do. (linear search, linked list)
	 */
	
	for(; prop!=NULL; prop=prop->next){
		match=0;
		
		/* *.* -> 2
		 * *.bar -> 3
		 * foo.* -> 4
		 * foo.bar -> 5
		 */
		
		if(prop->wclass==NULL)
			match+=1;
		else if(wclass!=NULL && strcmp(prop->wclass, wclass)==0)
			match+=3;
		else
			continue;

		if(prop->winstance==NULL)
			match+=1;
		else if(winstance!=NULL && strcmp(prop->winstance, winstance)==0)
			match+=2;
		else
			continue;

		/* exact match? */
		if(match==5)
			return prop;
		
		if(match>bestmatch){
			bestmatch=match;
			loosematch=prop;
		}
	}
	
	return loosematch;
}


WWinPropGen *find_winpropgen_win(WWinPropGen *list, Window win)
{
	char *winstance, *wclass=NULL;
	int n, tmp;
	
	winstance=get_string_property(win, XA_WM_CLASS, &n);
	
	if(winstance==NULL)
		return NULL;
	
	tmp=strlen(winstance);
	if(tmp+1<n)
		wclass=winstance+tmp+1;

	return find_winpropgen(list, wclass, winstance);
}


void free_winpropgen(WWinPropGen **list, WWinPropGen *winprop)
{	
	if(winprop->prev!=NULL){
		UNLINK_ITEM(*list, winprop, next, prev);
	}
	
	if(winprop->data!=NULL)
		free(winprop->data);
}


void add_winpropgen(WWinPropGen **list, WWinPropGen *winprop)
{
	LINK_ITEM(*list, winprop, next, prev);
}


bool init_winpropgen(WWinPropGen *winprop, const char *name)
{
	char *wclass, *winstance;

	winprop->data=NULL;
	winprop->next=NULL;
	winprop->prev=NULL;
	
	wclass=scopy(name);

	if(wclass==NULL)
		return FALSE;

	winstance=strchr(wclass, '.');

	if(winstance!=NULL){
		*winstance++='\0';
		if(strcmp(winstance, "*")==0)
			winstance=NULL;
	}
	
	if(strcmp(wclass, "*")==0)
		wclass=NULL;
	
	winprop->data=wclass;
	winprop->wclass=wclass;
	winprop->winstance=winstance;
	
	return TRUE;
}
