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
extern void region_remove_bindmap(WRegion *reg, WBindmap *bindmap);
extern void region_remove_bindings(WRegion *reg);

extern void activate_ggrabs(WRegion *reg);
extern void release_ggrabs(WRegion *reg);

extern WBinding *region_lookup_keybinding(const WRegion *reg,
										  const XKeyEvent *ev,
										  bool grabonly, const WSubmapState *sc);
extern WBinding *region_lookup_binding_area(const WRegion *reg, int act,
											uint state, uint kcb, int area);

#endif /* INCLUDED_REGBIND_H */

