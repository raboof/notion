/*
 * ion/ioncore/eventh.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_EVENTH_H
#define ION_IONCORE_EVENTH_H

#include "common.h"

extern void ioncore_mainloop();
extern bool handle_event_default(XEvent *ev);

extern WHooklist *handle_event_alt;

#endif /* ION_IONCORE_EVENTH_H */
