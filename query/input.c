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
EXTL_EXPORT_MEMBER
void input_scrollup(WInput *input)
{
	CALL_DYN(input_scrollup, input, (input));
}


/*EXTL_DOC
 * Scroll input \var{input} text contents down.
 */
EXTL_EXPORT_MEMBER
void input_scrolldown(WInput *input)
{
	CALL_DYN(input_scrolldown, input, (input));
}


void input_calc_size(WInput *input, WRectangle *geom)
{
	CALL_DYN(input_calc_size, input, (input, geom));
}


const char *input_style(WInput *input)
{
	const char *ret="input";
	CALL_DYN_RET(ret, const char*, input_style, input, (input));
	return ret;
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
	GrBrush *brush;
	
	brush=gr_get_brush(ROOTWIN_OF(input), input->win.win,
					   input_style(input));
	
	if(brush==NULL)
		return;
	
	if(input->brush!=NULL)
		grbrush_release(input->brush, input->win.win);
	input->brush=brush;
	input_refit(input);
	
	region_default_draw_config_updated((WRegion*)input);
	
	window_draw((WWindow*)input, TRUE);
}


/*}}}*/


/*{{{ Init/deinit */


bool input_init(WInput *input, WWindow *par, WRectangle geom)
{
	Window win;

	input->max_geom=geom;
	
	win=create_simple_window(ROOTWIN_OF(par), par->win, geom);
	
	input->brush=gr_get_brush(ROOTWIN_OF(par), win, input_style(input));
	if(input->brush==NULL){
		warn("Could not get a brush for WInput.");
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}

	if(!window_init((WWindow*)input, par, win, geom)){
		grbrush_release(input->brush, win);
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}

	input_refit(input);
	
	XSelectInput(wglobal.dpy, input->win.win, INPUT_MASK);
	region_add_bindmap((WRegion*)input, &query_bindmap);
	
	return TRUE;
}


void input_deinit(WInput *input)
{
	if(input->brush!=NULL)
		grbrush_release(input->brush, input->win.win);
	
	window_deinit((WWindow*)input);
}


/*EXTL_DOC
 * Close input not calling any possible finish handlers.
 */
EXTL_EXPORT_MEMBER
void input_cancel(WInput *input)
{
	defer_destroy((WObj*)input);
}


/*EXTL_DOC
 * Same as \fnref{WInput.cancel}.
 */
EXTL_EXPORT_MEMBER
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
