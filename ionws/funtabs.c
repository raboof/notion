/*
 * ion/funtab.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/exec.h>
#include <wmcore/focus.h>
#include <wmcore/functionp.h>
#include <wmcore/clientwin.h>
#include <wmcore/screen.h>
#include <wmcore/commandsq.h>
#include <wmcore/region.h>
#include <wmcore/objp.h>
#include <wmcore/tags.h>
#include "frame.h"
#include "frame-pointer.h"
#include "ionws.h"
#include "resize.h"
#include "funtabs.h"
#include "split.h"
#include "splitframe.h"


/*{{{ Call handlers */


#define WSCURRENT_HANDLE(HND, T, G)               \
	WRegion *reg;                                 \
	typedef void Func(WThing*, T);                \
	assert(WTHING_IS(thing, WIonWS));             \
	reg=ionws_find_current((WIonWS*)thing);       \
	if(reg!=NULL)                                 \
		((Func*)func->fn)((WThing*)reg, G(args));

void callhnd_wscurrent_void(WThing *thing, WFunction *func,
							int n, const Token *args)
{
	WRegion *reg;
	typedef void Func(WThing*);
	assert(WTHING_IS(thing, WIonWS));
	reg=ionws_find_current((WIonWS*)thing);
	if(reg!=NULL)
		((Func*)func->fn)((WThing*)reg);
}


void callhnd_wscurrent_l(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
	WSCURRENT_HANDLE(callhnd_generic_l, long, TOK_LONG_VAL);
}


void callhnd_wscurrent_d(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
	WSCURRENT_HANDLE(callhnd_generic_d, double, TOK_DOUBLE_VAL);
}


void callhnd_wscurrent_s(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
	WSCURRENT_HANDLE(callhnd_generic_s, const char*, TOK_STRING_VAL);
}

#undef WSCURRENT_HANDLE


/*}}}*/


/*{{{ Misc. wrappers */


static void frame_toggle_sub_tag(WFrame *frame)
{
	if(frame->current_sub!=NULL)
		toggle_region_tag(frame->current_sub);
}


/*}}}*/


/*{{{ Function tables */


WFunclist ion_ionws_funclist=INIT_FUNCLIST;

static WFunction ion_ionws_funtab[]={
	FN_VOID(generic, WIonWS,	"ionws_goto_above",		ionws_goto_above),
	FN_VOID(generic, WIonWS,	"ionws_goto_below",		ionws_goto_below),
	FN_VOID(generic, WIonWS,	"ionws_goto_right",		ionws_goto_right),
	FN_VOID(generic, WIonWS,	"ionws_goto_left",		ionws_goto_left),
	
	FN(s,	generic, WIonWS,	"ionws_split_top",		split_top),
	FN(s,	wscurrent, WIonWS,	"ionws_split",			split),
	FN(s,	wscurrent, WIonWS,	"ionws_split_empty", 	split_empty),
	
	/*
	FN_VOID(wscurrent, WIonWS,	"resize_vert",		resize_vert),
	FN_VOID(wscurrent, WIonWS,	"resize_horiz",		resize_horiz),
	FN_VOID(wscurrent, WIonWS,	"maximize_vert", 	maximize_vert),
	FN_VOID(wscurrent, WIonWS,	"maximize_horiz", 	maximize_horiz),
	FN(l,	wscurrent, WIonWS,	"set_width",		set_width),
	FN(l,	wscurrent, WIonWS,	"set_height",		set_height),
	FN(d,	wscurrent, WIonWS,	"set_widthq",		set_widthq),
	FN(d,	wscurrent, WIonWS,	"set_heightq",		set_heightq),
	 */
	END_FUNTAB
};


WFunclist ion_frame_funclist=INIT_FUNCLIST;

static WFunction ion_frame_funtab[]={
	FN(l,	generic, WFrame,	"frame_switch_nth",		frame_switch_nth),
	FN_VOID(generic, WFrame, 	"frame_switch_next",	frame_switch_next),
	FN_VOID(generic, WFrame, 	"frame_switch_prev",	frame_switch_prev),
	FN_VOID(generic, WFrame,	"frame_attach_tagged",	frame_attach_tagged),
	FN_VOID(generic, WFrame,	"frame_toggle_sub_tag",	frame_toggle_sub_tag),
	FN_VOID(generic, WFrame,	"frame_close",			frame_close),
	FN_VOID(generic, WFrame,	"frame_close_if_empty", frame_close_if_empty),

	FN_VOID(generic, WRegion,	"frame_resize_vert",	resize_vert),
	FN_VOID(generic, WRegion,	"frame_resize_horiz",	resize_horiz),
	FN_VOID(generic, WFrame,	"frame_maximize_vert", 	maximize_vert),
	FN_VOID(generic, WFrame,	"frame_maximize_horiz", maximize_horiz),
	FN(l,	generic, WRegion,	"frame_set_width",		set_width),
	FN(l,	generic, WRegion,	"frame_set_height",		set_height),
	FN(d,	generic, WRegion,	"frame_set_widthq",		set_widthq),
	FN(d,	generic, WRegion,	"frame_set_heightq",	set_heightq),

	/* mouse move/resize and tab drag */
	FN_VOID(generic, WFrame,	"frame_p_resize",		p_resize_setup),
	FN_VOID(generic, WRegion,	"frame_p_tabdrag", 		p_tabdrag_setup),
	FN_VOID(generic, WRegion,	"frame_p_switch_tab",	frame_switch_tab),

	FN_VOID(generic, WFrame,	"frame_move_current_tab_left", frame_move_current_tab_left),
	FN_VOID(generic, WFrame,	"frame_move_current_tab_right", frame_move_current_tab_right),

	/* Common functions */
	FN_VOID(generic, WFrame,	"close",				frame_close_if_empty),
	
	END_FUNTAB
};


WFunclist ion_moveres_funclist=INIT_FUNCLIST;

static WFunction ion_moveres_funtab[]={
	FN_VOID(generic, WWindow,	"end_resize",		end_resize),
	FN_VOID(generic, WWindow,	"cancel_resize",	cancel_resize),
	FN_VOID(generic, WWindow,	"grow",				grow),
	FN_VOID(generic, WWindow,	"shrink",			shrink),
	
	END_FUNTAB
};


/*}}}*/


/*{{{ Initialisation */


void init_funclists()
{
	assert(add_to_funclist(&ion_ionws_funclist, ion_ionws_funtab));
	assert(add_to_funclist(&ion_frame_funclist, ion_frame_funtab));
	assert(add_to_funclist(&ion_moveres_funclist, ion_moveres_funtab));
}


/*}}}*/
