/*
 * ion/query/input.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/global.h>
#include <ioncore/regbind.h>
#include "inputp.h"


static DynFunTab input_dynfuntab[]={
	{fit_region, fit_input},
	{region_draw_config_updated, input_draw_config_updated},
	END_DYNFUNTAB
};


IMPLOBJ(WInput, WWindow, deinit_input, input_dynfuntab, &query_input_funclist)


/*{{{ Dynfuns */


void input_scrollup(WInput *input)
{
	CALL_DYN(input_scrollup, input, (input));
}


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
	WScreen *scr=SCREEN_OF(input);
	WGRData *grdata=&(scr->grdata);
	
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
	fit_window(&input->win, geom);
}


void fit_input(WInput *input, WRectangle geom)
{
	input->max_geom=geom;
	input_refit(input);
}


void input_draw_config_updated(WInput *input)
{
	XSetWindowBackground(wglobal.dpy, input->win.win,
						 COLOR_PIXEL(GRDATA_OF(input)->input_colors.bg));

	input_refit(input);
	default_draw_config_updated((WRegion*)input);
	draw_window((WWindow*)input, TRUE);
}


/*}}}*/


/*{{{ Init/deinit */


bool init_input(WInput *input, WWindow *par, WRectangle geom)
{
	WScreen *scr;
	Window win;

	input->max_geom=geom;
	scr=SCREEN_OF(par);
	win=create_simple_window_bg(scr, par->win, geom,
								scr->grdata.input_colors.bg);
	
	if(!init_window((WWindow*)input, par, win, geom))
		return FALSE;

	input_refit(input);
	
	XSelectInput(wglobal.dpy, input->win.win, INPUT_MASK);
	region_add_bindmap((WRegion*)input, &query_bindmap);
	
	return TRUE;
}


void deinit_input(WInput *input)
{
	deinit_window((WWindow*)input);
}


void input_cancel(WInput *input)
{
	destroy_thing((WThing*)input);
}


/*}}}*/

