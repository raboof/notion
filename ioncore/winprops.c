/*
 * ion/ioncore/winprops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "global.h"
#include "property.h"
#include "winprops.h"
#include "attach.h"


static WWinProp *winprop_list=NULL;


/*{{{ Static functions */


static WWinProp *do_find_winprop(WWinProp *list, const char *wclass,
								 const char *wrole,
								 const char *winstance)
{
	WWinProp *prop=list;

	for(; prop!=NULL; prop=prop->next){
		if(prop->wclass!=NULL){
			if(wclass==NULL || strcmp(prop->wclass, wclass)!=0)
				continue;
		}

		if(prop->wrole!=NULL){
			if(wrole==NULL || strcmp(prop->wrole, wrole)!=0)
				continue;
		}

		if(prop->winstance!=NULL){
			if(winstance==NULL || strcmp(prop->winstance, winstance)!=0)
				continue;
		}
		
		return prop;
	}
	
	return NULL;
}


static WWinProp *do_find_winprop_win(WWinProp *list, Window win)
{
	char *winstance=NULL, *wclass=NULL, *wrole=NULL;
	int n=0, n2=0, tmp=0;
	WWinProp *prop;
	
	winstance=get_string_property(win, XA_WM_CLASS, &n);
	wrole=get_string_property(win, wglobal.atom_wm_window_role, &n2);
	
	if(winstance==NULL)
		return NULL;
	
	tmp=strlen(winstance);
	if(tmp+1<n)
		wclass=winstance+tmp+1;

	prop=do_find_winprop(list, wclass, wrole, winstance);
	
	if(winstance!=NULL)
		free(winstance);
	if(wrole!=NULL)
		free(wrole);
	
	return prop;
}


static void do_free_winprop(WWinProp **list, WWinProp *winprop)
{	
	if(winprop->prev!=NULL){
		UNLINK_ITEM(*list, winprop, next, prev);
	}
	
	if(winprop->wclass!=NULL)
		free(winprop->wclass);
	if(winprop->wrole!=NULL)
		free(winprop->wrole);
	if(winprop->winstance!=NULL)
		free(winprop->winstance);
	
	if(winprop->target_name!=NULL)
		free(winprop->target_name);
	
	free(winprop);
}


static void do_add_winprop(WWinProp **list, WWinProp *winprop)
{
	LINK_ITEM(*list, winprop, next, prev);
}


static bool init_winprop(WWinProp *winprop, const char *cls,
						 const char *role, const char *inst)
{
	winprop->next=NULL;
	winprop->prev=NULL;
	winprop->wclass=NULL;
	winprop->wrole=NULL;
	winprop->winstance=NULL;
	
	if(strcmp(cls, "*")){
		winprop->wclass=scopy(cls);
		if(winprop->wclass==NULL)
			goto fail;
	}

	if(strcmp(role, "*")){
		winprop->wrole=scopy(role);
		if(winprop->wrole==NULL)
			goto fail;
	}

	if(strcmp(inst, "*")){
		winprop->winstance=scopy(inst);
		if(winprop->winstance==NULL)
			goto fail;
	}
	
		
	winprop->flags=0;
	winprop->manage_flags=REGION_ATTACH_SWITCHTO;
	winprop->stubborn=FALSE;
	winprop->max_w=winprop->max_h=0;
	winprop->aspect_w=winprop->aspect_h=0;
	winprop->target_name=NULL;
	
	return TRUE;
	
fail:
	warn_err();
	
	if(winprop->wclass!=NULL){
		free(winprop->wclass);
		winprop->wclass=NULL;
	}

	if(winprop->wrole!=NULL){
		free(winprop->wrole);
		winprop->wrole=NULL;
	}
	
	return FALSE;
}


/*}}}*/


/*{{{ Interface */


WWinProp *alloc_winprop(const char *cls, const char *role,
						const char *instance)
{
	WWinProp *winprop;
	
	winprop=ALLOC(WWinProp);
	
	if(winprop==NULL){
		warn_err();
		return NULL;
	}
		
	if(!init_winprop(winprop, cls, role, instance)){
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
