/*
 * ion/ioncore/grab.h
 *
 * Copyright (c) Lukas Schroeder 2002,
 *               Tuomo Valkonen 2003-2007.
 *
 * See the included file LICENSE for details.
 *
 * Alternatively, you may apply the Clarified Artistic License to this file,
 * since Lukas' contributions were originally under that.
 */

#ifndef ION_IONCORE_GRAB_H
#define ION_IONCORE_GRAB_H

#include "global.h" /* for InputHandler and InputHandlerContext */
#include "common.h"
#include "region.h"

// Grab flags:
#define GRAB_KEYBOARD 0x0100
#define GRAB_POINTER  0x0200

#define GRAB_DEFAULT_FLAGS (GRAB_KEYBOARD|GRAB_POINTER)

// GrabHandler result flags:
#define GRAB_COMPLETE 0x0001
#define GRAB_FORWARD  0x0002

/* GrabHandler:
   the default_keyboard_handler now simplifies access to subsequent keypresses
   when you establish a grab using grab_establish().

   A GrabHandler returns a bitmap with actions to take after the grab:
   - GRAB_COMPLETE remove the grab
   - GRAB_FORWARD forward the grabbed event to the event handler
   kept active and you get more grabbed events passed to your handler.
 */
typedef int GrabHandler(WRegion *reg, XEvent *ev);
typedef void GrabKilledHandler(WRegion *reg);

/**
 * @param eventmask mask of X11 events *not* to grab. Usually 0.
 * @param flags grab flags, usually GRAB_DEFAULT_FLAGS.
 */
extern void ioncore_grab_establish(WRegion *reg, GrabHandler *func,
                                   GrabKilledHandler *kh,
                                   long eventmask, int flags);
extern void ioncore_grab_remove(GrabHandler *func);
extern void ioncore_grab_holder_remove(WRegion *holder);
extern WRegion *ioncore_grab_get_holder();
extern WRegion *ioncore_grab_get_my_holder(GrabHandler *func);
extern bool ioncore_grab_held();
/** if the current grab, if any, also holds the mouse pointer */
extern bool ioncore_pointer_grab_held();
extern void ioncore_change_grab_cursor(int cursor);
extern void ioncore_grab_confine_to(Window confine_to);

/**
 * @returns TRUE if the event was consumed, FALSE if it must still be processed
 */
extern bool ioncore_handle_grabs(XEvent *ev);

#endif /* ION_IONCORE_GRAB_H */

