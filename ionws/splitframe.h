/*
 * ion/ionws/splitframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONWS_SPLITFRAME_H
#define ION_IONWS_SPLITFRAME_H

#include <ioncore/common.h>
#include "split.h"
#include "ionframe.h"
#include "ionws.h"

extern WIonFrame *find_frame_at(WIonWS *ws, int x, int y);

extern void split(WRegion *reg, const char *str);
extern void split_empty(WRegion *reg, const char *str);
extern void split_top(WIonWS *ws, const char *str);
extern void ionframe_close(WIonFrame *frame);
extern void ionframe_close_if_empty(WIonFrame *frame);

#endif /* ION_IONWS_SPLITFRAME_H */

