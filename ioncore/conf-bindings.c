/*
 * ion/ioncore/conf-bindings.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#ifdef CF_SUN_F1X_REMAP
#include <X11/Sunkeysym.h>
#endif

#include <libtu/map.h>

#include "common.h"
#include "binding.h"
#include <libextl/readconfig.h>
#include "global.h"
#include <libextl/extl.h>
#include "conf-bindings.h"
#include "bindmaps.h"


/*{{{ parse_keybut */


#define MOD5_NDX 7

static StringIntMap state_map[]={
    {"Shift",       ShiftMask},
    {"Lock",        LockMask},
    {"Control",     ControlMask},
    {"Mod1",        Mod1Mask},
    {"Mod2",        Mod2Mask},
    {"Mod3",        Mod3Mask},
    {"Mod4",        Mod4Mask},
    {"Mod5",        Mod5Mask},
    {"AnyModifier", AnyModifier},
    {"NoModifier",  0},
    {NULL,          0},
};

static StringIntMap button_map[]={
    {"Button1",     Button1},
    {"Button2",     Button2},
    {"Button3",     Button3},
    {"Button4",     Button4},
    {"Button5",     Button5},
    {"Button6",     6},
    {"Button7",     7},
    {"AnyButton",   AnyButton},
    {NULL,          0},
};


