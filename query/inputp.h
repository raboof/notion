/*
 * ion/query/inputp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_INPUTP_H
#define ION_QUERY_INPUTP_H

#include <ioncore/common.h>
#include <ioncore/objp.h>
#include <ioncore/binding.h>
#include "input.h"

#define INPUT_MASK (ExposureMask|KeyPressMask| \
					ButtonPressMask|ButtonReleaseMask| \
					FocusChangeMask)

typedef void WInputCalcSizeFn(WInput*, WRectangle*);
typedef void WInputScrollFn(WInput*);
typedef void WInputDrawFn(WInput*, bool complete);

extern WBindmap query_bindmap;
extern WBindmap query_wedln_bindmap;

#endif /* ION_QUERY_INPUTP_H */
