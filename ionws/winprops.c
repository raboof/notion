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
#include <wmcore/winpropsgen.h>
#include "winprops.h"


static WWinProp *winprop_list=NULL;
static WWinProp *tmp_winprop=NULL;


/*{{{ Create/free/find */


WWinProp *alloc_winprop(const char *winname)
{
	char *wclass, *winstance;
	WWinProp *winprop;
	
	winprop=ALLOC(WWinProp);
	
	if(winprop==NULL){
		warn_err();
		return NULL;
	}
		
	if(!init_winpropgen(&winprop->genprop, winname)){
		free(winprop);
		return NULL;
	}
		
	winprop->flags=0;
	winprop->switchto=-1;
	winprop->stubborn=0;

	return winprop;
}


WWinProp *find_winprop_win(Window win)
{
	return (WWinProp*)find_winpropgen_win((WWinPropGen*)winprop_list,
										  win);
}


void free_winprop(WWinProp *winprop)
{
	free_winpropgen((WWinPropGen**)&winprop_list, (WWinPropGen*)winprop);
}


void free_winprops()
{
	while(winprop_list!=NULL)
		free_winprop(winprop_list);
}


/*}}}*/


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
	tmp_winprop->switchto=TOK_BOOL_VAL(&(toks[1]));
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
	add_winpropgen((WWinPropGen**)&winprop_list, (WWinPropGen*)tmp_winprop);
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
	
	{"#end", NULL, end_winprop, NULL},
	{"#cancel", NULL, cancel_winprop, NULL},
	
	END_CONFOPTS
};


/*}}}*/
