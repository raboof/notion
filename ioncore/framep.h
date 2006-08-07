/*
 * ion/ioncore/framep.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAMEP_H
#define ION_IONCORE_FRAMEP_H

#include "frame.h"
#include "llist.h"

#define FRAME_MCOUNT(FRAME) mplex_mx_count(&(FRAME)->mplex)
#define FRAME_CURRENT(FRAME) mplex_mx_current(&(FRAME)->mplex)

#define FRAME_MX_FOR_ALL(REG, FRAME, TMP) \
    FOR_ALL_REGIONS_ON_LLIST(REG, (FRAME)->mplex.mx_list, TMP)

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
