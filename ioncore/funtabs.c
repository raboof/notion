/*
 * wmcore/funtabs.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * 
 * See the included file LICENSE for details.
 */

#include "screen.h"
#include "clientwin.h"
#include "exec.h"
#include "focus.h"
#include "function.h"
#include "functionp.h"
#include "funtabs.h"
#include "viewport.h"


WBindmap wmcore_screen_bindmap=BINDMAP_INIT;
WBindmap wmcore_viewport_bindmap=BINDMAP_INIT;
WBindmap wmcore_clientwin_bindmap=BINDMAP_INIT;


WFunclist wmcore_screen_funclist=INIT_FUNCLIST;

static WFunction wmcore_screen_funtab[]={
	FN_SCREEN(s,				"exec",					wm_exec),
	FN_GLOBAL_VOID(				"goto_previous",		goto_previous),
	FN_GLOBAL_VOID(				"restart",				wm_restart),
	FN_GLOBAL_VOID(				"exit",					wm_exit),
	FN_GLOBAL(s,				"restart_other",		wm_restart_other),
	FN_GLOBAL(l,				"goto_nth_viewport",	goto_viewport_id),
	FN_GLOBAL_VOID(				"goto_next_viewport",	goto_next_viewport),
	FN_GLOBAL_VOID(				"goto_prev_viewport",	goto_prev_viewport),

	END_FUNTAB
};


WFunclist wmcore_viewport_funclist=INIT_FUNCLIST;

static WFunction wmcore_viewport_funtab[]={
	FN(l, 	generic, WViewport,	"switch_ws_nth",	viewport_switch_nth),
	FN_VOID(generic, WViewport,	"switch_ws_next",	viewport_switch_next),     
	FN_VOID(generic, WViewport,	"switch_ws_prev",	viewport_switch_prev),     

	END_FUNTAB
};


WFunclist wmcore_clientwin_funclist=INIT_FUNCLIST;

static WFunction wmcore_clientwin_funtab[]={
	FN_VOID(generic, WClientWin,	"close",		close_clientwin),
	FN_VOID(generic, WClientWin,	"kill",			kill_clientwin),
	FN(b,	generic, WClientWin,	"enter_fullscreen",	clientwin_enter_fullscreen),
	
	END_FUNTAB
};


void wmcore_init_funclists()
{
	assert(add_to_funclist(&wmcore_screen_funclist,
						   wmcore_screen_funtab));
	assert(add_to_funclist(&wmcore_clientwin_funclist,
						   wmcore_clientwin_funtab));
	assert(add_to_funclist(&wmcore_viewport_funclist,
						   wmcore_viewport_funtab));
}
