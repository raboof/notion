/*
 * wmcore/conf-draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <libtu/tokenizer.h>

#include "common.h"
#include "screen.h"
#include "grdata.h"
#include "conf-draw.h"
#include "readconfig.h"
#include "font.h"
#include "global.h"
#include "draw.h"


static WScreen *tmp_screen;


#define FOR_ALL_SELECTED_SCREENS(SCR) SCR=tmp_screen;


/*{{{ Font */


static bool opt_screen_font(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;

	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.font=load_font(wglobal.dpy,
								   TOK_STRING_VAL(&(toks[1])));
	}
	
	return TRUE;
}


static bool opt_screen_tab_font(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.tab_font=load_font(wglobal.dpy,
									   TOK_STRING_VAL(&(toks[1])));
	}
	
	return TRUE;
}


static bool opt_screen_term_font(Tokenizer *tokz, int n, Token *toks)
{

	WScreen *scr;
	WFont *term_font;
	int w, h;
	
	term_font=load_font(wglobal.dpy, TOK_STRING_VAL(&(toks[1])));
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->w_unit=MAX_FONT_WIDTH(term_font);
		scr->h_unit=MAX_FONT_HEIGHT(term_font);
	}

	free_font(wglobal.dpy, term_font);

	return TRUE;
}


/*}}}*/


/*{{{ Border */


static int do_border(Tokenizer *tokz, int n, Token *toks, WBorder *bd)
{
	int tl, br, ipad;
	
	tl=TOK_LONG_VAL(&(toks[1]));
	br=TOK_LONG_VAL(&(toks[2]));
	ipad=TOK_LONG_VAL(&(toks[3]));
	
	if(tl<0 || br<0 || ipad<0){
		tokz_warn(tokz, toks[1].line, "Erroneous values");
		return FALSE;
	}

	bd->tl=tl;
	bd->br=br;
	bd->ipad=ipad;
	
	return TRUE;
}

#define BDHAND(NAME)                                               \
 static int opt_screen_##NAME(Tokenizer *tokz, int n, Token *toks) \
 {                                                                 \
	WScreen *scr;                                                  \
	FOR_ALL_SELECTED_SCREENS(scr){                                 \
		do_border(tokz, n, toks, &(scr->grdata.NAME));             \
	}                                                              \
	return TRUE;                                                   \
 }


BDHAND(frame_border)
BDHAND(tab_border)
BDHAND(input_border)

#undef BDHAND

			
/*}}}*/

			
/*{{{ Colors */

			
static bool do_colorgroup(Tokenizer *tokz, Token *toks,
						  WScreen *scr, WColorGroup *cg)
{
	int cnt=0;
	
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[1])), &(cg->hl));
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[2])), &(cg->sh));
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[3])), &(cg->bg));
	cnt+=alloc_color(scr, TOK_STRING_VAL(&(toks[4])), &(cg->fg));
	
	if(cnt!=4){
		tokz_warn(tokz, toks[1].line, "Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}

#define CGHAND(CG)                                                \
 static bool opt_screen_##CG(Tokenizer *tokz, int n, Token *toks) \
 {                                                                \
	WScreen *scr;                                                 \
	FOR_ALL_SELECTED_SCREENS(scr){                                \
		do_colorgroup(tokz, toks, scr, &(scr->grdata.CG));        \
	}                                                             \
	return TRUE;                                                  \
 }

CGHAND(frame_colors)
CGHAND(tab_colors)
CGHAND(tab_sel_colors)
CGHAND(act_frame_colors)
CGHAND(act_tab_colors)
CGHAND(act_tab_sel_colors)
CGHAND(input_colors)

#undef CGHAND


static bool opt_screen_frame_bgcolor(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		/* TODO: free */
		if(!alloc_color(scr, TOK_STRING_VAL(&(toks[1])),
						&(scr->grdata.frame_bgcolor))){
			tokz_warn(tokz, toks[1].line,
					  "Unable to allocate one or more colors");
		}
	}
	
	return TRUE;
}


static bool opt_screen_selection_colors(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		/* TODO: free */
		if(!alloc_color(scr, TOK_STRING_VAL(&(toks[1])),
						&(scr->grdata.selection_bgcolor)) ||
		   !alloc_color(scr, TOK_STRING_VAL(&(toks[2])),
						&(scr->grdata.selection_fgcolor))){
		
			tokz_warn(tokz, toks[1].line,
					  "Unable to allocate one or more colors");
		}
	}
	
	return TRUE;
}
		

