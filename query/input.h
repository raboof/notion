/*
 * query/input.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_INPUT_H
#define QUERY_INPUT_H

#include <wmcore/common.h>
#include <wmcore/window.h>

INTROBJ(WInput)

DECLOBJ(WInput){
	WWindow win;
	WRectangle max_geom;
};


extern bool init_input(WInput *input, WRegion *par, WRectangle geom);
extern void deinit_input(WInput *input);

extern void fit_input(WInput *input, WRectangle geom);
extern void input_refit(WInput *input);
extern void input_cancel(WInput *input);
extern void input_draw_config_updated(WInput *input);

DYNFUN void input_scrollup(WInput *input);
DYNFUN void input_scrolldown(WInput *input);
DYNFUN void input_calc_size(WInput *input, WRectangle *geom);

#endif /* QUERY_INPUT_H */
