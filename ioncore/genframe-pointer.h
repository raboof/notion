/*
 * ion/ioncore/genframe-pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GENFRAME_POINTER_H
#define ION_IONCORE_GENFRAME_POINTER_H

#include "common.h"
#include "pointer.h"
#include "genframe.h"

extern void genframe_p_resize(WGenFrame *genframe);
extern void genframe_p_tabdrag(WGenFrame *genframe);
extern void genframe_p_move(WGenFrame *genframe);
extern void genframe_switch_tab(WGenFrame *genframe);

extern int genframe_press(WGenFrame *genframe, XButtonEvent *ev,
						  WRegion **reg_ret);
extern void genframe_release(WGenFrame *genframe);

DYNFUN bool region_handle_drop(WRegion *reg, int x, int y,
							   WRegion *dropped);
							   
#endif /* ION_IONCORE_GENFRAME_POINTER_H */
