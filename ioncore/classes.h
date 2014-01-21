/*
 * ion/ioncore/classes.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
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
INTRCLASS(WGroupWS);

INTRCLASS(WPHolder);
INTRCLASS(WMPlexPHolder);
INTRCLASS(WFramedPHolder);
INTRCLASS(WGroupPHolder);
INTRCLASS(WGroupedPHolder);

INTRSTRUCT(WStacking);
INTRSTRUCT(WLListNode);
INTRSTRUCT(WStackingIterTmp);

INTRSTRUCT(WSubmapState);

INTRSTRUCT(WRegionAttachData);

INTRSTRUCT(WRQGeomParams);

#endif /* ION_IONCORE_CLASSES_H */
