/*
 * ion/ioncore/clientwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_CLIENTWIN_H
#define ION_IONCORE_CLIENTWIN_H

#include "common.h"
#include "region.h"
#include "hooks.h"
#include "viewport.h"

INTROBJ(WClientWin)

#define CWIN_P_WM_DELETE 		0x0001
#define CWIN_P_WM_TAKE_FOCUS 	0x0002
#define CWIN_PROP_ACROBATIC		0x0010
#define CWIN_PROP_MAXSIZE 		0x0020
#define CWIN_PROP_ASPECT 		0x0040
#define CWIN_PROP_TRANSPARENT 	0x0080
#define CWIN_NEED_CFGNTFY 		0x1000

#define MANAGE_RESPECT_POS		0x0001
#define MANAGE_INITIAL			0x0002

#define FOR_ALL_CLIENTWINS(CWIN) \
    for((CWIN)=wglobal.cwin_list; (CWIN)!=NULL; (CWIN)=(CWIN)->g_cwin_next)
	
	
DECLOBJ(WClientWin){
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
	char *name;
	
	WWatch last_mgr_watch;
};


extern void get_protocols(WClientWin *cwin);
extern void get_clientwin_size_hints(WClientWin *cwin);
extern WClientWin* manage_clientwin(Window win, int mflags);
extern void clientwin_unmapped(WClientWin *cwin);
extern void clientwin_destroyed(WClientWin *cwin);
extern void kill_clientwin(WClientWin *cwin);
extern void close_clientwin(WClientWin *cwin);
extern WClientWin *find_clientwin(Window win);
extern void set_clientwin_name(WClientWin *cwin, char *p);
extern WClientWin *lookup_clientwin(const char *name);
extern int complete_clientwin(char *nam, char ***cp_ret, char **beg,
							  void *unused);

extern void clientwin_set_target_id(WClientWin *cwin, int id);
extern void clientwin_clear_target_id(WClientWin *cwin);

extern void clientwin_handle_configure_request(WClientWin *cwin,
											   XConfigureRequestEvent *ev);

extern bool clientwin_fullscreen_vp(WClientWin *cwin, WViewport *vp,
									bool switchto);
extern bool clientwin_toggle_fullscreen(WClientWin *cwin);
extern bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto);
extern bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto);

extern void clientwin_broken_app_resize_kludge(WClientWin *cwin);

extern void clientwin_get_set_name(WClientWin *cwin);

/* Hooks */

extern WHooklist *add_clientwin_alt;


#endif /* ION_IONCORE_CLIENTWIN_H */