/*}}}*/


/*{{{ Ion-specific */


static bool opt_screen_spacing(Tokenizer *tokz, int n, Token *toks)
{
	long spacing=TOK_LONG_VAL(&(toks[1]));
	WScreen *scr;

	if(spacing<0){
		tokz_warn(tokz, toks[1].line, "Invalid value");
		return FALSE;
	}
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.spacing=spacing;
	}
	
	return TRUE;
}


static bool opt_screen_bar_inside_frame(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.bar_inside_frame=TOK_BOOL_VAL(&(toks[1]));
	}
	
	return TRUE;
}


/*}}}*/


/*{{{ PWM-specific */


static int opt_screen_bar_widths(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	int i, k;
	float j;
	
	i=TOK_LONG_VAL(&(toks[1]));
	j=TOK_DOUBLE_VAL(&(toks[2]));
	k=TOK_LONG_VAL(&(toks[3]));
	
	if(i<0 || j<0.0 || k<0 || k>i){
		warn_obj_line(tokz->name, toks[1].line, "Erroneous values");
		return FALSE;
	}
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.bar_min_width=i;
		scr->grdata.bar_max_width_q=j;
		scr->grdata.tab_min_width=k;
	}
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc. */


static bool opt_screen_transparent_background(Tokenizer *tokz, int n, Token *toks)
{
	WScreen *scr;
	
	FOR_ALL_SELECTED_SCREENS(scr){
		scr->grdata.transparent_background=TOK_BOOL_VAL(&(toks[1]));
	}
	
	return TRUE;
}


/*}}}*/


static ConfOpt screen_opts[]={
	{"font", "s", opt_screen_font, NULL},
	{"tab_font", "s", opt_screen_tab_font, NULL},
	{"term_font", "s", opt_screen_term_font, NULL},

	{"frame_border", "lll", opt_screen_frame_border, NULL},
	{"tab_border", "lll", opt_screen_tab_border, NULL},
	{"input_border", "lll", opt_screen_input_border, NULL},
	
	{"act_tab_colors", "ssss", opt_screen_act_tab_colors, NULL},
	{"act_tab_sel_colors", "ssss", opt_screen_act_tab_sel_colors, NULL},
	{"act_frame_colors", "ssss", opt_screen_act_frame_colors, NULL},
	{"tab_colors", "ssss", opt_screen_tab_colors, NULL},
	{"tab_sel_colors", "ssss", opt_screen_tab_sel_colors, NULL},
	{"frame_colors", "ssss", opt_screen_frame_colors, NULL},
	{"input_colors", "ssss", opt_screen_input_colors, NULL},
	{"background_color", "s", opt_screen_frame_bgcolor, NULL},
	{"transparent_background", "b", opt_screen_transparent_background, NULL},
	{"selection_colors", "ss", opt_screen_selection_colors, NULL},
	
	{"ion_spacing", "l", opt_screen_spacing, NULL},
	{"ion_bar_inside_frame", "b", opt_screen_bar_inside_frame, NULL},
	
	{"pwm_bar_widths", "ldl", opt_screen_bar_widths, NULL},

	
	/*{"#end", NULL, end_screen, NULL},
	{"#cancel", NULL, end_screen, NULL},*/
	
	{NULL, NULL, NULL, NULL}
};


bool read_draw_config(WScreen *scr)
{
	bool ret;
	
	tmp_screen=scr;
	ret=read_core_config_for_scr("draw", scr->xscr, screen_opts);
	tmp_screen=NULL;
	
	return ret;
}

