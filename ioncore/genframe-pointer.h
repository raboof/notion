/*
 * ion/ioncore/genframe-pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GENFRAME_POINTER_H
#define ION_IONCORE_GENFRAME_POINTER_H

#include "common.h"
#include "pointer.h"
#include "genframe.h"

extern void genframe_p_resize_setup(WGenFrame *genframe);
extern void genframe_p_tabdrag_setup(WGenFrame *genframe);
extern void genframe_p_move_setup(WGenFrame *genframe);
extern void genframe_switch_tab(WGenFrame *genframe);

extern int genframe_press(WGenFrame *genframe, XButtonEvent *ev,
						  WThing **thing_ret);
extern void genframe_release(WGenFrame *genframe);

DYNFUN bool region_handle_drop(WRegion *reg, int x, int y,
							   WRegion *dropped);
							   
#endif /* ION_IONCORE_GENFRAME_POINTER_H */
