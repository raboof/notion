/*
 * ion/ionws/splitframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_SPLITFRAME_H
#define ION_IONWS_SPLITFRAME_H

#include <ioncore/common.h>
#include "split.h"
#include "ionframe.h"
#include "ionws.h"

extern WIonFrame *find_frame_at(WIonWS *ws, int x, int y);

extern WIonFrame *ionframe_split(WIonFrame *frame, const char *dirstr);
extern WIonFrame *ionframe_split_empty(WIonFrame *frame, const char *dirstr);
extern WIonFrame *ionws_newframe(WIonWS *ws, const char *dirstr);
extern void ionframe_close(WIonFrame *frame);
extern void ionframe_close_if_empty(WIonFrame *frame);

#endif /* ION_IONWS_SPLITFRAME_H */

