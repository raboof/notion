/*
 * ion/ioncore/frame.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FRAME_H
#define ION_IONCORE_FRAME_H

#include <libtu/stringstore.h>
#include <libtu/setparam.h>
#include <libextl/extl.h>

#include "common.h"
#include "window.h"
#include "attach.h"
#include "mplex.h"
#include "gr.h"
#include "rectangle.h"
#include "sizehint.h"
#include "frame-tabs-recalc.h"

#define FRAME_SAVED_VERT  0x0008
#define FRAME_SAVED_HORIZ 0x0010
#define FRAME_SHADED      0x0020
#define FRAME_SHADED_TOGGLE 0x0040
/*#define FRAME_DEST_EMPTY  0x0100*/
#define FRAME_MAXED_VERT  0x0200
#define FRAME_MAXED_HORIZ 0x0400
#define FRAME_MIN_HORIZ   0x0800

/*#define FRAME_SZH_USEMINMAX 0x1000 */
/*#define FRAME_FWD_CWIN_RQGEOM 0x2000 */


typedef enum{
    FRAME_MODE_UNKNOWN,
    FRAME_MODE_TILED,
    FRAME_MODE_TILED_ALT,
    FRAME_MODE_FLOATING,
    FRAME_MODE_TRANSIENT,
    FRAME_MODE_UNKNOWN_ALT,
    FRAME_MODE_FLOATING_ALT,
    FRAME_MODE_TRANSIENT_ALT
} WFrameMode;

typedef enum{
    FRAME_BAR_INSIDE,
    FRAME_BAR_OUTSIDE,
    FRAME_BAR_SHAPED,
    FRAME_BAR_NONE
} WFrameBarMode;
        


DECLCLASS(WFrame){
    WMPlex mplex;
    
    int flags;
    WFrameMode mode;
    int saved_w, saved_h;
    int saved_x, saved_y;
    
    int tab_dragged_idx;
    void *quasiact_source;
    
    GrBrush *brush;
    GrBrush *bar_brush;
    GrStyleSpec baseattr;
    GrTransparency tr_mode;
    GrTextElem *titles;
    int titles_n;
    
    /* Bar stuff */
    WFrameBarMode barmode;
    int bar_w, bar_h;
    /* Parameters to calculate tab sizes. */
    TabCalcParams tabs_params;
};


/* Create/destroy */
extern WFrame *create_frame(WWindow *parent, const WFitParams *fp,
                            WFrameMode mode);
extern bool frame_init(WFrame *frame, WWindow *parent, const WFitParams *fp,
                       WFrameMode mode);
extern void frame_deinit(WFrame *frame);
extern bool frame_rqclose(WFrame *frame);

/* Mode */

extern void frame_set_mode(WFrame *frame, WFrameMode mode);
extern WFrameMode frame_mode(WFrame *frame);

/* Resize and reparent */
extern bool frame_fitrep(WFrame *frame, WWindow *par, const WFitParams *fp);
extern void frame_size_hints(WFrame *frame, WSizeHints *hints_ret);

/* Focus */
extern void frame_activated(WFrame *frame);
extern void frame_inactivated(WFrame *frame);

/* Tabs */
extern int frame_nth_tab_w(WFrame *frame, int n);
extern int frame_nth_tab_x(WFrame *frame, int n);
extern int frame_tab_at_x(WFrame *frame, int x);
extern void frame_update_attr_nth(WFrame *frame, int i);

extern bool frame_set_shaded(WFrame *frame, int sp);
extern bool frame_is_shaded(WFrame *frame);
extern bool frame_set_numbers(WFrame *frame, int sp);
extern bool frame_is_numbers(WFrame *frame);

extern int frame_default_index(WFrame *frame);

/* Misc */
extern void frame_managed_notify(WFrame *frame, WRegion *sub, WRegionNotify how);
extern bool frame_managed_rqdispose(WFrame *frame, WRegion *reg);

extern WPHolder *frame_prepare_manage_transient(WFrame *frame,
                                                const WClientWin *transient,
                                                const WManageParams *param,
                                                int unused);

/* Save/load */
extern ExtlTab frame_get_configuration(WFrame *frame);
extern WRegion *frame_load(WWindow *par, const WFitParams *fp, ExtlTab tab);
extern void frame_do_load(WFrame *frame, ExtlTab tab);

extern WHook *frame_managed_changed_hook;

extern WFrameMode framemode_unalt(WFrameMode mode);

#endif /* ION_IONCORE_FRAME_H */
