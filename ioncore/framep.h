/*
 * ion/ioncore/framep.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAMEP_H
#define ION_IONCORE_FRAMEP_H

#include "frame.h"

#define FRAME_WIN(FRAME) (((WFrame*)(FRAME))->mplex.win.win)
#define FRAME_DRAW(FRAME) (((WFrame*)(FRAME))->mplex.win.draw)
#define FRAME_MCOUNT(FRAME) (((WFrame*)(FRAME))->mplex.l1_count)
#define FRAME_MLIST(FRAME) (((WFrame*)(FRAME))->mplex.l1_list)
#define FRAME_CURRENT(FRAME) (((WFrame*)(FRAME))->mplex.l1_current)

enum{
    FRAME_AREA_NONE=0,
    FRAME_AREA_BORDER=1,
    FRAME_AREA_TAB=2,
    FRAME_AREA_CLIENT=3
};

#define IONCORE_EVENTMASK_FRAME    (FocusChangeMask|          \
                     ButtonPressMask|          \
                     ButtonReleaseMask|        \
                     KeyPressMask|             \
                     EnterWindowMask|          \
                     ExposureMask|             \
                     SubstructureRedirectMask)

#endif /* ION_IONCORE_FRAMEP_H */
