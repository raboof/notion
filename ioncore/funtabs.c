/*
 * ion/ioncore/funtabs.c
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
#include "genframe.h"
#include "genframe-pointer.h"
#include "resize.h"


WBindmap ioncore_screen_bindmap=BINDMAP_INIT;


WFunclist ioncore_screen_funclist=INIT_FUNCLIST;

static WFunction ioncore_screen_funtab[]={
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


WFunclist ioncore_clientwin_funclist=INIT_FUNCLIST;

static WFunction ioncore_clientwin_funtab[]={
	FN_VOID(generic, WClientWin,	"clientwin_close",	close_clientwin),
	FN_VOID(generic, WClientWin,	"clientwin_kill",	kill_clientwin),
	FN_VOID(generic, WClientWin,	"clientwin_toggle_fullscreen",	clientwin_toggle_fullscreen),
	FN_VOID(generic, WClientWin,	"clientwin_broken_app_resize_kludge", clientwin_broken_app_resize_kludge),
    FN_VOID(generic, WClientWin,    "clientwin_quote_next", quote_next),
	/* Common functions */
	FN_VOID(generic, WClientWin,	"close",			close_clientwin),
	END_FUNTAB
};


WFunclist ioncore_genframe_funclist=INIT_FUNCLIST;

static WFunction ioncore_genframe_funtab[]={
	FN(l,	generic, WGenFrame,	"genframe_switch_nth",		genframe_switch_nth),
	FN_VOID(generic, WGenFrame, "genframe_switch_next",		genframe_switch_next),
	FN_VOID(generic, WGenFrame, "genframe_switch_prev",		genframe_switch_prev),
	FN_VOID(generic, WGenFrame,	"genframe_attach_tagged",	genframe_attach_tagged),
	FN_VOID(generic, WGenFrame,	"genframe_toggle_sub_tag",	genframe_toggle_sub_tag),

	FN_VOID(generic, WGenFrame,	"genframe_maximize_vert", 	genframe_maximize_vert),
	FN_VOID(generic, WGenFrame,	"genframe_maximize_horiz", 	genframe_maximize_horiz),
	
	FN(l,	generic, WRegion,	"genframe_set_width",		set_width),
	FN(l,	generic, WRegion,	"genframe_set_height",		set_height),
	FN(d,	generic, WRegion,	"genframe_set_widthq",		set_widthq),
	FN(d,	generic, WRegion,	"genframe_set_heightq",		set_heightq),

	/* mouse move/resize and tab drag */
	FN_VOID(generic, WGenFrame,	"genframe_p_resize",		genframe_p_resize_setup),
	FN_VOID(generic, WGenFrame,	"genframe_p_tabdrag", 		genframe_p_tabdrag_setup),
	FN_VOID(generic, WGenFrame,	"genframe_p_switch_tab",	genframe_switch_tab),

	FN_VOID(generic, WGenFrame,	"genframe_move_current_tab_left", genframe_move_current_tab_left),
	FN_VOID(generic, WGenFrame,	"genframe_move_current_tab_right", genframe_move_current_tab_right),

	END_FUNTAB
};

/*
WFunclist ioncore_moveres_funclist=INIT_FUNCLIST;

static WFunction ioncore_moveres_funtab[]={
	FN_VOID(generic, WWindow,	"end_resize",		end_resize),
	FN_VOID(generic, WWindow,	"cancel_resize",	cancel_resize),
	FN_VOID(generic, WWindow,	"grow",				grow),
	FN_VOID(generic, WWindow,	"shrink",			shrink),
	
	END_FUNTAB
};
*/

void ioncore_init_funclists()
{
	assert(add_to_funclist(&ioncore_screen_funclist,
						   ioncore_screen_funtab));
	assert(add_to_funclist(&ioncore_clientwin_funclist,
						   ioncore_clientwin_funtab));
	assert(add_to_funclist(&ioncore_genframe_funclist,
						   ioncore_genframe_funtab));
	/*
	assert(add_to_funclist(&ioncore_moveres_funclist,
						   ioncore_moveres_funtab));
	 */
}
