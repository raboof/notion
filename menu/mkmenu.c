/*
 * ion/menu/mkmenu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/extl.h>
#include "menu.h"
#include "mkmenu.h"

/*EXTL_DOC
 * Display a menu inside multiplexer. The \var{handler} parameter
 * is a function that gets the selected menu entry as argument and
 * should call it with proper parameters; use \fnref{make_menu_fn}
 * to create functions that pass a proper \var{handler}. The table
 * \var{tab} is a list of menu entries of the form
 * \code{\{name = ???, fn = ???\}}; the convenience function
 * \fnref{menuentry} maybe also used to create these entries.
 */
EXTL_EXPORT
WMenu *menu_menu(WMPlex *mplex, ExtlFn handler, ExtlTab tab)
{
	WRectangle geom;
	WMenuCreateParams fnp;

	fnp.handler=handler;
	fnp.tab=tab;
	
	return (WMenu*)mplex_add_input(mplex,
								   (WRegionAttachHandler*)create_menu,
								   (void*)&fnp);
}
