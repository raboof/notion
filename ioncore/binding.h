/*
 * ion/ioncore/binding.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_BINDING_H
#define ION_IONCORE_BINDING_H

#include <libtu/obj.h>
#include <libtu/rb.h>
#include <libtu/map.h>
#include "common.h"
#include "region.h"
#include <libextl/extl.h>


#define BINDING_KEYPRESS        0
#define BINDING_BUTTONPRESS     1
#define BINDING_BUTTONMOTION    2
#define BINDING_BUTTONCLICK     3
#define BINDING_BUTTONDBLCLICK  4

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
    bool waitrel;
    WBindmap *submap;
    ExtlFn func;
};



DECLSTRUCT(WRegBindingInfo){
    WBindmap *bindmap;
    WRegBindingInfo *next, *prev;
    WRegBindingInfo *bm_next, *bm_prev;
    WRegion *reg;
    WRegion *owner;
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
