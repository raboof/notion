/*
 * ion/winprops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <libtu/parser.h>

#include <wmcore/common.h>
#include <wmcore/clientwin.h>
#include <wmcore/winprops.h>
#include <wmcore/attach.h>
#include "winprops.h"


static WWinProp *tmp_winprop=NULL;


/*{{{ Configuration */


static bool opt_winprop_max_size(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->max_w=TOK_LONG_VAL(&(toks[1]));;
	tmp_winprop->max_h=TOK_LONG_VAL(&(toks[2]));;
	tmp_winprop->flags|=CWIN_PROP_MAXSIZE;
	return TRUE;
}


static bool opt_winprop_aspect(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->aspect_w=TOK_LONG_VAL(&(toks[1]));;
	tmp_winprop->aspect_h=TOK_LONG_VAL(&(toks[2]));;
	tmp_winprop->flags|=CWIN_PROP_ASPECT;
	return TRUE;
}


static bool opt_winprop_switchto(Tokenizer *tokz, int n, Token *toks)
{
	if(TOK_BOOL_VAL(&(toks[1])))
		tmp_winprop->manage_flags|=REGION_ATTACH_SWITCHTO;
	else
		tmp_winprop->manage_flags&=~REGION_ATTACH_SWITCHTO;
	
	return TRUE;
}


static bool opt_winprop_stubborn(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->stubborn=TOK_BOOL_VAL(&(toks[1]));
	return TRUE;
}


static bool opt_winprop_acrobatic(Tokenizer *tokz, int n, Token *toks)
{
	tmp_winprop->flags|=CWIN_PROP_ACROBATIC;
	return TRUE;
}


static bool opt_winprop_target(Tokenizer *tokz, int n, Token *toks)
{
	if(tmp_winprop->target_name!=NULL)
		free(tmp_winprop->target_name);
	
	tmp_winprop->target_name=TOK_TAKE_STRING_VAL(&(toks[1]));
	return TRUE;
}


bool ion_begin_winprop(Tokenizer *tokz, int n, Token *toks)
{
	WWinProp *wrop;
	char *wclass, *winstance;
	
	tmp_winprop=alloc_winprop(TOK_STRING_VAL(&(toks[1])));
	
	if(tmp_winprop==NULL){
		warn_err();
		return FALSE;
	}

	return TRUE;
}
	

static bool end_winprop(Tokenizer *tokz, int n, Token *toks)
{
	add_winprop(tmp_winprop);
	tmp_winprop=NULL;
	
	return TRUE;
}


#define cancel_winprop end_winprop


ConfOpt ion_winprop_opts[]={
	{"max_size", "ll", opt_winprop_max_size, NULL},
	{"aspect", "ll", opt_winprop_aspect, NULL},
	{"acrobatic", NULL, opt_winprop_acrobatic, NULL},
	{"switchto", "b", opt_winprop_switchto, NULL},
	{"stubborn", "b", opt_winprop_stubborn, NULL},
	{"target", "s", opt_winprop_target, NULL},
		
	{"#end", NULL, end_winprop, NULL},
	{"#cancel", NULL, cancel_winprop, NULL},
	
	END_CONFOPTS
};


/*}}}*/
