/*
 * ion/query/input.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <ioncore/event.h>
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
    *geom=input->last_fp.g;
    {
        CALL_DYN(input_calc_size, input, (input, geom));
    }
}


const char *input_style(WInput *input)
{
    const char *ret="input";
    CALL_DYN_RET(ret, const char*, input_style, input, (input));
    return ret;
}


/*}}}*/


/*{{{ Resize and draw config update */


static void input_do_refit(WInput *input, WWindow *par)
{
    WRectangle g;
    input_calc_size(input, &g);
    window_do_fitrep(&input->win, par, &g);
}


void input_refit(WInput *input)
{
    input_do_refit(input, NULL);
}


void input_fitrep(WInput *input, WWindow *par, const WFitParams *fp)
{
    input->last_fp=*fp;
    input_do_refit(input, par);
}


void input_updategr(WInput *input)
{
    GrBrush *brush;
    
    brush=gr_get_brush(region_rootwin_of((WRegion*)input),
                       input->win.win,
                       input_style(input));
    
    if(brush==NULL)
        return;
    
    if(input->brush!=NULL)
        grbrush_release(input->brush, input->win.win);
    input->brush=brush;
    input_refit(input);
    
    region_updategr_default((WRegion*)input);
    
    window_draw((WWindow*)input, TRUE);
}


/*}}}*/


/*{{{ Init/deinit */


bool input_init(WInput *input, WWindow *par, const WFitParams *fp)
{
    Window win;

    input->last_fp=*fp;

    if(!window_init((WWindow*)input, par, fp))
        return FALSE;
    
    win=input->win.win;
    
    input->brush=gr_get_brush(region_rootwin_of((WRegion*)par),
                              win, input_style(input));
    
    if(input->brush==NULL)
        goto fail;
    
    input_refit(input);
    XSelectInput(ioncore_g.dpy, input->win.win, IONCORE_EVENTMASK_INPUT);
    region_add_bindmap((WRegion*)input, mod_query_input_bindmap);
    
    return TRUE;
    
fail:
    window_deinit((WWindow*)input);
    return FALSE;
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
    ioncore_defer_destroy((Obj*)input);
}


bool input_rqclose(WInput *input)
{
    input_cancel(input);
    return TRUE;
}


/*}}}*/


/*{{{ Focus  */


static void input_inactivated(WInput *input)
{
    window_draw((WWindow*)input, FALSE);
}


static void input_activated(WInput *input)
{
    window_draw((WWindow*)input, FALSE);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab input_dynfuntab[]={
    {(DynFun*)region_fitrep, (DynFun*)input_fitrep},
    {region_updategr, input_updategr},
    {(DynFun*)region_rqclose, (DynFun*)input_rqclose},
    {region_activated, input_activated},
    {region_inactivated, input_inactivated},
    END_DYNFUNTAB
};


IMPLCLASS(WInput, WWindow, input_deinit, input_dynfuntab);

    
/*}}}*/
