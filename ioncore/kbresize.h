/*
 * ion/ioncore/kbresize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_KBRESIZE_H
#define ION_IONCORE_KBRESIZE_H

#include "common.h"
#include "frame.h"
#include "resize.h"
#include <libextl/extl.h>

extern WMoveresMode *region_begin_kbresize(WRegion *reg);

extern void ioncore_set_moveres_accel(ExtlTab tab);
extern void ioncore_get_moveres_accel(ExtlTab tab);

extern void moveresmode_finish(WMoveresMode *mode);
extern void moveresmode_cancel(WMoveresMode *mode);
extern void moveresmode_move(WMoveresMode *mode, 
                             int horizmul, int vertmul);
extern void moveresmode_resize(WMoveresMode *mode, 
                               int left, int right, int top, int bottom);
extern ExtlTab moveresmode_geom(WMoveresMode *mode);
extern ExtlTab moveresmode_rqgeom_extl(WMoveresMode *mode, ExtlTab g);
extern void moveresmode_accel(WMoveresMode *mode, 
                              int *wu, int *hu, int accel_mode);

#endif /* ION_IONCORE_KBRESIZE_H */
