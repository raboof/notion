/*
 * ion/ioncore/key.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_KEY_H
#define ION_IONCORE_KEY_H

#include <X11/keysym.h>

#include "common.h"
#include "clientwin.h"

extern void handle_keypress(XKeyEvent *ev);
extern void clientwin_quote_next(WClientWin *cwin);

#endif /* ION_IONCORE_KEY_H */
