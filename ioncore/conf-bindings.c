/*
 * ion/ioncore/conf-bindings.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
    {"Shift",        ShiftMask},
    {"Lock",        LockMask},
    {"Control",        ControlMask},
    {"Mod1",        Mod1Mask},
    {"Mod2",        Mod2Mask},
    {"Mod3",        Mod3Mask},
    {"Mod4",        Mod4Mask},
    {"Mod5",        Mod5Mask},
    {"AnyModifier",    AnyModifier},
    {"Button1",        Button1},
    {"Button2",        Button2},
    {"Button3",        Button3},
    {"Button4",        Button4},
    {"Button5",        Button5},
    {"Button6",        6},
    {"Button7",        7},
    {"AnyButton",    AnyButton},
    {NULL,          0},
};


static bool parse_keybut(const char *str, uint *mod_ret, uint *ksb_ret,
                         bool button)
{
    char *str2, *p, *p2;
    int keysym=NoSymbol, i;
    bool ret=FALSE;
    
    *ksb_ret=NoSymbol;
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
            int tmp;
            if(*ksb_ret!=NoSymbol){
                warn_obj(str, "Insane key combination");
                break;
            }
            if(XKeysymToKeycode(ioncore_g.dpy, keysym)==0){
                warn_obj(str, "Could not convert keysym to keycode");
                break;
            }
            *ksb_ret=keysym;
        }else{
            i=stringintmap_ndx(state_map, p);

            if(i<0){
                warn("\"%s\" unknown", p);
                break;
            }
            
            if(i>=BUTTON1_NDX){
                if(!button || *ksb_ret!=NoSymbol){
                    warn_obj(str, "Insane button combination");
                    break;
                }
                *ksb_ret=state_map[i].value;
            }else{
                if(*mod_ret==AnyModifier || 
                   (*mod_ret!=0 && state_map[i].value==AnyModifier)){
                    warn_obj(str, "Insane modifier combination");
                    break;
                }
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


/*{{{ bindmap_do_table */


static bool do_action(WBindmap *bindmap, const char *str,
                      ExtlFn func, uint act, uint mod, uint ksb,
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
    binding.ksb=ksb;
    binding.kcb=(act==BINDING_KEYPRESS ? XKeysymToKeycode(ioncore_g.dpy, ksb) : ksb);
    binding.area=area;
    binding.submap=NULL;
    
    if(func!=extl_fn_none()){
        binding.func=extl_ref_fn(func);
        if(bindmap_add_binding(bindmap, &binding))
            return TRUE;
        extl_unref_fn(binding.func);
        warn("Unable to add binding %s.", str);
    }else{
        binding.func=func;
        if(bindmap_remove_binding(bindmap, &binding))
            return TRUE;
        warn("Unable to remove binding %s. Either you are trying to "
             "remove a binding that has not been set or you're trying "
             "to bind to a nil function", str);
    }

    return FALSE;
}


static bool do_submap(WBindmap *bindmap, const char *str,
                      ExtlTab subtab, uint action, uint mod, uint ksb)
{
    WBinding binding, *bnd;
    uint kcb;

    if(action!=BINDING_KEYPRESS)
        return FALSE;
    
    kcb=XKeysymToKeycode(ioncore_g.dpy, ksb);
    bnd=bindmap_lookup_binding(bindmap, action, mod, kcb);
    
    if(bnd!=NULL && bnd->submap!=NULL && bnd->state==mod)
        return bindmap_do_table(bnd->submap, NULL, subtab);

    binding.waitrel=FALSE;
    binding.act=BINDING_KEYPRESS;
    binding.state=mod;
    binding.ksb=ksb;
    binding.kcb=kcb;
    binding.area=0;
    binding.func=extl_fn_none();
    binding.submap=create_bindmap();
    
    if(binding.submap==NULL)
        return FALSE;

    if(bindmap_add_binding(bindmap, &binding))
        return bindmap_do_table(binding.submap, NULL, subtab);

    binding_deinit(&binding);
    
    warn("Unable to add submap for binding %s.", str);
    
    return FALSE;
}


static StringIntMap action_map[]={
    {"kpress", BINDING_KEYPRESS},
    {"mpress", BINDING_BUTTONPRESS},
    {"mclick", BINDING_BUTTONCLICK},
    {"mdblclick", BINDING_BUTTONDBLCLICK},
    {"mdrag", BINDING_BUTTONMOTION},
    {NULL, 0}
};


static bool do_entry(WBindmap *bindmap, ExtlTab tab, StringIntMap *areamap)
{
    bool ret=FALSE;
    char *action_str=NULL, *ksb_str=NULL, *area_str=NULL;
    int action;
    uint ksb, mod;
    WBinding *bnd;
    ExtlTab subtab;
    ExtlFn func;
    bool wr=FALSE;
    int area=0;
    
    if(!extl_table_gets_s(tab, "action", &action_str)){
        warn("Invalid action set for binding.");
        goto fail;
    }

    if(strcmp(action_str, "kpress_waitrel")==0){
        action=BINDING_KEYPRESS;
        wr=TRUE;
    }else{
        action=stringintmap_value(action_map, action_str, -1);
        if(action<0){
            warn("Unknown binding action %s.", action_str);
            goto fail;
        }
    }

    if(!extl_table_gets_s(tab, "kcb", &ksb_str))
        goto fail;

    if(!parse_keybut(ksb_str, &mod, &ksb, (action!=BINDING_KEYPRESS &&
                                           action!=-1))){
        goto fail;
    }
    
    if(extl_table_gets_t(tab, "submap", &subtab)){
        ret=do_submap(bindmap, ksb_str, subtab, action, mod, ksb);
        extl_unref_table(subtab);
    }else{
        if(areamap!=NULL){
            if(extl_table_gets_s(tab, "area", &area_str)){
                area=stringintmap_value(areamap, area_str, -1);
                if(area<0){
                    warn("Unknown area %s for binding %s.", area_str, ksb_str);
                    area=0;
                }
            }
        }
        
        if(!extl_table_gets_f(tab, "func", &func)){
            /*warn("Function for binding %s not set/nil/undefined.", ksb_str);
            goto fail;*/
            func=extl_fn_none();
        }
        ret=do_action(bindmap, ksb_str, func, action, mod, ksb, area, wr);
        if(!ret)
            extl_unref_fn(func);
    }
    
fail:
    if(action_str!=NULL)
        free(action_str);
    if(ksb_str!=NULL)
        free(ksb_str);
    if(area_str!=NULL)
        free(area_str);
    return ret;
}


bool bindmap_do_table(WBindmap *bindmap, StringIntMap *areamap, ExtlTab tab)
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

