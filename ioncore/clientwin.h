/*
 * ion/ioncore/clientwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_CLIENTWIN_H
#define ION_IONCORE_CLIENTWIN_H

#include "common.h"
#include "region.h"
#include "hooks.h"
#include "extl.h"
#include "window.h"
#include "rectangle.h"
#include "attach.h"

#define CLIENTWIN_P_WM_DELETE 		 0x00001
#define CLIENTWIN_P_WM_TAKE_FOCUS 	 0x00002
#define CLIENTWIN_PROP_ACROBATIC	 0x00010
#define CLIENTWIN_PROP_MAXSIZE 		 0x00020
#define CLIENTWIN_PROP_ASPECT 		 0x00040
#define CLIENTWIN_PROP_TRANSPARENT 	 0x00080
#define CLIENTWIN_PROP_IGNORE_RSZINC 0x00100
#define CLIENTWIN_PROP_MINSIZE 		 0x00200
#define CLIENTWIN_PROP_IGNORE_CFGRQ  0x00400
#define CLIENTWIN_NEED_CFGNTFY 		 0x01000
#define CLIENTWIN_USE_NET_WM_NAME 	 0x10000
#define CLIENTWIN_TRANSIENTS_AT_TOP	 0x20000

#define FOR_ALL_CLIENTWINS(CWIN) \
    for((CWIN)=ioncore_g.cwin_list; (CWIN)!=NULL; (CWIN)=(CWIN)->g_cwin_next)

#define CLIENTWIN_IS_FULLSCREEN(cwin) \
		(REGION_PARENT_CHK(cwin, WScreen)!=NULL)


typedef struct{
	WWatch last_mgr_watch;
	WRectangle saved_rootrel_geom;
} WClientWinFSInfo;


DECLCLASS(WClientWin){
	WRegion region;
	
	int flags;
	int state;
	int event_mask;
	Window win;
	WRectangle max_geom;
	
	int orig_bw;

	Window transient_for;
	
	WRegion *transient_list;
	
	WClientWin *g_cwin_next, *g_cwin_prev;
	
	Colormap cmap;
	Colormap *cmaps;
	Window *cmapwins;
	int n_cmapwins;

	XSizeHints size_hints;
	
	WClientWinFSInfo fsinfo;
	
	int last_h_rq;
	
	ExtlTab proptab;
};


extern void clientwin_get_protocols(WClientWin *cwin);
extern void clientwin_get_size_hints(WClientWin *cwin);
extern void clientwin_unmapped(WClientWin *cwin);
extern void clientwin_destroyed(WClientWin *cwin);
extern void clientwin_kill(WClientWin *cwin);
extern void clientwin_close(WClientWin *cwin);

extern void clientwin_tfor_changed(WClientWin *cwin);

extern bool clientwin_attach_transient(WClientWin *cwin, WRegion *transient);

extern void clientwin_get_set_name(WClientWin *cwin);

extern void clientwin_handle_configure_request(WClientWin *cwin,
											   XConfigureRequestEvent *ev);

extern void clientwin_broken_app_resize_kludge(WClientWin *cwin);

extern WRegion *clientwin_load(WWindow *par, const WRectangle *geom, 
							   ExtlTab tab);

/* Some standard winprops */

enum{
	TRANSIENT_MODE_NORMAL,
	TRANSIENT_MODE_CURRENT,
	TRANSIENT_MODE_OFF
};

extern bool clientwin_get_switchto(WClientWin *cwin);
extern int clientwin_get_transient_mode(WClientWin *cwin);
extern WClientWin *clientwin_get_transient_for(WClientWin *cwin);

/* Hooks */

extern WHooklist *clientwin_do_manage_alt;

/* Manage */

extern WClientWin *ioncore_manage_clientwin(Window win, bool maprq);

#endif /* ION_IONCORE_CLIENTWIN_H */
