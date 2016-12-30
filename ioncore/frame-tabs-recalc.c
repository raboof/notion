/*
 * notion/ioncore/frame-tabs-recalc.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 * Copyright (c) Tomas Ebenlendr 2011.
 *
 * See the included file LICENSE for details.
 */
#include <string.h>
#include <stdlib.h> /* qsort */

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
    int bar_w, tmaxw=frame->tabs_params.tab_min_w, tmp=0;
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

/*{{{ Proportional/Elastic tab sizes algorithms
 *    This gives tabs sizes proportional to the contained text.
 */

/* Failsafes to 'equal' algorithm if there are more tabs.
 * The result will be same in most cases of many tabs anyway.
 */
#define PROPOR_MAX_TABS 30

static int intcompare (const void *a, const void *b)
{
    return *(int *)a-*(int *)b;
}

static bool tab_sizes_propor_elastic(WFrame * frame, bool complete, bool proportional)
{
    int bar_w, titles_total=0;
    int titles_padded_total1=0, titles_padded_total2=0;
    WRectangle bg;
    GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
    int i, m=frame->titles_n;
    int iw, min_w=0, max_w, w;
    int sorted_sizes[PROPOR_MAX_TABS];
    uint bdtotal;

    if (m>PROPOR_MAX_TABS)
        return tab_sizes_equal(frame, complete);


    frame_bar_geom(frame, &bg);
    grbrush_get_border_widths(frame->bar_brush, &bdw);

    if (frame->barmode==FRAME_BAR_SHAPED) {
        bar_w=frame->tabs_params.bar_max_width_q*REGION_GEOM(frame).w;
    } else {
        bar_w=bg.w;
        frame->bar_w=bar_w;
    }

    get_titles_text_width(frame);

    /* Calculate thresholds. */
    for (i=0;i<m;i++) {
        iw=sorted_sizes[i]=frame->titles[i].iw;
        titles_total+=
            (iw<frame->tabs_params.propor_tab_min_w ?
             frame->tabs_params.propor_tab_min_w : iw);
        titles_padded_total1+=
            (iw < frame->tabs_params.propor_tab_min_w-2*frame->tabs_params.requested_pad ?
             frame->tabs_params.propor_tab_min_w : iw+2*frame->tabs_params.requested_pad);
        titles_padded_total2+=
            (iw < frame->tabs_params.tab_min_w-2*frame->tabs_params.requested_pad ?
             frame->tabs_params.tab_min_w : iw+2*frame->tabs_params.requested_pad);
    }
    bdtotal=((m-1)*(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)
            +bdw.right+bdw.left);
    w=bar_w-bdtotal;

    /* Do different things based on thresholds. */
    if (m*frame->tabs_params.propor_tab_min_w>=w) {
        /*Equal sizes (tiny tabs). Base width is zero.*/
        for(i=0;i<m;i++)
            frame->titles[i].iw=0;
    } else if (titles_total>=w) {
        /*Truncate long tabs.*/
        qsort(sorted_sizes, m, sizeof(int), intcompare);
        min_w=frame->tabs_params.propor_tab_min_w;
        max_w=sorted_sizes[m-1];titles_total=0;
        for(i=0;i<m;i++){
            if (sorted_sizes[i]>frame->tabs_params.propor_tab_min_w)
                break;
            titles_total+=frame->tabs_params.propor_tab_min_w;
        }
        for(;i<m;i++){
            if (titles_total+sorted_sizes[i]*(m-i)>=w) {
                max_w=(w-titles_total)/(m-i);
                break;
            }
            titles_total+=sorted_sizes[i];
        }
        for(i=0;i<m;i++)
            if (frame->titles[i].iw>max_w)
                frame->titles[i].iw=max_w;
    } else if (titles_padded_total1>=w) {
        /*Just a little padding.
         *Pad equally, no tab shorter than tabs_params.propor_tab_min_w-1.
         */
        max_w=frame->tabs_params.propor_tab_min_w;
equal_pad:
        qsort(sorted_sizes, m, sizeof(int), intcompare);
        titles_total=m*max_w;
        i=m-1;min_w=0;
        while((i>=0) &&
              (sorted_sizes[i]>=max_w)){
            titles_total+=sorted_sizes[i];
            i--;
        }
        while(i>=0){
            /*Test padding by max_w-sorted_sizes[i].*/
            if(titles_total-(m-i-1)*sorted_sizes[i]>=w){
                min_w=(titles_total-w)/(m-i-1);
                break;
            }
            titles_total+=sorted_sizes[i];
            i--;
        }
        /* After expanding to min_w: equal padding will extend short tabs to
         * required size (+- 1pixel).
         */

    } else if (titles_padded_total2>=w) {
        /* Expand as many short tabs as possible to equal size,
         * they will be shorter than tabs_params.tab_min_w anyway.
         * Long tabs should be padded by 2*tabs_params.requested_pad.
         */
equal_tab:
        qsort(sorted_sizes, m, sizeof(int), intcompare);
        min_w=0;titles_total=2*m*frame->tabs_params.requested_pad;
        for(i=m-1;i>=0;i--){
            if (sorted_sizes[i]*(i+1)+titles_total<=w){
                min_w=(w-titles_total)/(i+1);
                break;
            }
            titles_total+=sorted_sizes[i];
        }
        /* After expanding to min_w: it should remain
         * 2*m*tabs_params.requested_pad +- m
         */
    } else if (frame->barmode==FRAME_BAR_SHAPED) {
        /* Shorter bar. */
        w=titles_padded_total2;
        bar_w=w+bdtotal;
        min_w=frame->tabs_params.tab_min_w-2*frame->tabs_params.requested_pad;
        /* After expanding to min_w: it should remain
         * 2*m*tabs_params.requested_pad exactly
         */
    } else {
        if(proportional){
            /* Pad equally, no tab shorter than tabs_params.tab_min_w-1. */
            max_w=frame->tabs_params.tab_min_w;
            goto equal_pad;
        }else{
        /* Expand as many short tabs as possible to equal size,
         * Long tabs should be padded by 2*tabs_params.requested_pad.
         */
            goto equal_tab;
        }
    }
    if (min_w>0) {
        for(i=0;i<m;i++)
            if (frame->titles[i].iw<min_w)
                frame->titles[i].iw=min_w;
    }


    /* Calculate remaining space */
    titles_total=0;
    for(i=0;i<m;i++)
        titles_total+=frame->titles[i].iw;
    w-=titles_total;
    /* Distribute remaining space equally up to 1 pixel */
    for(i=0;i<m;i++)
        frame->titles[i].iw+=((i+1)*w)/m-(i*w)/m;

    if((frame->bar_w!=bar_w)){
        frame->bar_w=bar_w;
        return TRUE;
    }
    return complete;
}


static bool tab_sizes_proportional(WFrame * frame, bool complete)
{
    return tab_sizes_propor_elastic(frame, complete, TRUE);
}

static bool tab_sizes_elastic(WFrame * frame, bool complete)
{
    return tab_sizes_propor_elastic(frame, complete, FALSE);
}



/*}}}*/

DECLFUNPTRMAP(TabCalcPtr);

static StringTabCalcPtrMap frame_tabs_width_algorithms[] = {
    { "equal", tab_sizes_equal },
    { "elastic", tab_sizes_elastic },
    { "proportional", tab_sizes_proportional },
    END_STRINGPTRMAP
};

static TabCalcPtr default_frame_tabs_width_algorithm=tab_sizes_equal;



static void param_init(TabCalcParams *pars)
{
    pars->tab_min_w=100;
    pars->propor_tab_min_w=50;
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

    if(grbrush_get_extra(frame->brush, "frame_propor_tab_min_w",
                         'i', &(pars->propor_tab_min_w))){
        if(pars->propor_tab_min_w<=0)
            pars->propor_tab_min_w=1;
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
