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

#include <limits.h>
#include <ioncore/common.h>
#include <ioncore/pointer.h>
#include <ioncore/grab.h>
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
WMenu *menu_menu(WMPlex *mplex, ExtlFn handler, ExtlTab tab, bool big_mode)
{
	WMenuCreateParams fnp;

	fnp.handler=handler;
	fnp.tab=tab;
	fnp.pmenu_mode=FALSE;
	fnp.submenu_mode=FALSE;
	fnp.big_mode=big_mode;
	
	return (WMenu*)mplex_add_input(mplex,
								   (WRegionAttachHandler*)create_menu,
								   (void*)&fnp);
}


EXTL_EXPORT
WMenu *menu_pmenu(WWindow *where, ExtlFn handler, ExtlTab tab)
{
	WScreen *scr;
	WMenuCreateParams fnp;
	XEvent *ev=p_current_event();
	WMenu *menu;
	WRectangle geom;
	
	if(ev==NULL || ev->type!=ButtonPress)
		return NULL;

	scr=region_screen_of((WRegion*)where);

	if(scr==NULL)
		return NULL;
	
	fnp.handler=handler;
	fnp.tab=tab;
	fnp.pmenu_mode=TRUE;
	fnp.big_mode=FALSE;
	fnp.submenu_mode=FALSE;
	fnp.ref_x=ev->xbutton.x_root-REGION_GEOM(scr).x;
	fnp.ref_y=ev->xbutton.y_root-REGION_GEOM(scr).y;
	
	geom.x=0;
	geom.y=0;
	geom.w=REGION_GEOM(where).w;
	geom.h=REGION_GEOM(where).h;
	
	menu=create_menu((WWindow*)scr, &geom, &fnp);
	
	if(menu==NULL)
		return NULL;
	
	if(!p_set_drag_handlers((WRegion*)menu,
							NULL,
							(WMotionHandler*)menu_motion,
							(WButtonHandler*)menu_release,
							NULL, 
							(GrabKilledHandler*)menu_cancel)){
		destroy_obj((WObj*)menu);
		return NULL;
	}
	
	region_map((WRegion*)menu);
	
	return menu;
}

