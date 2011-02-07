/*
 * notion/ioncore/frame-tabs-recalc.h
 *
 * Copyright (c) Tomas Ebenlendr 2011.
 *
 * See the included file LICENSE for details.
 */

#ifndef NOTION_IONCORE_FRAME_TABS_RECALC_H
#define NOTION_IONCORE_FRAME_TABS_RECALC_H

#include "libtu/map.h"
#include "libtu/obj.h"

/* Compute inner widths of tabs and width of shaped bar.
 * There will be more algorithms to do this, thus here
 * is prototype of the called function.
 *
 * Requirements:
 *
 * If complete is set and frame->barmode==FRAME_BAR_SHAPED
 * then frame->bar_w has to be updated.
 * return TRUE if bar_w changed (i.e, new value != old value)
 * return the value of 'complete' on normal run.
 * frame_set_shape or frame_clear_shape is called when TRUE is returned.
 *
 * The function is called only if
 *   frame->bar_brush != NULL && frame->titles != NULL
 */
typedef bool (*TabCalcPtr)(WFrame *frame, bool complete);

/* Gets the identifier of the algorithm. */
const char *frame_get_tabs_sizes_algorithm(WFrame *frame);

/* Sets the algorithm based on the identifier.
 * Returns -1 on failure, 1 on successfull change, 0 on success but no change
 * If 1 is returned, frame_bar_recalc and frame_bar_draw should be called.
 */
int frame_do_set_tabs_sizes_algorithm(WFrame *frame, const char *algstr);

INTRSTRUCT(TabCalcParams);
DECLSTRUCT(TabCalcParams){
    TabCalcPtr alg;
    /* Maximum size of shaped bar. */
    double bar_max_width_q;
    /* Requested size of the tab. */
    int tab_min_w;
    /* Requested empty space to be added before and after text. */
    int requested_pad;
};

void frame_tabs_calc_brushes_updated(WFrame *frame);
void frame_tabs_width_recalc_init(WFrame *frame);
#endif /* NOTION_IONCORE_FRAME_TABS_RECALC_H */
