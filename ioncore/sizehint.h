/*
 * wmcore/sizehint.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_SIZEHINT_H
#define WMCORE_SIZEHINT_H

#include "common.h"

void correct_size(int *wp, int *hp, const XSizeHints *hints, bool min);
void get_sizehints(WScreen *scr, Window win, XSizeHints *hints);

#endif /* WMCORE_SIZEHINT_H */
