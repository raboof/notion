/*
 * ion/query/input.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_IONCORE_INPUTMODE_H
#define ION_QUERY_IONCORE_INPUTMODE_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>

INTRCLASS(WInput);

DECLCLASS(WInput){
	WWindow win;
	WRectangle max_geom;
	GrBrush *brush;
};


extern bool input_init(WInput *input, WWindow *par, const WRectangle *geom);
extern void input_deinit(WInput *input);

extern void input_fit(WInput *input, const WRectangle *geom);
extern void input_refit(WInput *input);
extern void input_cancel(WInput *input);
extern void input_draw_config_updated(WInput *input);

DYNFUN void input_scrollup(WInput *input);
DYNFUN void input_scrolldown(WInput *input);
DYNFUN void input_calc_size(WInput *input, WRectangle *geom);
DYNFUN const char *input_style(WInput *input);

#endif /* ION_QUERY_IONCORE_INPUTMODE_H */
