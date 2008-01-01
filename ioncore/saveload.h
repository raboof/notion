/*
 * ion/ioncore/saveload.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_SAVELOAD_H
#define ION_IONCORE_SAVELOAD_H

#include <libextl/extl.h>
#include "common.h"
#include "region.h"
#include "screen.h"
#include "pholder.h"
#include "attach.h"

extern WRegion *create_region_load(WWindow *par, const WFitParams *fp, 
                                   ExtlTab tab, WPHolder **sm_ph_p);

extern bool region_supports_save(WRegion *reg);
DYNFUN ExtlTab region_get_configuration(WRegion *reg);
extern ExtlTab region_get_base_configuration(WRegion *reg);

extern bool ioncore_init_layout();
extern bool ioncore_save_layout();

/* Session management support */

typedef bool SMAddCallback(WPHolder *ph, ExtlTab tab);
typedef void SMCfgCallback(WClientWin *cwin, ExtlTab tab);
    
extern void ioncore_set_sm_callbacks(SMAddCallback *add, SMCfgCallback *cfg);
extern void ioncore_get_sm_callbacks(SMAddCallback **add, SMCfgCallback **cfg);

extern WPHolder *ioncore_get_load_pholder();

#endif /* ION_IONCORE_SAVELOAD_H */

