/*
 * ion/ioncore/return.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_RETURN_H
#define ION_IONCORE_RETURN_H

#include "region.h"
#include "pholder.h"

/* Note: 'ph' is under the control of this 'return' module after succesfully
 * (true return value) having been passed to this function and must no
 * longer be destroyed by other code unless removed with unset_get.
 */
extern bool region_do_set_return(WRegion *reg, WPHolder *ph);

/*extern WPHolder *region_set_return(WRegion *reg);*/

extern WPHolder *region_get_return(WRegion *reg);

extern void region_unset_return(WRegion *reg);

extern WPHolder *region_unset_get_return(WRegion *reg);

extern WPHolder *region_make_return_pholder(WRegion *reg);

#endif /* ION_IONCORE_RETURN_H */
