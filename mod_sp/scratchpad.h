/*
 * ion/mod_sp/scratchpad.h
 *
 * Copyright (c) Tuomo Valkonen 2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_SP_SCRATCHPAD_H
#define ION_MOD_SP_SCRATCHPAD_H

#include <ioncore/common.h>
#include <ioncore/frame.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRCLASS(WScratchpad);

DECLCLASS(WScratchpad){
    WFrame frame;
    WFitParams last_fp;
    Watch notifywin_watch;
};

extern WScratchpad* create_scratchpad(WWindow *parent, const WFitParams *fp);
extern WRegion *scratchpad_load(WWindow *par, const WFitParams *fp,
                                ExtlTab tab);

extern void scratchpad_notify(WScratchpad *sp);
extern void scratchpad_unnotify(WScratchpad *sp);

#endif /* ION_MOD_SP_SCRATCHPAD_H */
