/*
 * wmcore/input.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/window.h>
#include <wmcore/global.h>
#include "inputp.h"


static DynFunTab input_dynfuntab[]={
	{fit_region,	fit_input},
	END_DYNFUNTAB
};


IMPLOBJ(WInput, WWindow, deinit_input, input_dynfuntab)


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
	dinfo->gc=grdata->gc;
	dinfo->grdata=grdata;
	dinfo->colors=&(grdata->input_colors);
	dinfo->border=&(grdata->input_border);
	dinfo->font=INPUT_FONT(grdata);
}


/*}}}*/


/*{{{ Resize */


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


/*}}}*/


/*{{{ Init/deinit */


bool init_input(WInput *input, WScreen *scr, WWinGeomParams params)
{
	Window win;
	
	input->max_geom=params.geom;
	
	/* Kludge -- TODO */
	input->win.region.screen=scr;
	
	input_calc_size(input, &params.geom);
	
	params.win_x+=params.geom.x-input->max_geom.x;
	params.win_y+=params.geom.y-input->max_geom.y;

	win=create_simple_window_bg(scr, params, scr->grdata.input_colors.bg);

	if(!init_window((WWindow*)input, scr, win, params.geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}
	
	XSelectInput(wglobal.dpy, input->win.win, INPUT_MASK);

	input->win.bindmap=query_bindmap;
	
	/*link_thing((WThing*)parent, (WThing*)input);
	map_window((WWindow*)input);*/

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

