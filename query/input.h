/*
 * ion/query/input.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_INPUT_H
#define ION_QUERY_INPUT_H

#include <ioncore/common.h>
#include <ioncore/window.h>

INTROBJ(WInput);

DECLOBJ(WInput){
	WWindow win;
	WRectangle max_geom;
};


extern bool input_init(WInput *input, WWindow *par, WRectangle geom);
extern void input_deinit(WInput *input);

extern void input_fit(WInput *input, WRectangle geom);
extern void input_refit(WInput *input);
extern void input_cancel(WInput *input);
extern void input_draw_config_updated(WInput *input);

DYNFUN void input_scrollup(WInput *input);
DYNFUN void input_scrolldown(WInput *input);
DYNFUN void input_calc_size(WInput *input, WRectangle *geom);

#endif /* ION_QUERY_INPUT_H */
