/*
 * ion/ioncore/eventh.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_EVENTH_H
#define ION_IONCORE_EVENTH_H

#include "common.h"

extern void ioncore_mainloop();
extern bool handle_event_default(XEvent *ev);

extern WHooklist *handle_event_alt;

#endif /* ION_IONCORE_EVENTH_H */
