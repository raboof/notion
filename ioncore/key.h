/*
 * wmcore/key.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_KEY_H
#define WMCORE_KEY_H

#include <X11/keysym.h>

#include "common.h"
#include "clientwin.h"

extern void handle_keypress(XKeyEvent *ev);
extern void quote_next(WClientWin *cwin);

#endif /* WMCORE_KEY_H */
