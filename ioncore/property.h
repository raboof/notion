/*
 * ion/ioncore/property.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_PROPERTY_H
#define ION_IONCORE_PROPERTY_H

#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include "common.h"

extern ulong xwindow_get_property(Window win, Atom atom, Atom type,
                                  ulong n32expected, bool more, uchar **p);
extern char *xwindow_get_string_property(Window win, Atom a, int *nret);
extern void xwindow_set_string_property(Window win, Atom a, const char *value);
extern bool xwindow_get_integer_property(Window win, Atom a, int *vret);
extern void xwindow_set_integer_property(Window win, Atom a, int value);
extern bool xwindow_get_state_property(Window win, int *state);
extern void xwindow_set_state_property(Window win, int state);
extern char **xwindow_get_text_property(Window win, Atom a, int *nret);
extern void xwindow_set_text_property(Window win, Atom a, 
                                      const char **p, int n);
extern bool xwindow_get_cardinal_property(Window win, Atom a, CARD32 *vret);
#ifdef X_HAVE_UTF8_STRING                                                                                                                                                                                                         
extern void xwindow_set_utf8_property(Window win, Atom a,                                                                                                                                                                         
                                      const char **p, int n);                                                                                                                                                                     
#endif                                                                                                                                                                                                                            
 

#endif /* ION_IONCORE_PROPERTY_H */

