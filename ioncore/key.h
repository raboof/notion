/*
 * ion/ioncore/key.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_KEY_H
#define ION_IONCORE_KEY_H

#include <X11/keysym.h>

#include "common.h"
#include "clientwin.h"

extern void handle_keypress(XKeyEvent *ev);
extern void clientwin_quote_next(WClientWin *cwin);

#endif /* ION_IONCORE_KEY_H */
