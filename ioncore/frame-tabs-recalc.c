/*
 * notion/ioncore/frame-tabs-recalc.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 * Copyright (c) Tomas Ebenlendr 2011.
 *
 * See the included file LICENSE for details.
 */
#include <string.h>

#include "common.h"
#include "names.h"
#include "frame.h"
#include "framep.h"
#include "frame-draw.h"
#include "frame-tabs-recalc.h"


/*{{{ Common functions */


/*
 * Temporarily store actual title widths (without truncation) in
 * frame->titles[*].iw, when calculating tabs widths and bar width. After the
 * algorithm returns it has to set frame->titles[*].iw to the proper values.
 *
 * This function is generic for all algorithms.
 */
static void get_titles_text_width(WFrame *frame)
{
    int i=0;
    WLListIterTmp itmp;
    WRegion *sub;
    const char *displayname;

    /* Assume frame->bar_brush != NULL.
       Assume FRAME_MCOUNT(frame) > 0 */

    FRAME_MX_FOR_ALL(sub, frame, itmp){
        displayname=region_displayname(sub);
        if(displayname==NULL)
            frame->titles[i].iw=0;
        else
            frame->titles[i].iw=grbrush_get_text_width(frame->bar_brush,
                                            displayname, strlen(displayname));
        i++;
    }
}

/*}}}*/

/*{{{ Equal tab sizes algorithm
 *    This gives tabs equal sizes (up to 1 pixel).
 */
static void frame_tabs_width_calculate_equal(WFrame *frame)
{
    WRectangle bg;
    GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
    int i,m=frame->titles_n;
    uint w;

    frame_bar_geom(frame, &bg);
    grbrush_get_border_widths(frame->bar_brush, &bdw);

    /* Remove borders */
    w=bg.w-bdw.left-bdw.right-(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)*(m-1);

    if(w<=0){
        for(i=0;i<m;i++)
            frame->titles[i].iw=0;
    }else{
        for(i=0;i<m;i++)
            /* Get i:th tab's portion of free area */
            frame->titles[i].iw=((i+1)*w)/m-(i*w)/m;
    }
}

/*
   Equal tab sizes algorithm.
   Calculate size of the bar of a shaped (i.e., floating) frame.
*/
static int frame_shaped_recalc_bar_size_equal(WFrame *frame)
{
    int bar_w, textw=0, tmaxw=frame->tabs_params.tab_min_w, tmp=0;
    GrBorderWidths bdw;
    uint bdtotal;
    int i, m;

    m=FRAME_MCOUNT(frame);
    bar_w=frame->tabs_params.bar_max_width_q*REGION_GEOM(frame).w;

    if(m>0){
        grbrush_get_border_widths(frame->bar_brush, &bdw);
        bdtotal=((m-1)*(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)
                 +bdw.right+bdw.left);

        get_titles_text_width(frame);
        for(i=0;i<m;i++)
            if(frame->titles[i].iw>tmaxw)
                tmaxw=frame->titles[i].iw;

        if(bar_w<frame->tabs_params.tab_min_w &&
           REGION_GEOM(frame).w>frame->tabs_params.tab_min_w)
            bar_w=frame->tabs_params.tab_min_w;

        tmp=bar_w-bdtotal-m*tmaxw;

        if(tmp>0){
            /* No label truncation needed, good. See how much can be padded. */
            tmp/=m*2;
            if(tmp>frame->tabs_params.requested_pad)
                tmp=frame->tabs_params.requested_pad;
            bar_w=(tmaxw+tmp*2)*m+bdtotal;
        }else{
            /* Some labels must be truncated */
        }
    }else{
        if(bar_w<frame->tabs_params.tab_min_w) bar_w=frame->tabs_params.tab_min_w;
    }

    return bar_w;
}

static bool tab_sizes_equal(WFrame * frame, bool complete)
{
    int bar_w;

    if(frame->barmode==FRAME_BAR_SHAPED){
        bar_w=frame_shaped_recalc_bar_size_equal(frame);
        if((frame->bar_w!=bar_w)){
            frame->bar_w=bar_w;
            complete=TRUE;
        }
    }
    frame_tabs_width_calculate_equal(frame);
    return complete;
}





/*}}}*/

DECLFUNPTRMAP(TabCalcPtr);

static StringTabCalcPtrMap frame_tabs_width_algorithms[] = {
    { "equal", tab_sizes_equal },
    END_STRINGPTRMAP
};

static TabCalcPtr default_frame_tabs_width_algorithm=tab_sizes_equal;



static void param_init(TabCalcParams *pars)
{
    pars->tab_min_w=100;
    pars->bar_max_width_q=0.95;
    pars->requested_pad=10;
    pars->alg=default_frame_tabs_width_algorithm;
}

void frame_tabs_calc_brushes_updated(WFrame *frame)
{
    char *str;
    TabCalcPtr alg;
    TabCalcParams *pars=&(frame->tabs_params);

    param_init(pars);
    
    if(grbrush_get_extra(frame->brush, "floatframe_tab_min_w",
                         'i', &(pars->tab_min_w)) ||
       grbrush_get_extra(frame->brush, "frame_tab_min_w",
                         'i', &(pars->tab_min_w))){
        if(pars->tab_min_w<=0)
            pars->tab_min_w=1;
    }
    
    if(grbrush_get_extra(frame->brush, "floatframe_bar_max_w_q", 
                         'd', &(pars->bar_max_width_q))){
        if(pars->bar_max_width_q<=0.0 || pars->bar_max_width_q>1.0)
            pars->bar_max_width_q=1.0;
    }

    if(grbrush_get_extra(frame->brush, "frame_tab_padding", 
                         'i', &(pars->requested_pad))){
        if(pars->requested_pad<=0)
            pars->requested_pad=1;
    }

    if(grbrush_get_extra(frame->brush, "frame_tab_width_alg", 
                         's', &str)){
        alg=STRINGFUNPTRMAP_VALUE(TabCalcPtr,
                                  frame_tabs_width_algorithms,
                                  str, NULL);

        if(alg!=NULL)
            frame->tabs_params.alg=alg;
    }
}

void frame_tabs_width_recalc_init(WFrame *frame)
{
    param_init(&(frame->tabs_params));
}
