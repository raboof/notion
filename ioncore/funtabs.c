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


WBindmap wmcore_screen_bindmap=BINDMAP_INIT;
WBindmap wmcore_clientwin_bindmap=BINDMAP_INIT;


WFunclist wmcore_screen_funclist=INIT_FUNCLIST;

static WFunction wmcore_screen_funtab[]={
	FN_SCREEN(l,				"switch_ws_nth",	screen_switch_nth),
	FN_SCREEN_VOID(				"switch_ws_next",	screen_switch_next),     
	FN_SCREEN_VOID(				"switch_ws_prev",	screen_switch_prev),     
	FN_SCREEN(s,				"exec",				wm_exec),

	/* global */
	FN_GLOBAL_VOID(				"goto_previous",	goto_previous),
	FN_GLOBAL_VOID(				"restart",			wm_restart),
	FN_GLOBAL_VOID(				"exit",				wm_exit),
	FN_GLOBAL(s,				"restart_other",	wm_restart_other),
	/*FN_GLOBAL(s,				"switch_ws_name",	switch_workspace_name),*/
	FN_GLOBAL(ll,				"switch_ws_nth2",	screen_switch_nth2),

	END_FUNTAB
};


WFunclist wmcore_clientwin_funclist=INIT_FUNCLIST;

static WFunction wmcore_clientwin_funtab[]={
	FN_VOID(generic, WClientWin,	"close",		close_clientwin),
	FN_VOID(generic, WClientWin,	"kill",			kill_clientwin),
	FN_VOID(generic, WClientWin,	"enter_fullscreen",	clientwin_enter_fullscreen),
	
	END_FUNTAB
};


void wmcore_init_funclists()
{
	assert(add_to_funclist(&wmcore_screen_funclist,
						   wmcore_screen_funtab));
	assert(add_to_funclist(&wmcore_clientwin_funclist,
						   wmcore_clientwin_funtab));
}
