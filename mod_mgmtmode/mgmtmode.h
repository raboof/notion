/*
 * ion/mod_mgmtmode/mgmtmode.h
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
