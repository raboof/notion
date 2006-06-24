/*
 * ion/ioncore/regbind.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_REGBIND_H
#define ION_IONCORE_REGBIND_H

#include "global.h"
#include "common.h"
#include "region.h"
#include "binding.h"


extern bool region_add_bindmap(WRegion *reg, WBindmap *bindmap);
extern bool region_add_bindmap_owned(WRegion *reg, WBindmap *bindmap,
                                     WRegion *owner);
extern void region_remove_bindmap(WRegion *reg, WBindmap *bindmap);
extern void region_remove_bindmap_owned(WRegion *reg, WBindmap *bindmap,
                                        WRegion *owner);
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

