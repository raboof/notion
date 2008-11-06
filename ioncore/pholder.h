/*
 * ion/ioncore/pholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2008. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_PHOLDER_H
#define ION_IONCORE_PHOLDER_H

#include "common.h"
#include "attach.h"
#include "extlconv.h"


#define PHOLDER_ATTACH_SWITCHTO 0x0001
#define PHOLDER_ATTACH_RETURN_CREATEROOT 0x0002


/* Note: PHolders should be destroyed by their acquirer. */

DECLCLASS(WPHolder){
    Obj obj;
};


extern bool pholder_init(WPHolder *ph);
extern void pholder_deinit(WPHolder *ph);

DYNFUN WRegion *pholder_do_attach(WPHolder *ph, int flags,
                                  WRegionAttachData *data);

extern WRegion *pholder_attach_(WPHolder *ph, int flags, 
                                WRegionAttachData *data);

extern bool pholder_attach(WPHolder *ph, int flags, WRegion *reg);

extern bool pholder_attach_mcfgoto(WPHolder *ph, int flags, WRegion *reg);

DYNFUN WRegion *pholder_do_target(WPHolder *ph);

extern WRegion *pholder_target(WPHolder *ph);

DYNFUN bool pholder_stale(WPHolder *ph);

DYNFUN bool pholder_do_check_reparent(WPHolder *ph, WRegion *reg);

extern bool pholder_check_reparent(WPHolder *ph, WRegion *reg);

DYNFUN bool pholder_do_goto(WPHolder *ph);

extern bool pholder_goto(WPHolder *ph);

extern WPHolder *pholder_either(WPHolder *a, WPHolder *b);

DYNFUN WPHolder *region_managed_get_pholder(WRegion *reg, WRegion *mgd);
DYNFUN WPHolder *region_get_rescue_pholder_for(WRegion *reg, WRegion *mgd);

extern WPHolder *region_get_rescue_pholder(WRegion *reg);

#endif /* ION_IONCORE_PHOLDER_H */
