/*
 * ion/ioncore/conf-bindings.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/tokenizer.h>
#include <libtu/map.h>

#include "common.h"
#include "binding.h"
#include "readconfig.h"
#include "global.h"


static WBindmap *tmp_bindmap=NULL;
static const StringIntMap *tmp_areamap=NULL;


/*{{{ Helpers */


static bool get_func(Tokenizer *tokz, Token *toks, int n, WBinding *binding)
{
	binding->cmd=TOK_TAKE_STRING_VAL(&(toks[0]));
	
	return TRUE;
}


#define BUTTON1_NDX 9


static StringIntMap state_map[]={
	{"Shift",		ShiftMask},
	{"Lock",		LockMask},
	{"Control",		ControlMask},
	{"Mod1",		Mod1Mask},
	{"Mod2",		Mod2Mask},
	{"Mod3",		Mod3Mask},
	{"Mod4",		Mod4Mask},
	{"Mod5",		Mod5Mask},
	{"AnyModifier",	AnyModifier},
	{"Button1",		Button1},
	{"Button2",		Button2},
	{"Button3",		Button3},
	{"Button4",		Button4},
	{"Button5",		Button5},
	{"AnyButton",	AnyButton},
	{NULL,  		0},
};


static bool parse_keybut(Tokenizer *tokz, Token *tok,
						 uint *mod_ret, uint *kcb_ret, bool button)
{
	
	char *p=TOK_STRING_VAL(tok);
	char *p2;
	int keysym=NoSymbol, i;
	
	while(*p!='\0'){
		p2=strchr(p, '+');
		
		if(p2!=NULL)
			*p2='\0';
		
		if(!button)
			keysym=XStringToKeysym(p);
		
		if(!button && keysym!=NoSymbol){
			if(*kcb_ret!=NoSymbol){
				tokz_warn(tokz, tok->line, "Insane key combination");
				return FALSE;
			}
			*kcb_ret=keysym;
		}else{
			i=stringintmap_ndx(state_map, p);

			if(i<0){
				tokz_warn(tokz, tok->line, "\"%s\" unknown", p);
				return FALSE;
			}
			
			if(i>=BUTTON1_NDX){
				if(!button || *kcb_ret!=NoSymbol){
					tokz_warn(tokz, tok->line, "Insane button combination");
					return FALSE;
				}
				*kcb_ret=state_map[i].value;
			}else{
				*mod_ret|=state_map[i].value;
			}
		}

		if(p2==NULL)
			break;
		
		p=p2+1;
	}
	
	return TRUE;
}

#undef BUTTON1_NDX

/*}}}*/


/*{{{ Bindings */


/* action "mods+key/button", "func" [, args] */
static bool do_bind(Tokenizer *tokz, int n, Token *toks, int act, bool wr,
					int area)
{
	WBinding binding=BINDING_INIT;
	uint kcb=NoSymbol, mod=tmp_bindmap->confdefmod;
	
	if(!parse_keybut(tokz, &(toks[0]), &mod, &kcb, act!=ACT_KEYPRESS))
		return TRUE;

	if(act==ACT_KEYPRESS){
		kcb=XKeysymToKeycode(wglobal.dpy, kcb);
		if(kcb==0)
			return FALSE;
	}
	
	if(wr && mod==0){
		tokz_warn(tokz, toks[0].line, "Cannot waitrel when no modifiers set. "
				  "Sorry.");
	}

	binding.waitrel=wr;
	binding.act=act;
	binding.state=mod;
	binding.kcb=kcb;
	binding.area=area;
	
	if(!get_func(tokz, &(toks[1]), n-1, &binding))
		return TRUE; /* just ignore the error */ 
	
	if(add_binding(tmp_bindmap, &binding))
		return TRUE;

	deinit_binding(&binding);
	
	tokz_warn(tokz, toks[0].line, "Unable to bind \"%s\" to \"%s\"", 
			  TOK_STRING_VAL(&(toks[0])), TOK_STRING_VAL(&(toks[1])));
	
	return TRUE;
}


