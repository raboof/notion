/*
 * ion/ioncore/reginfo.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_REGINFO_H
#define ION_IONCORE_REGINFO_H

#include "common.h"
#include <libtu/obj.h>
#include "region.h"
#include "window.h"
#include <libextl/extl.h>
#include "rectangle.h"

typedef WRegion *WRegionLoadCreateFn(WWindow *par, const WFitParams *fp,
                                     ExtlTab tab);
typedef WRegion *WRegionSimpleCreateFn(WWindow *par, const WFitParams *fp);

INTRSTRUCT(WRegClassInfo);
    
DECLSTRUCT(WRegClassInfo){
    ClassDescr *descr;
    WRegionLoadCreateFn *lc_fn;
    WRegClassInfo *next, *prev;
};


extern bool ioncore_register_regclass(ClassDescr *descr,
                                      WRegionLoadCreateFn *lc_fn);
extern void ioncore_unregister_regclass(ClassDescr *descr);

extern WRegClassInfo *ioncore_lookup_regclass(const char *name, 
                                              bool inheriting_ok);

#endif /* ION_IONCORE_REGINFO_H */

