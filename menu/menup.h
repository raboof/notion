/*
 * ion/query/menup.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MENU_MENUP_H
#define ION_MENU_MENUP_H

#include <ioncore/common.h>
#include <ioncore/objp.h>
#include <ioncore/binding.h>
#include "menu.h"

#define MENU_WIN(MENU) ((MENU)->win.win)

#define MENU_MASK (ExposureMask|KeyPressMask| \
				   ButtonPressMask|ButtonReleaseMask| \
				   FocusChangeMask)

extern WBindmap menu_bindmap;

#endif /* ION_MENU_MENUP_H */
