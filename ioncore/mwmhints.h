/*
 * ion/ioncore/mwmhints.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_MWMHINTS_H
#define ION_IONCORE_MWMHINTS_H

#include <X11/Xmd.h>

#include "common.h"


#define MWM_HINTS_FUNCTIONS		0x0001
#define MWM_HINTS_DECORATIONS	0x0002
#define MWM_HINTS_INPUT_MODE	0x0004
#define MWM_HINTS_INPUT_STATUS	0x0008

#define MWM_FUNC_ALL			0x0001
#define MWM_FUNC_RESIZE			0x0002
#define MWM_FUNC_MOVE			0x0004
#define MWM_FUNC_ICONIFY		0x0008
#define MWM_FUNC_MAXIMIZE		0x0010
#define MWM_FUNC_CLOSE			0x0020

#define MWM_DECOR_ALL			0x0001
#define MWM_DECOR_BORDER		0x0002
#define MWM_DECOR_HANDLE		0x0004
#define MWM_DECOR_TITLE			0x0008
#define MWM_DECOR_MENU			0x0010
#define MWM_DECOR_ICONIFY		0x0020
#define MWM_DECOR_MAXIMIZE		0x0040

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 	2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3

INTRSTRUCT(WMwmHints);
	
DECLSTRUCT(WMwmHints){
	CARD32 flags;
	CARD32 functions;
	CARD32 decorations;
	INT32 inputmode;
	CARD32 status;
};

#define MWM_DECOR_NDX 3
#define MWM_N_HINTS	5


extern WMwmHints *get_mwm_hints(Window win);
extern void check_mwm_hints_nodecor(Window win, bool *nodecor);


#endif /* ION_IONCORE_MWMHINTS_H */
