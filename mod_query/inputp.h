/*
 * ion/mod_query/inputp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_INPUTP_H
#define ION_MOD_QUERY_INPUTP_H

#include <ioncore/common.h>
#include <libtu/objp.h>
#include <ioncore/binding.h>
#include <ioncore/rectangle.h>
#include "input.h"

typedef void WInputCalcSizeFn(WInput*, WRectangle*);
typedef void WInputScrollFn(WInput*);
typedef void WInputDrawFn(WInput*, bool complete);

extern WBindmap *mod_query_input_bindmap;
extern WBindmap *mod_query_wedln_bindmap;

#endif /* ION_MOD_QUERY_INPUTP_H */
