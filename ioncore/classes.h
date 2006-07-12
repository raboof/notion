/*
 * ion/ioncore/classes.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_CLASSES_H
#define ION_IONCORE_CLASSES_H

/* Forward declarations of some classes to avoid problems
 * with the header system.
 */

#include <libtu/obj.h>

INTRCLASS(WClientWin);
INTRCLASS(WFrame);
INTRCLASS(WInfoWin);
INTRCLASS(WMPlex);
INTRCLASS(WRegion);
INTRCLASS(WMoveresMode);
INTRCLASS(WRootWin);
INTRCLASS(WScreen);
INTRCLASS(WWindow);
INTRCLASS(WGroup);
INTRCLASS(WGroupCW);

INTRCLASS(WPHolder);
INTRCLASS(WMPlexPHolder);

INTRSTRUCT(WLListNode);
INTRSTRUCT(WStacking);

INTRSTRUCT(WSubmapState);

#endif /* ION_IONCORE_CLASSES_H */
