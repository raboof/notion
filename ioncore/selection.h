/*
 * ion/ioncore/selection.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SELECTION_H
#define ION_IONCORE_SELECTION_H

#include "common.h"

void ioncore_handle_selection_request(XSelectionRequestEvent *ev);
void ioncore_handle_selection(XSelectionEvent *ev);
void ioncore_clear_selection();
void ioncore_set_selection_n(const char *p, int n);
void ioncore_set_selection(const char *p);
void ioncore_request_selection_for(Window win);

#endif /* ION_IONCORE_SELECTION_H */
