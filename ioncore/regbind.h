/*
 * ion/regbind.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_REGBIND_H
#define INCLUDED_REGBIND_H

#include "global.h"
#include "common.h"
#include "region.h"
#include "binding.h"


extern bool region_add_bindmap(WRegion *reg, WBindmap *bindmap, bool grab);
extern bool region_add_bindmap_owned(WRegion *reg, WBindmap *bindmap,
									 bool grab, WRegion *owner);
extern void region_remove_bindmap(WRegion *reg, WBindmap *bindmap);
extern void region_remove_bindmap_owned(WRegion *reg, WBindmap *bindmap,
										WRegion *owner);
extern void region_remove_bindings(WRegion *reg);

extern WBinding *region_lookup_keybinding(WRegion *reg, const XKeyEvent *ev,
										  const WSubmapState *sc,
										  WRegion **binding_owner_ret);
extern WBinding *region_lookup_binding_area(WRegion *reg, int act, uint state,
											uint kcb, int area);

extern void rbind_binding_added(const WRegBindingInfo *rbind, const WBinding *binding,
								const WBindmap *bindmap);

#endif /* INCLUDED_REGBIND_H */

