/*
 * ion/mod_mgmtmode/mgmtmode.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_MGMTMODE_MGMTMODE_H
#define ION_MOD_MGMTMODE_MGMTMODE_H

#include <ioncore/common.h>
#include <libextl/extl.h>

INTRCLASS(WMgmtMode);

DECLCLASS(WMgmtMode){
    Obj obj;
    Watch selw;
};


extern WMgmtMode *mod_mgmtmode_begin(WRegion *reg);

extern void mgmtmode_select(WMgmtMode *mode, WRegion *reg);
extern WRegion *mgmtmode_selected(WMgmtMode *mode);

extern void mgmtmode_finish(WMgmtMode *mode);

#endif /* ION_MOD_MGMTMODE_MGMTMODE_H */
