/*
 * ion/funtab.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/exec.h>
#include <wmcore/focus.h>
#include <wmcore/functionp.h>
#include <wmcore/clientwin.h>
#include <wmcore/screen.h>
#include <wmcore/close.h>
#include "frame.h"
#include "frame-pointer.h"
#include "workspace.h"
#include "resize.h"
#include "funtab.h"
#include "split.h"
#include "splitframe.h"


/*{{{ Wrapper functions */


static WRegion *get_csub(WRegion *reg)
{
	if(WTHING_IS(reg, WFrame))
		return ((WFrame*)reg)->current_sub;
	else
		return reg;
}


void callhnd_cclient_void(WThing *thing, WFunction *func,
						  int n, const Token *args)
{
	typedef void Func(WClientWin*);
	WClientWin *cwin=(WClientWin*)get_csub((WRegion*)thing);
	
	if(cwin!=NULL && WTHING_IS(cwin, WClientWin))
		((Func*)func->fn)(cwin);
}


/*}}}*/


/*{{{ ion_main_funtab */


WFunclist ion_main_funclist=INIT_FUNCLIST;


static WFunction ion_main_funtab[]={
	/* frame */
	FN(l,	generic, WFrame,	"switch_nth",		frame_switch_nth),
	FN_VOID(generic, WFrame, 	"switch_next",		frame_switch_next),
	FN_VOID(generic, WFrame, 	"switch_prev",		frame_switch_prev),
	FN(s,	generic, WFrame,	"split",			split),
	FN(s,	generic, WFrame,	"split_empty",		split_empty),
	FN_VOID(generic, WFrame,	"attach_tagged",	frame_attach_tagged),
	FN_VOID(generic, WFrame,	"destroy_frame",	region_request_close),
	FN_VOID(generic, WFrame,	"closedestroy",		close_propagate),
	FN_VOID(generic, WFrame,	"close",			close_sub_propagate),
	FN_VOID(generic, WFrame,	"close_main",		close_sub),
	
	FN_VOID(generic, WWindow,	"goto_above",		goto_above),
	FN_VOID(generic, WWindow,	"goto_below",		goto_below),
	FN_VOID(generic, WWindow,	"goto_right",		goto_right),
	FN_VOID(generic, WWindow,	"goto_left",		goto_left),
	FN_VOID(generic, WWindow,	"resize_vert",		resize_vert),
	FN_VOID(generic, WWindow,	"resize_horiz",		resize_horiz),
	FN_VOID(generic, WWindow,	"maximize_vert", 	maximize_vert),
	FN_VOID(generic, WWindow,	"maximize_horiz", 	maximize_horiz),

	/* client */
	FN_VOID(cclient, WFrame,	"kill",				kill_clientwin),
	/*FN_VOID(cclient, WFrame,	"toggle_tagged",	client_toggle_tagged),*/
	FN_VOID(generic, WRegion,	"switch_tab",		switch_region),
	FN(l,	generic, WFrame,	"set_width",		set_width),
	FN(l,	generic, WFrame,	"set_height",		set_height),
	FN(d,	generic, WFrame,	"set_widthq",		set_widthq),
	FN(d,	generic, WFrame,	"set_heightq",		set_heightq),

	FN(s,	generic, WWorkspace,"split_top",		split_top),

	/* screen */
	FN_SCREEN(l,				"switch_ws_nth",	screen_switch_nth),
	FN_SCREEN_VOID(				"switch_ws_next",	screen_switch_next),     
	FN_SCREEN_VOID(				"switch_ws_prev",	screen_switch_prev),     
	FN_SCREEN(s,				"exec",				wm_exec),

	/* global */
	FN_GLOBAL_VOID(				"goto_previous",	goto_previous),
	FN_GLOBAL_VOID(				"restart",			wm_restart),
	FN_GLOBAL_VOID(				"exit",				wm_exit),
	FN_GLOBAL(s,				"restart_other",	wm_restart_other),
	FN_GLOBAL(s,				"switch_ws_name",	switch_workspace_name),
	FN_GLOBAL(ll,				"switch_ws_nth2",	screen_switch_nth2),
/*	FN_GLOBAL_VOID(				"clear_tags",		clear_tags),*/

	/* mouse move/resize and tab drag */
	FN_VOID(drag, WFrame,		"p_resize",			&frame_resize_handler),
	FN_VOID(drag, WRegion,		"p_tabdrag", 		&frame_tabdrag_handler),

	END_FUNTAB
};


/*}}}*/


/*{{{ ion_moveres_funtab */


WFunclist ion_moveres_funclist=INIT_FUNCLIST;


static WFunction ion_moveres_funtab[]={
	FN_VOID(generic, WWindow,	"end_resize",		end_resize),
	FN_VOID(generic, WWindow,	"cancel_resize",	cancel_resize),
	FN_VOID(generic, WWindow,	"grow",				grow),
	FN_VOID(generic, WWindow,	"shrink",			shrink),
	
	END_FUNTAB
};



/*}}}*/


void init_funclists()
{
	assert(add_to_funclist(&ion_main_funclist, ion_main_funtab));
	assert(add_to_funclist(&ion_moveres_funclist, ion_moveres_funtab));
}


bool command_sequence(WThing *thing, char *fn)
{
	execute_command_sequence(thing, fn, &ion_main_funclist);
}