bool ioncore_parse_keybut(const char *str, uint *mod_ret, uint *ksb_ret,
                          bool button, bool init_any)
{
    char *str2, *p, *p2;
    int keysym=NoSymbol, i;
    bool ret=FALSE;
    
    *ksb_ret=NoSymbol;
    *mod_ret=(init_any && !button ? AnyModifier : 0);
    
    str2=scopy(str);
    
    if(str2==NULL)
        return FALSE;

    p=str2;
    
    while(*p!='\0'){
        p2=strchr(p, '+');
        
        if(p2!=NULL)
            *p2='\0';
        
        if(!button){
            keysym=XStringToKeysym(p);
#ifdef CF_SUN_F1X_REMAP
            if(keysym==XK_F11)
                keysym=SunXK_F36;
            else if(keysym==XK_F12)
                keysym=SunXK_F37;
#endif
        }
        
        if(!button && keysym!=NoSymbol){
            int tmp;
            if(*ksb_ret!=NoSymbol){
                warn_obj(str, TR("Insane key combination."));
                break;
            }
            if(XKeysymToKeycode(ioncore_g.dpy, keysym)==0){
                warn_obj(str, TR("Could not convert keysym to keycode."));
                break;
            }
            *ksb_ret=keysym;
        }else{
            i=stringintmap_ndx(state_map, p);

            if(i<0){
                i=stringintmap_ndx(button_map, p);
                
                if(i<0){
                    warn(TR("Unknown button \"%s\"."), p);
                    break;
                }
            
                if(!button || *ksb_ret!=NoSymbol){
                    warn_obj(str, TR("Insane button combination."));
                    break;
                }
                *ksb_ret=button_map[i].value;
            }else{
                if(*mod_ret==AnyModifier){
                    if(!init_any){
                        warn_obj(str, TR("Insane modifier combination."));
                        break;
                    }else{
                        *mod_ret=state_map[i].value;
                    }
                }else{
                    if(*mod_ret!=0 && state_map[i].value==AnyModifier){
                        warn_obj(str, TR("Insane modifier combination."));
                        break;
                    }else{
                        *mod_ret|=state_map[i].value;
                    }
                }
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


/*{{{ bindmap_defbindings */


static bool do_action(WBindmap *bindmap, const char *str,
                      ExtlFn func, uint act, uint mod, uint ksb,
                      int area, bool wr)
{
    WBinding binding;
    
    if(wr && mod==0){
        warn(TR("Can not wait on modifiers when no modifiers set in \"%s\"."),
             str);
        wr=FALSE;
    }
    
    binding.wait=wr;
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
        warn(TR("Unable to add binding %s."), str);
    }else{
        binding.func=func;
        if(bindmap_remove_binding(bindmap, &binding))
            return TRUE;
        warn(TR("Unable to remove binding %s."), str);
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
        return bindmap_defbindings(bnd->submap, subtab, TRUE);

    binding.wait=FALSE;
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
        return bindmap_defbindings(binding.submap, subtab, TRUE);

    binding_deinit(&binding);
    
    warn(TR("Unable to add submap for binding %s."), str);
    
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


static bool do_entry(WBindmap *bindmap, ExtlTab tab, 
                     const StringIntMap *areamap, bool init_any)
{
    bool ret=FALSE;
    char *action_str=NULL, *ksb_str=NULL, *area_str=NULL;
    int action=0;
    uint ksb=0, mod=0;
    WBinding *bnd=NULL;
    ExtlTab subtab;
    ExtlFn func;
    bool wr=FALSE;
    int area=0;
    
    if(!extl_table_gets_s(tab, "action", &action_str)){
        warn(TR("Binding type not set."));
        goto fail;
    }

    if(strcmp(action_str, "kpress_wait")==0){
        action=BINDING_KEYPRESS;
        wr=TRUE;
    }else{
        action=stringintmap_value(action_map, action_str, -1);
        if(action<0){
            warn(TR("Unknown binding type \"%s\"."), action_str);
            goto fail;
        }
    }

    if(!extl_table_gets_s(tab, "kcb", &ksb_str))
        goto fail;

    if(!ioncore_parse_keybut(ksb_str, &mod, &ksb,
                             (action!=BINDING_KEYPRESS && action!=-1), 
                             init_any)){
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
                    warn(TR("Unknown area \"%s\" for binding %s."),
                         area_str, ksb_str);
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


bool bindmap_defbindings(WBindmap *bindmap, ExtlTab tab, bool submap)
{
    int i, n, nok=0;
    ExtlTab ent;
    
    n=extl_table_get_n(tab);
    
    for(i=1; i<=n; i++){
        if(extl_table_geti_t(tab, i, &ent)){
            nok+=do_entry(bindmap, ent, bindmap->areamap, submap);
            extl_unref_table(ent);
            continue;
        }
        warn(TR("Unable to get bindmap entry %d."), i);
    }
    return (nok!=0);
}


/*}}}*/


/*{{{ bindmap_getbindings */


static char *get_mods(uint state)
{
    char *ret=NULL;
    int i;
    
    if(state==AnyModifier){
        ret=scopy("AnyModifier+");
    }else{
        ret=scopy("");
        for(i=0; i<=MOD5_NDX; i++){
            if(ret==NULL)
                break;
            if((int)(state&state_map[i].value)==state_map[i].value){
                char *ret2=ret;
                ret=scat3(ret, state_map[i].string, "+");
                free(ret2);
            }
        }
    }
    
    return ret;
}


static char *get_key(char *mods, uint ksb)
{
    const char *s=XKeysymToString(ksb);
    char *ret=NULL;
    
    if(s==NULL){
        warn(TR("Unable to convert keysym to string."));
        return NULL;
    }
    
    return scat(mods, s);
}


static char *get_button(char *mods, uint ksb)
{
    const char *s=stringintmap_key(button_map, ksb, NULL);
    char *ret=NULL;
    
    if(s==NULL){
        warn(TR("Unable to convert button to string."));
        return NULL;
    }
    
    return scat(mods, s);
}


static bool get_kpress(WBindmap *bindmap, WBinding *b, ExtlTab t)
{
    char *mods;
    char *key;
    
    if(b->wait)
        extl_table_sets_s(t, "action", "kpress_wait");
    else
        extl_table_sets_s(t, "action", "kpress");
    
    mods=get_mods(b->state);
    
    if(mods==NULL)
        return FALSE;
    
    key=get_key(mods, b->ksb);

    free(mods);
    
    if(key==NULL)
        return FALSE;
    
    extl_table_sets_s(t, "kcb", key);
    
    free(key);
    
    if(b->submap!=NULL){
        ExtlTab stab=bindmap_getbindings(b->submap);
        extl_table_sets_t(t, "submap", stab);
    }else{
        extl_table_sets_f(t, "func", b->func);
    }
    
    return TRUE;
}


static bool get_mact(WBindmap *bindmap, WBinding *b, ExtlTab t)
{
    char *mods;
    char *button;
    
    extl_table_sets_s(t, "action", stringintmap_key(action_map, b->act, NULL));
    
    mods=get_mods(b->state);
    
    if(mods==NULL)
        return FALSE;
    
    button=get_button(mods, b->ksb);

    free(mods);
    
    if(button==NULL)
        return FALSE;
    
    extl_table_sets_s(t, "kcb", button);
    
    free(button);
    
    if(b->area!=0 && bindmap->areamap!=NULL)
        extl_table_sets_s(t, "area", 
                          stringintmap_key(bindmap->areamap, b->area, NULL));

    extl_table_sets_f(t, "func", b->func);
    
    return TRUE;
}


static ExtlTab getbinding(WBindmap *bindmap, WBinding *b)
{
    ExtlTab t=extl_create_table();
    
    if(b->act==BINDING_KEYPRESS){
        if(get_kpress(bindmap, b, t))
            return t;
    }else{
        if(get_mact(bindmap, b, t))
            return t;
    }
    
    return extl_unref_table(t);
}


ExtlTab bindmap_getbindings(WBindmap *bindmap)
{
    Rb_node node;
    WBinding *b;
    ExtlTab tab;
    ExtlTab btab;
    int n=0;
    
    tab=extl_create_table();
    
    FOR_ALL_BINDINGS(b, node, bindmap->bindings){
        btab=getbinding(bindmap, b);
        extl_table_seti_t(tab, n+1, btab);
        extl_unref_table(btab);
        n++;
    }
    
    return tab;
}


/*}}}*/

