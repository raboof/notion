/*
 * ion/ioncore/conf-draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "screen.h"
#include "grdata.h"
#include "readconfig.h"
#include "font.h"
#include "global.h"
#include "draw.h"


/* This all tmp_screen stuff is supposed to go away eventually. */

static WScreen *tmp_screen;

#define DO_CHK_SCR(RET) \
	if(tmp_screen==NULL){warn("Screen not set.\n"); return RET;}
#define CHK_SCR_V DO_CHK_SCR( )
#define CHK_SCR DO_CHK_SCR(FALSE)


/*{{{ Font */


EXTL_EXPORT
bool set_font(const char *fname)
{
	WFontPtr fnt;
	CHK_SCR;
	
	fnt=load_font(wglobal.dpy, fname);
	
	if(fnt!=NULL){
		if(tmp_screen->grdata.font!=NULL)
			free_font(wglobal.dpy, tmp_screen->grdata.font);
		tmp_screen->grdata.font=fnt;
	}
	
	return (fnt!=NULL);
}


EXTL_EXPORT
bool set_tab_font(const char *fname)
{
	WFontPtr fnt;
	CHK_SCR;
	
	fnt=load_font(wglobal.dpy, fname);
	if(fnt!=NULL){
		if(tmp_screen->grdata.tab_font!=NULL)
			free_font(wglobal.dpy, tmp_screen->grdata.tab_font);
		tmp_screen->grdata.tab_font=fnt;
	}
	
	return (fnt!=NULL);
}


EXTL_EXPORT
bool set_term_font(const char *fname)
{

	WFontPtr term_font;
	CHK_SCR;
	
	term_font=load_font(wglobal.dpy, fname);
	
	if(term_font!=NULL){
		tmp_screen->w_unit=MAX_FONT_WIDTH(term_font);
		tmp_screen->h_unit=MAX_FONT_HEIGHT(term_font);
		free_font(wglobal.dpy, term_font);
	}

	return (term_font!=NULL);
}


/*}}}*/


/*{{{ Border */


static bool do_border(WBorder *bd, int tl, int br, int ipad)
{
	if(tl<0 || br<0 || ipad<0){
		warn("Erroneous values");
		return FALSE;
	}

	bd->tl=tl;
	bd->br=br;
	bd->ipad=ipad;
	
	return TRUE;
}

EXTL_EXPORT
bool set_frame_border(int tl, int br, int ipad)
{
	CHK_SCR;
	return do_border(&(tmp_screen->grdata.frame_border), tl, br, ipad);
}

EXTL_EXPORT
bool set_tab_border(int tl, int br, int ipad)
{
	CHK_SCR;
	return do_border(&(tmp_screen->grdata.tab_border), tl, br, ipad);
}

EXTL_EXPORT
bool set_input_border(int tl, int br, int ipad)
{
	CHK_SCR;
	return do_border(&(tmp_screen->grdata.input_border), tl, br, ipad);
}


/*}}}*/

			
/*{{{ Colors */

			
static bool do_colorgroup(WScreen *scr, WColorGroup *cg,
						  const char *hl, const char *sh,
						  const char *bg, const char *fg)
{
	int cnt=0;

	/* alloc_color wil free cg->xx */
	cnt+=alloc_color(scr, hl, &(cg->hl));
	cnt+=alloc_color(scr, sh, &(cg->sh));
	cnt+=alloc_color(scr, bg, &(cg->bg));
	cnt+=alloc_color(scr, fg, &(cg->fg));
	
	if(cnt!=4){
		warn("Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}


EXTL_EXPORT
bool set_frame_colors(const char *hl, const char *sh, const char *bg,
					  const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.frame_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_tab_colors(const char *hl, const char *sh, const char *bg,
					const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.tab_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_tab_sel_colors(const char *hl, const char *sh, const char *bg,
						const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.tab_sel_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_act_frame_colors(const char *hl, const char *sh, const char *bg,
						  const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.act_frame_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_act_tab_colors(const char *hl, const char *sh, const char *bg,
						const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.act_tab_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_act_tab_sel_colors(const char *hl, const char *sh, const char *bg,
							const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.act_tab_sel_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_input_colors(const char *hl, const char *sh, const char *bg,
					  const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_screen, &(tmp_screen->grdata.input_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_background_color(const char *bg)
{
	CHK_SCR;
	if(!alloc_color(tmp_screen, bg, &(tmp_screen->grdata.frame_bgcolor))){
		warn("Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}


EXTL_EXPORT
bool set_selection_colors(const char *bg, const char *fg)
{
	CHK_SCR;
	if(!alloc_color(tmp_screen, bg, &(tmp_screen->grdata.selection_bgcolor)) ||
	   !alloc_color(tmp_screen, fg, &(tmp_screen->grdata.selection_fgcolor))){
		warn("Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}
		

/*}}}*/


/*{{{ Ion-specific */


EXTL_EXPORT
bool set_ion_spacing(int spacing)
{
	CHK_SCR;
	if(spacing<0){
		warn("Invalid value");
		return FALSE;
	}
	
	tmp_screen->grdata.spacing=spacing;
	
	return TRUE;
}


EXTL_EXPORT
void enable_ion_bar_inside_frame(bool enable)
{
	CHK_SCR_V;
	tmp_screen->grdata.bar_inside_frame=enable;
}


/*}}}*/


/*{{{ PWM-specific */

/* Move to floatws module? */


EXTL_EXPORT
bool set_pwm_bar_widths(int i, double j)
{
	CHK_SCR;
	if(i<0 || j<0.0){
		warn("Erroneous values");
		return FALSE;
	}
	
	tmp_screen->grdata.pwm_tab_min_width=i;
	tmp_screen->grdata.pwm_bar_max_width_q=j;
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc. */


EXTL_EXPORT
void enable_transparent_background(bool enable)
{
	CHK_SCR_V;
	tmp_screen->grdata.transparent_background=enable;
}


/*}}}*/


bool read_draw_config(WScreen *scr)
{
	bool ret;
	
	tmp_screen=scr;
	ret=read_config_for_scr("draw", scr->xscr);
	tmp_screen=NULL;
	
	return ret;
}

