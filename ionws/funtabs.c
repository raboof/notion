/*
 * ion/ionws/funtab.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include <ioncore/exec.h>
#include <ioncore/focus.h>
#include <ioncore/functionp.h>
#include <ioncore/clientwin.h>
#include <ioncore/screen.h>
#include <ioncore/commandsq.h>
#include <ioncore/region.h>
#include <ioncore/objp.h>
#include <ioncore/tags.h>
#include <ioncore/genframe.h>
#include <ioncore/genframe-pointer.h>
#include <ioncore/resize.h>
#include "ionframe.h"
#include "ionws.h"
#include "funtabs.h"
#include "split.h"
#include "splitframe.h"
#include "resize.h"


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


/*{{{ Function tables */


WFunclist ionws_funclist=INIT_FUNCLIST;

static WFunction ionws_funtab[]={
	FN_VOID(generic, WIonWS,	"ionws_goto_above",		ionws_goto_above),
	FN_VOID(generic, WIonWS,	"ionws_goto_below",		ionws_goto_below),
	FN_VOID(generic, WIonWS,	"ionws_goto_right",		ionws_goto_right),
	FN_VOID(generic, WIonWS,	"ionws_goto_left",		ionws_goto_left),
	
	FN(s,	generic, WIonWS,	"ionws_split_top",		split_top),
	FN(s,	wscurrent, WIonWS,	"ionws_split",			split),
	FN(s,	wscurrent, WIonWS,	"ionws_split_empty", 	split_empty),

	END_FUNTAB
};


WFunclist ionframe_funclist=INIT_FUNCLIST;

static WFunction ionframe_funtab[]={
	FN_VOID(generic, WRegion,	"genframe_resize_vert",		resize_vert),
	FN_VOID(generic, WRegion,	"genframe_resize_horiz",	resize_horiz),
	FN_VOID(generic, WIonFrame,	"ionframe_close",		ionframe_close),
	FN_VOID(generic, WIonFrame,	"ionframe_close_if_empty", ionframe_close_if_empty),
	/* Common functions */
	FN_VOID(generic, WIonFrame,	"close",				ionframe_close_if_empty),
	
	END_FUNTAB
};


WFunclist ionframe_moveres_funclist=INIT_FUNCLIST;

static WFunction ionframe_moveres_funtab[]={
	FN_VOID(generic, WWindow,	"end_resize",		end_resize),
	FN_VOID(generic, WWindow,	"cancel_resize",	cancel_resize),
	FN_VOID(generic, WWindow,	"grow",				grow),
	FN_VOID(generic, WWindow,	"shrink",			shrink),
	
	END_FUNTAB
};


/*}}}*/


/*{{{ Initialisation */


bool ionws_module_init_funclists()
{
	return (add_to_funclist(&ionws_funclist, ionws_funtab) &&
			add_to_funclist(&ionframe_funclist, ionframe_funtab) &&
			add_to_funclist(&ionframe_moveres_funclist, ionframe_moveres_funtab));
}


void ionws_module_clear_funclists()
{
	clear_funclist(&ionws_funclist);
	clear_funclist(&ionframe_funclist);
	clear_funclist(&ionframe_moveres_funclist);
}


/*}}}*/
