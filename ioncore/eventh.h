/*
 * wmcore/eventh.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_EVENTH_H
#define WMCORE_EVENTH_H

#include "common.h"

extern void mainloop();
extern void handle_event(XEvent *ev);

#endif /* WMCORE_EVENTH_H */
