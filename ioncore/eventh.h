/*
 * wmcore/eventh.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_EVENTH_H
#define WMCORE_EVENTH_H

#include "common.h"

extern void mainloop();
extern bool handle_event_default(XEvent *ev);

extern WHooklist *handle_event_alt;

#endif /* WMCORE_EVENTH_H */
