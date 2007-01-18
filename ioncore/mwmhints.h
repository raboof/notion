/*
 * ion/ioncore/mwmhints.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MWMHINTS_H
#define ION_IONCORE_MWMHINTS_H

#include <X11/Xmd.h>

#include "common.h"


#define MWM_HINTS_FUNCTIONS     0x0001
#define MWM_HINTS_DECORATIONS   0x0002
#define MWM_HINTS_IONCORE_INPUTMODE_MODE    0x0004
#define MWM_HINTS_IONCORE_INPUTMODE_STATUS  0x0008

#define MWM_FUNC_ALL            0x0001
#define MWM_FUNC_RESIZE         0x0002
#define MWM_FUNC_MOVE           0x0004
#define MWM_FUNC_ICONIFY        0x0008
#define MWM_FUNC_MAXIMIZE       0x0010
#define MWM_FUNC_CLOSE          0x0020

#define MWM_DECOR_ALL           0x0001
#define MWM_DECOR_BORDER        0x0002
#define MWM_DECOR_HANDLE        0x0004
#define MWM_DECOR_TITLE         0x0008
#define MWM_DECOR_MENU          0x0010
#define MWM_DECOR_ICONIFY       0x0020
#define MWM_DECOR_MAXIMIZE      0x0040

#define MWM_IONCORE_INPUTMODE_MODELESS 0
#define MWM_IONCORE_INPUTMODE_PRIMARY_APPLICATION_MODAL 1
#define MWM_IONCORE_INPUTMODE_SYSTEM_MODAL 2
#define MWM_IONCORE_INPUTMODE_FULL_APPLICATION_MODAL 3

INTRSTRUCT(WMwmHints);
    
DECLSTRUCT(WMwmHints){
    CARD32 flags;
    CARD32 functions;
    CARD32 decorations;
    INT32 inputmode;
    CARD32 status;
};

#define MWM_DECOR_NDX 3
#define MWM_N_HINTS 5


extern WMwmHints *xwindow_get_mwmhints(Window win);
extern void xwindow_check_mwmhints_nodecor(Window win, bool *nodecor);


#endif /* ION_IONCORE_MWMHINTS_H */
