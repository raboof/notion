/*
 * wmcore/winprops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "property.h"
#include "winprops.h"
#include "attach.h"


static WWinProp *winprop_list=NULL;


/*{{{ Static functions */


static WWinProp *do_find_winprop(WWinProp *list, const char *wclass,
								 const char *winstance)
{
	WWinProp *prop=list, *loosematch=NULL;
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


static WWinProp *do_find_winprop_win(WWinProp *list, Window win)
{
	char *winstance, *wclass=NULL;
	int n, tmp;
	
	winstance=get_string_property(win, XA_WM_CLASS, &n);
	
	if(winstance==NULL)
		return NULL;
	
	tmp=strlen(winstance);
	if(tmp+1<n)
		wclass=winstance+tmp+1;

	return do_find_winprop(list, wclass, winstance);
}


static void do_free_winprop(WWinProp **list, WWinProp *winprop)
{	
	if(winprop->prev!=NULL){
		UNLINK_ITEM(*list, winprop, next, prev);
	}
	
	if(winprop->data!=NULL)
		free(winprop->data);
	
	if(winprop->target_name!=NULL)
		free(winprop->target_name);
	
	free(winprop);
}


static void do_add_winprop(WWinProp **list, WWinProp *winprop)
{
	LINK_ITEM(*list, winprop, next, prev);
}


static bool init_winprop(WWinProp *winprop, const char *name)
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
	
	winprop->flags=0;
	winprop->manage_flags=REGION_ATTACH_SWITCHTO;
	winprop->stubborn=FALSE;
	winprop->max_w=winprop->max_h=0;
	winprop->aspect_w=winprop->aspect_h=0;
	winprop->target_name=NULL;
	
	return TRUE;
}


/*}}}*/


/*{{{ Interface */


WWinProp *alloc_winprop(const char *winname)
{
	char *wclass, *winstance;
	WWinProp *winprop;
	
	winprop=ALLOC(WWinProp);
	
	if(winprop==NULL){
		warn_err();
		return NULL;
	}
		
	if(!init_winprop(winprop, winname)){
		free(winprop);
		return NULL;
	}

	return winprop;
}


void add_winprop(WWinProp *winprop)
{
	do_add_winprop(&winprop_list, winprop);
}


WWinProp *find_winprop_win(Window win)
{
	return do_find_winprop_win(winprop_list, win);
}


void free_winprop(WWinProp *winprop)
{
	do_free_winprop(&winprop_list, winprop);
}


void free_winprops()
{
	while(winprop_list!=NULL)
		free_winprop(winprop_list);
}


/*}}}*/
