/*
 * ion/mod_menu/mkmenu.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_MENU_MKMENU_H
#define ION_MOD_MENU_MKMENU_H

#include <ioncore/common.h>
#include <libextl/extl.h>
#include "menu.h"

extern WMenu *mod_menu_do_menu(WMPlex *mplex, ExtlFn handler, ExtlTab tab,
                               ExtlTab param);
extern WMenu *mod_menu_do_pmenu(WWindow *where, ExtlFn handler, ExtlTab tab);

#endif /* ION_MOD_MENU_MKMENU_H */
