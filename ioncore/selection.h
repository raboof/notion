/*
 * wmcore/selection.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_SELECTION_H
#define WMCORE_SELECTION_H

#include "common.h"

void send_selection(XSelectionRequestEvent *ev);
void receive_selection(XSelectionEvent *ev);
void clear_selection();
void set_selection(const char *p, int n);
void request_selection(Window win);

#endif /* WMCORE_SELECTION_H */
