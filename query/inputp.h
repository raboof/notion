/*
 * query/inputp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_INPUTP_H
#define QUERY_INPUTP_H

#include <wmcore/common.h>
#include <wmcore/thingp.h>
#include <wmcore/drawp.h>
#include <wmcore/binding.h>
#include "input.h"

#define INPUT_BORDER_SIZE(GRDATA) \
	(BORDER_TL_TOTAL(&(GRDATA->input_border))+ \
	 BORDER_BR_TOTAL(&(GRDATA->input_border)))

#define INPUT_MASK (ExposureMask|KeyPressMask| \
					ButtonPressMask|FocusChangeMask)

#define INPUT_FONT(GRDATA) ((GRDATA)->font)

typedef void WInputCalcSizeFn(WInput*, WRectangle*);
typedef void WInputScrollFn(WInput*);
typedef void WInputDrawFn(WInput*, bool complete);

extern void setup_input_dinfo(WInput *input, DrawInfo *dinfo);

extern WBindmap *query_bindmap;
extern WBindmap *query_edln_bindmap;
extern WFunclist query_input_funclist;
extern WFunclist query_edln_funclist;

#endif /* QUERY_INPUTP_H */
