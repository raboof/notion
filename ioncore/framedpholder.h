/*
 * ion/ioncore/framedpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FRAMEDPHOLDER_H
#define ION_IONCORE_FRAMEDPHOLDER_H

#include "common.h"
#include "pholder.h"
#include "attach.h"
#include "frame.h"


#define FRAMEDPARAM_INIT {0, 0, {0, 0, 0, 0}, FRAME_MODE_FLOATING /*, NULL*/}

INTRSTRUCT(WFramedParam);

DECLSTRUCT(WFramedParam){
    uint inner_geom_gravity_set:1;
    int gravity;
    WRectangle inner_geom;
    WFrameMode mode;
    /*WRegionSimpleCreateFn *mkframe;*/
};


DECLCLASS(WFramedPHolder){
    WPHolder ph;
    WPHolder *cont;
    WFramedParam param;
};


extern WFramedPHolder *create_framedpholder(WPHolder *cont,
                                            const WFramedParam *param);

extern bool framedpholder_init(WFramedPHolder *ph, WPHolder *cont,
                               const WFramedParam *param);

extern void framedpholder_deinit(WFramedPHolder *ph);

extern bool framedpholder_do_goto(WFramedPHolder *ph);

extern bool framedpholder_stale(WFramedPHolder *ph);

extern WRegion *framedpholder_do_target(WFramedPHolder *ph);

extern WRegion *framedpholder_do_attach(WFramedPHolder *ph, int flags,
                                        WRegionAttachData *data);

extern WRegion *region_attach_framed(WRegion *reg, WFramedParam *param,
                                     WRegionAttachFn *fn, void *fn_param,
                                     WRegionAttachData *data);

extern void frame_adjust_to_initial(WFrame *frame, const WFitParams *fp, 
                                    const WFramedParam *param, WRegion *reg);

#endif /* ION_IONCORE_FRAMEDPHOLDER_H */
