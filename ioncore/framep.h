/*
 * ion/ioncore/framep.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
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
