/*
 * ion/ioncore/conf-bindings.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/map.h>

#include "common.h"
#include "binding.h"
#include "readconfig.h"
#include "global.h"
#include "extl.h"
#include "conf-bindings.h"


/*{{{ parse_keybut */


#define BUTTON1_NDX 9

static StringIntMap state_map[]={
	{"Shift",		ShiftMask},
	{"Lock",		LockMask},
	{"Control",		ControlMask},
	{"Mod1",		Mod1Mask},
	{"Mod2",		Mod2Mask},
	{"Mod3",		Mod3Mask},
	{"Mod4",		Mod4Mask},
	{"Mod5",		Mod5Mask},
	{"AnyModifier",	AnyModifier},
	{"Button1",		Button1},
	{"Button2",		Button2},
	{"Button3",		Button3},
	{"Button4",		Button4},
	{"Button5",		Button5},
	{"AnyButton",	AnyButton},
	{NULL,  		0},
};


static bool parse_keybut(const char *str, uint *mod_ret, uint *kcb_ret,
						 bool button)
{
	char *str2, *p, *p2;
	int keysym=NoSymbol, i;
	bool ret=FALSE;
	
	*kcb_ret=NoSymbol;
	*mod_ret=0;
	
	str2=scopy(str);
	
	if(str2==NULL)
		return FALSE;

	p=str2;
	
	while(*p!='\0'){
		p2=strchr(p, '+');
		
		if(p2!=NULL)
			*p2='\0';
		
		if(!button)
			keysym=XStringToKeysym(p);
		
		if(!button && keysym!=NoSymbol){
			if(*kcb_ret!=NoSymbol){
				warn_obj(str, "Insane key combination");
				break;
			}
			*kcb_ret=XKeysymToKeycode(wglobal.dpy, keysym);
			if(*kcb_ret==0){
				warn_obj(str, "Could not convert keysym to keycode");
				break;
			}
		}else{
			i=stringintmap_ndx(state_map, p);

			if(i<0){
				warn("\"%s\" unknown", p);
				break;
			}
			
			if(i>=BUTTON1_NDX){
				if(!button || *kcb_ret!=NoSymbol){
					warn_obj(str, "Insane button combination");
					break;
				}
				*kcb_ret=state_map[i].value;
			}else{
				*mod_ret|=state_map[i].value;
			}
		}

		if(p2==NULL){
			ret=TRUE;
			break;
		}
		
		p=p2+1;
	}

	free(str2);
	
	return ret;
}

#undef BUTTON1_NDX


/*}}}*/


/*{{{ process_bindings */


static bool do_action(WBindmap *bindmap, const char *str,
					  ExtlFn func, uint act, uint mod, uint kcb,
					  int area, bool wr)
{
	WBinding binding;
	
	if(wr && mod==0){
		warn("Cannot waitrel when no modifiers set in \"%s\". Sorry.", str);
		wr=FALSE;
	}

	binding.waitrel=wr;
	binding.act=act;
	binding.state=mod;
	binding.kcb=kcb;
	binding.area=area;
	binding.func=extl_ref_fn(func);
	binding.submap=NULL;
	
	if(add_binding(bindmap, &binding))
		return TRUE;

	deinit_binding(&binding);
	
	warn("Unable to add binding %s.", str);
	
	return FALSE;
}


static bool do_submap(WBindmap *bindmap, const char *str,
					  ExtlTab subtab, uint action, uint mod, uint kcb)
{
	WBinding binding, *bnd;

	bnd=lookup_binding(bindmap, action, mod, kcb);
	
	if(bnd!=NULL && bnd->submap!=NULL && bnd->state==mod)
		return process_bindings(bnd->submap, NULL, subtab);

	binding.waitrel=FALSE;
	binding.act=ACT_KEYPRESS;
	binding.state=mod;
	binding.kcb=kcb;
	binding.area=0;
	binding.func=extl_fn_none();
	binding.submap=create_bindmap();
	
	if(binding.submap==NULL)
		return FALSE;

	if(add_binding(bindmap, &binding)){
		binding.submap->parent=bindmap;
		return process_bindings(binding.submap, NULL, subtab);
	}

	deinit_binding(&binding);
	
	warn("Unable to add submap for binding %s.", str);
	
	return FALSE;
}


static StringIntMap action_map[]={
	{"kpress", ACT_KEYPRESS},
	{"mpress", ACT_BUTTONPRESS},
	{"mclick", ACT_BUTTONCLICK},
	{"mdblclick", ACT_BUTTONDBLCLICK},
	{"mdrag", ACT_BUTTONMOTION},
	{NULL, 0}
};


static bool do_entry(WBindmap *bindmap, ExtlTab tab, StringIntMap *areamap)
{
	bool ret=FALSE;
	char *action_str=NULL, *kcb_str=NULL, *area_str=NULL;
	int action;
	uint kcb, mod;
	WBinding *bnd;
	ExtlTab subtab;
	ExtlFn func;
	bool wr=FALSE;
	int area=0;
	
	if(!extl_table_gets_s(tab, "action", &action_str))
		goto fail;

	if(strcmp(action_str, "kpress_waitrel")==0){
		action=ACT_KEYPRESS;
		wr=TRUE;
	}else{
		action=stringintmap_value(action_map, action_str, -1);
		if(action<0){
			warn("Unknown binding action %s.", action_str);
			goto fail;
		}
	}

	if(!extl_table_gets_s(tab, "kcb", &kcb_str))
		goto fail;

	if(!parse_keybut(kcb_str, &mod, &kcb, (action!=ACT_KEYPRESS &&
										   action!=-1))){
		goto fail;
	}
	
	if(extl_table_gets_t(tab, "submap", &subtab)){
		ret=do_submap(bindmap, kcb_str, subtab, action, mod, kcb);
		extl_unref_table(subtab);
	}else{
		if(areamap!=NULL){
			if(extl_table_gets_s(tab, "area", &area_str)){
				area=stringintmap_value(areamap, area_str, -1);
				if(area<0){
					warn("Unknown area %s for binding %s.", area_str, kcb_str);
					area=0;
				}
			}
		}
		
		if(!extl_table_gets_f(tab, "func", &func)){
			warn("Function for binding %s not set/nil/undefined.", kcb_str);
			goto fail;
		}
		ret=do_action(bindmap, kcb_str, func, action, mod, kcb, area, wr);
		if(!ret)
			extl_unref_fn(func);
	}
	
fail:
	if(action_str!=NULL)
		free(action_str);
	if(kcb_str!=NULL)
		free(kcb_str);
	if(area_str!=NULL)
		free(area_str);
	return ret;
}


bool process_bindings(WBindmap *bindmap, StringIntMap *areamap, ExtlTab tab)
{
	int i, n, nok=0;
	ExtlTab ent;
	
	n=extl_table_get_n(tab);
	
	for(i=1; i<=n; i++){
		if(extl_table_geti_t(tab, i, &ent)){
			nok+=do_entry(bindmap, ent, areamap);
			extl_unref_table(ent);
			continue;
		}
		warn("Unable to get bindmap entry %d.", i);
	}
	return (nok!=0);
}


/*}}}*/

