/*
 * ion/mod_query/input.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_INPUT_H
#define ION_MOD_QUERY_INPUT_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>

INTRCLASS(WInput);

DECLCLASS(WInput){
    WWindow win;
    WFitParams last_fp;
    GrBrush *brush;
};

extern bool input_init(WInput *input, WWindow *par, const WFitParams *fp);
extern void input_deinit(WInput *input);

extern void input_fitrep(WInput *input, WWindow *par, const WFitParams *fp);
extern void input_refit(WInput *input);
extern void input_cancel(WInput *input);
extern bool input_rqclose(WInput *input);
extern void input_updategr(WInput *input);

DYNFUN void input_scrollup(WInput *input);
DYNFUN void input_scrolldown(WInput *input);
DYNFUN void input_calc_size(WInput *input, WRectangle *geom);
DYNFUN const char *input_style(WInput *input);

#endif /* ION_MOD_QUERY_INPUT_H */
