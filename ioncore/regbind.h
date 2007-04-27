/*
 * ion/ioncore/regbind.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_REGBIND_H
#define ION_IONCORE_REGBIND_H

#include "global.h"
#include "common.h"
#include "region.h"
#include "binding.h"


DECLSTRUCT(WSubmapState){
    uint key, state;
    WSubmapState *next;
};


extern bool region_add_bindmap(WRegion *reg, WBindmap *bindmap);
extern void region_remove_bindmap(WRegion *reg, WBindmap *bindmap);

extern void region_remove_bindings(WRegion *reg);

extern WBinding *region_lookup_keybinding(WRegion *reg, const XKeyEvent *ev,
                                          const WSubmapState *sc,
                                          WRegion **binding_owner_ret);
extern WBinding *region_lookup_binding(WRegion *reg, int act, uint state,
                                            uint kcb, int area);

extern void rbind_binding_added(const WRegBindingInfo *rbind, 
                                const WBinding *binding,
                                const WBindmap *bindmap);
extern void rbind_binding_removed(const WRegBindingInfo *rbind, 
                                  const WBinding *binding,
                                  const WBindmap *bindmap);

extern void region_update_owned_grabs(WRegion *reg);
extern void region_do_update_owned_grabs(WRegion *reg);

#endif /* ION_IONCORE_REGBIND_H */

