/*
 * wmcore/funtabs.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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
#include "tags.h"
#include "key.h"
#include "names.h"
#include "draw.h"
#include "commandsq.h"


WBindmap wmcore_screen_bindmap=BINDMAP_INIT;
WBindmap wmcore_clientwin_bindmap=BINDMAP_INIT;


WFunclist wmcore_screen_funclist=INIT_FUNCLIST;

static WFunction wmcore_screen_funtab[]={
	FN_SCREEN(s,   "exec",					wm_exec),
	FN_GLOBAL_VOID("goto_previous",			goto_previous),
	FN_GLOBAL_VOID("restart",				wm_restart),
	FN_GLOBAL_VOID("exit",					wm_exit),
	FN_GLOBAL(s,   "restart_other",			wm_restart_other),
	FN_GLOBAL(l,   "goto_nth_viewport",		goto_viewport_id),
	FN_GLOBAL_VOID("goto_next_viewport",	goto_next_viewport),
	FN_GLOBAL_VOID("goto_prev_viewport",	goto_prev_viewport),
	FN_GLOBAL_VOID("clear_tags",			clear_tags),
	FN_GLOBAL(s,   "goto_named_region",		goto_named_region),
	FN_GLOBAL_VOID("reread_draw_config",	reread_draw_config),

	FN_GLOBAL(l,   "switch_ws_nth",			switch_ws_nth),
	FN_GLOBAL_VOID("switch_ws_next",		switch_ws_next),
	FN_GLOBAL_VOID("switch_ws_prev",		switch_ws_prev),

	FN_GLOBAL_VOID("commands_at_leaf",		commands_at_leaf),
	END_FUNTAB
};


WFunclist wmcore_clientwin_funclist=INIT_FUNCLIST;

static WFunction wmcore_clientwin_funtab[]={
	FN_VOID(generic, WClientWin,	"clientwin_close",	close_clientwin),
	FN_VOID(generic, WClientWin,	"clientwin_kill",	kill_clientwin),
	FN_VOID(generic, WClientWin,	"clientwin_toggle_fullscreen",	clientwin_toggle_fullscreen),
	FN_VOID(generic, WClientWin,	"clientwin_broken_app_resize_kludge", clientwin_broken_app_resize_kludge),
    FN_VOID(generic, WClientWin,    "clientwin_quote_next", quote_next),
	/* Common functions */
	FN_VOID(generic, WClientWin,	"close",			close_clientwin),
	END_FUNTAB
};


void wmcore_init_funclists()
{
	assert(add_to_funclist(&wmcore_screen_funclist,
						   wmcore_screen_funtab));
	assert(add_to_funclist(&wmcore_clientwin_funclist,
						   wmcore_clientwin_funtab));
}
