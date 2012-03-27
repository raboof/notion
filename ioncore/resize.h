/*
 * ion/ioncore/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_RESIZE_H
#define ION_IONCORE_RESIZE_H

#include "common.h"
#include "frame.h"
#include "infowin.h"
#include "rectangle.h"
#include "sizehint.h"


/* To make it easier for region_managed_rqgeom handlers, the geom
 * parameter contain a complete requested geometry for the region that
 * wants its geometry changed. The REGION_WEAK_* flags are used to
 * indicate that the respective geometry value has not been changed or
 * that the requestor doesn't really care what the result is. In any case,
 * managers are free to give the managed object whatever geometry it wishes.
 */
#define REGION_RQGEOM_WEAK_X    0x0001
#define REGION_RQGEOM_WEAK_Y    0x0002
#define REGION_RQGEOM_WEAK_W    0x0004
#define REGION_RQGEOM_WEAK_H    0x0008
#define REGION_RQGEOM_WEAK_ALL  0x000f
#define REGION_RQGEOM_TRYONLY   0x0010
#define REGION_RQGEOM_ABSOLUTE  0x0020
#define REGION_RQGEOM_GRAVITY   0x0040

#define REGION_RQGEOM_NORMAL    0
#define REGION_RQGEOM_VERT_ONLY (REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_W)
#define REGION_RQGEOM_HORIZ_ONLY (REGION_RQGEOM_WEAK_Y|REGION_RQGEOM_WEAK_H)
#define REGION_RQGEOM_H_ONLY    (REGION_RQGEOM_VERT_ONLY|REGION_RQGEOM_WEAK_Y)
#define REGION_RQGEOM_W_ONLY    (REGION_RQGEOM_HORIZ_ONLY|REGION_RQGEOM_WEAK_X)

#define REGION_ORIENTATION_NONE 0
#define REGION_ORIENTATION_HORIZONTAL 1
#define REGION_ORIENTATION_VERTICAL 2


#define RQGEOMPARAMS_INIT {{0, 0, 0, 0}, 0, 0}


DECLSTRUCT(WRQGeomParams){
    WRectangle geom;
    int flags;
    int gravity;
};


typedef void WDrawRubberbandFn(WRootWin *rw, const WRectangle *geom);


DECLCLASS(WMoveresMode){
    Obj obj;
    WSizeHints hints;
    int dx1, dx2, dy1, dy2;
    WRectangle origgeom;
    WRectangle geom;
    WRegion *reg;
    WDrawRubberbandFn *rubfn;
    int parent_rx, parent_ry;
    enum {MOVERES_SIZE, MOVERES_POS} mode;
    bool resize_cumulative;
    bool snap_enabled;
    WRectangle snapgeom;
    int rqflags;
    WInfoWin *infowin;
};


extern WMoveresMode *region_begin_resize(WRegion *reg, 
                                         WDrawRubberbandFn *rubfn,
                                         bool cumulative);

extern WMoveresMode *region_begin_move(WRegion *reg,
                                       WDrawRubberbandFn *rubfn,
                                       bool cumulative);

/* dx1/dx2/dy1/dy2: left/right/top/bottom difference to previous values. 
 * left/top negative, bottom/right positive increases size.
 */
extern void moveresmode_delta_resize(WMoveresMode *mode, 
                                     int dx1, int dx2, int dy1, int dy2,
                                     WRectangle *rret);
extern void moveresmode_delta_move(WMoveresMode *mode, 
                                   int dx, int dy, WRectangle *rret);
extern void moveresmode_rqgeom(WMoveresMode *mode, WRQGeomParams *rq, 
                               WRectangle *rret);
extern bool moveresmode_do_end(WMoveresMode *mode, bool apply);
extern WRegion *moveresmode_target(WMoveresMode *mode);

extern WMoveresMode *moveres_mode(WRegion *reg);



/* Note: even if REGION_RQGEOM_(X|Y|W|H) are not all specified, the
 * geom parameter should contain a proper geometry!
 */
DYNFUN void region_managed_rqgeom(WRegion *reg, WRegion *sub, 
                                  const WRQGeomParams *rqgp,
                                  WRectangle *geomret);
DYNFUN void region_managed_rqgeom_absolute(WRegion *reg, WRegion *sub, 
                                           const WRQGeomParams *rqgp,
                                           WRectangle *geomret);

extern void region_rqgeom(WRegion *reg, const WRQGeomParams *rqgp,
                          WRectangle *geomret);

/* Implementation for regions that do not allow subregions to resize
 * themselves; default is to give the size the region wants.
 */
extern void region_managed_rqgeom_unallow(WRegion *reg, WRegion *sub,
                                          const WRQGeomParams *rq,
                                          WRectangle *geomret);
/* default */
extern void region_managed_rqgeom_allow(WRegion *reg, WRegion *sub,
                                        const WRQGeomParams *rq,
                                        WRectangle *geomret);

extern void region_managed_rqgeom_absolute_default(WRegion *reg, WRegion *sub,
                                                   const WRQGeomParams *rq,
                                                   WRectangle *geomret);

DYNFUN void region_managed_save(WRegion *reg, WRegion *sub, int dir);
DYNFUN void region_managed_restore(WRegion *reg, WRegion *sub, int dir);
DYNFUN bool region_managed_verify(WRegion *reg, WRegion *sub, int dir);


DYNFUN void region_size_hints(WRegion *reg, WSizeHints *hints_ret);
DYNFUN int region_orientation(WRegion *reg);

extern void region_size_hints_correct(WRegion *reg, 
                                      int *wp, int *hp, bool min);

extern uint region_min_h(WRegion *reg);
extern uint region_min_w(WRegion *reg);

extern void frame_maximize_vert_2(WFrame *frame);
extern void frame_maximize_vert(WFrame *frame);
extern void frame_maximize_horiz_2(WFrame *frame);
extern void frame_maximize_horiz(WFrame *frame);

extern void region_convert_root_geom(WRegion *reg, WRectangle *geom);

extern void region_absolute_geom_to_parent(WRegion *reg, 
                                           const WRectangle *rgeom,
                                           WRectangle *pgeom);

extern void rqgeomparams_from_table(WRQGeomParams *rq, 
                                    const WRectangle *origg, ExtlTab g);

#endif /* ION_IONCORE_RESIZE_H */
