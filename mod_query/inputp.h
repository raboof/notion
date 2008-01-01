/*
 * ion/mod_query/inputp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
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
