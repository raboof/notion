/*
 * wmcore/close.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_CLOSE_H
#define WMCORE_CLOSE_H

#include "common.h"
#include "region.h"
#include "clientwin.h"

DYNFUN void region_request_close(WRegion *reg);
extern void close_propagate(WRegion *reg);
extern void close_sub(WRegion *reg);
extern void close_sub_propagate(WRegion *reg);

#endif /* WMCORE_CLOSE_H */

