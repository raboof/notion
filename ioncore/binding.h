/*
 * ion/ioncore/binding.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_BINDING_H
#define ION_IONCORE_BINDING_H

#include <libtu/obj.h>
#include <libtu/rb.h>
#include <libtu/map.h>
#include "common.h"
#include "region.h"
#include <libextl/extl.h>


enum{
    BINDING_KEYPRESS,
    BINDING_BUTTONPRESS,
    BINDING_BUTTONMOTION,
    BINDING_BUTTONCLICK,
    BINDING_BUTTONDBLCLICK,
    BINDING_SUBMAP_ENTER
    /*BINDING_SUBMAP_LEAVE*/
};

#define BINDING_IS_PSEUDO(A) \
    ((A)==BINDING_SUBMAP_ENTER /*|| (A)==BINDING_SUBMAP_LEAVE*/)
    
#define BINDMAP_INIT        {0, NULL, NULL, NULL, NULL}

#define FOR_ALL_BINDINGS(B, NODE, MAP) \
        rb_traverse(NODE, MAP) if(((B)=(WBinding*)rb_val(NODE))!=NULL)

INTRSTRUCT(WBinding);
INTRSTRUCT(WBindmap);
INTRSTRUCT(WRegBindingInfo);


DECLSTRUCT(WBinding){
    uint kcb; /* keycode or button */
    uint ksb; /* keysym or button */
    uint state;
    uint act;
    int area;
    bool wait;
    WBindmap *submap;
    ExtlFn func;
};



DECLSTRUCT(WRegBindingInfo){
    WBindmap *bindmap;
    WRegBindingInfo *next, *prev;
    WRegBindingInfo *bm_next, *bm_prev;
    WRegion *reg;
    WRegion *owner;
    int tmp;
};



DECLSTRUCT(WBindmap){
    int nbindings;
    Rb_node bindings;
    WRegBindingInfo *rbind_list;
    const StringIntMap *areamap;
};


extern void ioncore_init_bindings();
extern void ioncore_update_modmap();
extern int ioncore_unmod(int state, int keycode);
extern bool ioncore_ismod(int keycode);
extern int ioncore_modstate();

extern WBindmap *create_bindmap();

extern void bindmap_destroy(WBindmap *bindmap);
extern void bindmap_refresh(WBindmap *bindmap);
extern bool bindmap_add_binding(WBindmap *bindmap, const WBinding *binding);
extern bool bindmap_remove_binding(WBindmap *bindmap, const WBinding *binding);
extern WBinding *bindmap_lookup_binding(WBindmap *bindmap, int act,
                                uint state, uint kcb);
extern WBinding *bindmap_lookup_binding_area(WBindmap *bindmap, int act,
                                     uint state, uint kcb, int area);

extern void binding_deinit(WBinding *binding);
extern void binding_grab_on(const WBinding *binding, Window win);
extern void binding_ungrab_on(const WBinding *binding, Window win);

#endif /* ION_IONCORE_BINDING_H */
