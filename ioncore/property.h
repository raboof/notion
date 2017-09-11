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
extern bool xwindow_get_state_property(Window win, unsigned int *state);
extern void xwindow_set_state_property(Window win, unsigned int  state);
extern char **xwindow_get_text_property(Window win, Atom a, int *nret);

/**
 * Set a text property. The type of the property (STRING, COMPOUND_STRING,
 * UTF8_STRING or even any custom multibyte encoding) is determined
 * automatically based on the string and the current locale.
 *
 * This may be used for any property of type 'TEXT' (not 'STRING') in
 * http://tronche.com/gui/x/icccm/sec-2.html#s-2.6.2
 *
 * @param p null-terminated list of input strings, in the current locale
 *          encoding
 */
extern void xwindow_set_text_property(Window win, Atom a,
                                      const char **p, int n);
extern bool xwindow_get_cardinal_property(Window win, Atom a, CARD32 *vret);
extern bool xwindow_get_atom_property(Window win, Atom a, Atom *vret);

/**
 * Set a property as UTF8_STRING. To read UTF8_STRING properties, the normal
 * xwindow_get_text_property can be used.
 *
 * @param p null-terminated list of input strings, in the current locale
 *          encoding
 */
extern void xwindow_set_utf8_property(Window win, Atom a,
                                      const char **p, int n);


#endif /* ION_IONCORE_PROPERTY_H */