static bool do_mbind(Tokenizer *tokz, int n, Token *toks, int act, bool wr)
{
	int ndx=-1, area=0;

	if(TOK_IS_IDENT(&(toks[0]))){
		if(tmp_areamap!=NULL)
			ndx=stringintmap_ndx(tmp_areamap, TOK_IDENT_VAL(&(toks[0])));
		if(ndx<0){
			tokz_warn(tokz, toks[1].line, "Unknown area");
			return FALSE;
		}
		area=tmp_areamap[ndx].value;
		n--;
		toks++;
	}
	return do_bind(tokz, n, toks, act, wr, area);
}


static bool opt_kpress(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n-1, toks+1, ACT_KEYPRESS, FALSE, 0);
}


static bool opt_kpress_waitrel(Tokenizer *tokz, int n, Token *toks)
{
	return do_bind(tokz, n-1, toks+1, ACT_KEYPRESS, TRUE, 0);
}


static bool opt_mpress(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind(tokz, n-1, toks+1, ACT_BUTTONPRESS, FALSE);
}


static bool opt_mdrag(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind(tokz, n-1, toks+1, ACT_BUTTONMOTION, FALSE);
}


static bool opt_mclick(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind(tokz, n-1, toks+1, ACT_BUTTONCLICK, FALSE);
}


static bool opt_mdblclick(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind(tokz, n-1, toks+1, ACT_BUTTONDBLCLICK, FALSE);
}


static bool opt_submap(Tokenizer *tokz, int n, Token *toks)
{
	WBinding binding=BINDING_INIT, *bnd;
	uint kcb=NoSymbol, mod=tmp_bindmap->confdefmod;
	
	if(!parse_keybut(tokz, &(toks[1]), &mod, &kcb, 0))
		return TRUE;

	kcb=XKeysymToKeycode(wglobal.dpy, kcb);
	if(kcb==0)
		return FALSE;

	bnd=lookup_binding(tmp_bindmap, ACT_KEYPRESS, mod, kcb);
	
	if(bnd!=NULL && bnd->submap!=NULL && bnd->state==mod){
		tmp_bindmap=bnd->submap;
		return TRUE;
	}

	binding.act=ACT_KEYPRESS;
	binding.state=mod;
	binding.kcb=kcb;
	binding.cmd=NULL;
	binding.submap=create_bindmap();
	
	if(binding.submap==NULL)
		return FALSE;

	if(add_binding(tmp_bindmap, &binding)){
		binding.submap->parent=tmp_bindmap;
		tmp_bindmap=binding.submap;
		return TRUE;
	}

	deinit_binding(&binding);
	
	tokz_warn(tokz, toks[0].line, "Unable to bind \"%s\" to \"%s\"", 
			  TOK_STRING_VAL(&(toks[1])), TOK_STRING_VAL(&(toks[2])));
	
	return TRUE;
}


static bool opt_set_mod(Tokenizer *tokz, int n, Token *toks)
{
	uint mod=0, kcb=NoSymbol;
	
	if(!parse_keybut(tokz, &(toks[1]), &mod, &kcb, FALSE))
		return FALSE; 
	
	tmp_bindmap->confdefmod=mod;
	
	return TRUE;
}


static bool end_bindings(Tokenizer *tokz, int n, Token *toks)
{
	tmp_bindmap=tmp_bindmap->parent;
	return TRUE;
}


/*}}}*/


/*{{{ Exports */


bool ioncore_begin_bindings(WBindmap *bindmap, const StringIntMap *areas)
{
	tmp_bindmap=bindmap;
	tmp_areamap=areas;
	return (tmp_bindmap!=NULL);
}


ConfOpt ioncore_binding_opts[]={
	{"set_mod", "s", opt_set_mod, NULL},
	{"submap", "s", opt_submap, ioncore_binding_opts},
	
	{"kpress", "ss", opt_kpress, NULL},
	{"kpress_waitrel", "ss", opt_kpress_waitrel, NULL},
	{"mpress", "?iss", opt_mpress, NULL},
	{"mdrag", "?iss", opt_mdrag, NULL},
	{"mclick", "?iss", opt_mclick, NULL},
	{"mdblclick", "?iss", opt_mdblclick, NULL},

	{"#end", NULL, end_bindings, NULL},
	{"#cancel", NULL, end_bindings, NULL},
	
	END_CONFOPTS
};


/*}}}*/

