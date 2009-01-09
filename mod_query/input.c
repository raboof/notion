/*
 * ion/mod_query/input.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <libmainloop/defer.h>
#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/global.h>
#include <ioncore/regbind.h>
#include <ioncore/event.h>
#include <ioncore/names.h>
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


bool input_fitrep(WInput *input, WWindow *par, const WFitParams *fp)
{
    if(par!=NULL && !region_same_rootwin((WRegion*)input, (WRegion*)par))
        return FALSE;
    input->last_fp=*fp;
    input_do_refit(input, par);
    
    return TRUE;
}


void input_updategr(WInput *input)
{
    GrBrush *brush;
    
    brush=gr_get_brush(input->win.win,
                       region_rootwin_of((WRegion*)input),
                       input_style(input));
    
    if(brush==NULL)
        return;
    
    if(input->brush!=NULL)
        grbrush_release(input->brush);
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
    
    input->brush=gr_get_brush(win, region_rootwin_of((WRegion*)par),
                              input_style(input));
    
    if(input->brush==NULL)
        goto fail;
    
    input_refit(input);
    window_select_input(&(input->win), IONCORE_EVENTMASK_NORMAL);

    region_add_bindmap((WRegion*)input, mod_query_input_bindmap);
    
    region_register((WRegion*)input);
    
    return TRUE;
    
fail:
    window_deinit((WWindow*)input);
    return FALSE;
}


void input_deinit(WInput *input)
{
    if(input->brush!=NULL)
        grbrush_release(input->brush);
    
    window_deinit((WWindow*)input);
}


/*EXTL_DOC
 * Close input not calling any possible finish handlers.
 */
EXTL_EXPORT_MEMBER
void input_cancel(WInput *input)
{
    region_defer_rqdispose((WRegion*)input);
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


/*{{{{ Misc */


void mod_query_get_minimum_extents(GrBrush *brush, bool with_spacing, 
                                   int *w, int *h)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    int spc;
    
    grbrush_get_border_widths(brush, &bdw);
    grbrush_get_font_extents(brush, &fnte);
    
    spc=(with_spacing ? bdw.spacing : 0);
    
    *h=(fnte.max_height+bdw.top+bdw.bottom+spc);
    *w=(bdw.left+bdw.right+spc);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab input_dynfuntab[]={
    {(DynFun*)region_fitrep, (DynFun*)input_fitrep},
    {region_updategr, input_updategr},
    {region_activated, input_activated},
    {region_inactivated, input_inactivated},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WInput, WWindow, input_deinit, input_dynfuntab);

    
/*}}}*/
