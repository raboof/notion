/*
 * ion/query/input.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/global.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>
#include "inputp.h"


/*{{{ Dynfuns */


/*EXTL_DOC
 * Scroll input \var{input} text contents up.
 */
EXTL_EXPORT
void input_scrollup(WInput *input)
{
	CALL_DYN(input_scrollup, input, (input));
}


/*EXTL_DOC
 * Scroll input \var{input} text contents down.
 */
EXTL_EXPORT
void input_scrolldown(WInput *input)
{
	CALL_DYN(input_scrolldown, input, (input));
}


void input_calc_size(WInput *input, WRectangle *geom)
{
	CALL_DYN(input_calc_size, input, (input, geom));
}


/*}}}*/


/*{{{ Draw */


void setup_input_dinfo(WInput *input, DrawInfo *dinfo)
{
	WGRData *grdata=GRDATA_OF(input);
	
	dinfo->win=input->win.win;
	dinfo->draw=input->win.draw;

	dinfo->gc=grdata->gc;
	dinfo->grdata=grdata;
	dinfo->colors=&(grdata->input_colors);
	dinfo->border=&(grdata->input_border);
	dinfo->font=INPUT_FONT(grdata);
}


/*}}}*/


/*{{{ Resize and draw config update */


void input_refit(WInput *input)
{
	WRectangle geom=input->max_geom;
	input_calc_size(input, &geom);
	window_fit(&input->win, geom);
}


void input_fit(WInput *input, WRectangle geom)
{
	input->max_geom=geom;
	input_refit(input);
}


void input_draw_config_updated(WInput *input)
{
	XSetWindowBackground(wglobal.dpy, input->win.win,
						 COLOR_PIXEL(GRDATA_OF(input)->input_colors.bg));

	input_refit(input);
	region_default_draw_config_updated((WRegion*)input);
	draw_window((WWindow*)input, TRUE);
}


/*}}}*/


/*{{{ Init/deinit */


bool input_init(WInput *input, WWindow *par, WRectangle geom)
{
	WGRData *grdata;
	Window win;

	input->max_geom=geom;
	
	grdata=GRDATA_OF(par);
	
	win=create_simple_window_bg(grdata, par->win, geom,
								grdata->input_colors.bg);
	
	if(!window_init((WWindow*)input, par, win, geom))
		return FALSE;

	input_refit(input);
	
	XSelectInput(wglobal.dpy, input->win.win, INPUT_MASK);
	region_add_bindmap((WRegion*)input, &query_bindmap);
	
	return TRUE;
}


void input_deinit(WInput *input)
{
	window_deinit((WWindow*)input);
}


/*EXTL_DOC
 * Close input not calling any possible finish handlers.
 */
EXTL_EXPORT
void input_cancel(WInput *input)
{
	defer_destroy((WObj*)input);
}


/*EXTL_DOC
 * Same as \fnref{input_cancel}.
 */
EXTL_EXPORT
void input_close(WInput *input)
{
	input_cancel(input);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab input_dynfuntab[]={
	{region_fit, input_fit},
	{region_draw_config_updated, input_draw_config_updated},
	{region_close, input_close},
	END_DYNFUNTAB
};


IMPLOBJ(WInput, WWindow, input_deinit, input_dynfuntab);

	
/*}}}*/
