/*
 * ion/ioncore/conf-draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "rootwin.h"
#include "grdata.h"
#include "readconfig.h"
#include "font.h"
#include "global.h"
#include "draw.h"


/* This all tmp_rootwin stuff is supposed to go away eventually. */

static WRootWin *tmp_rootwin;

#define DO_CHK_SCR(RET) \
	if(tmp_rootwin==NULL){warn("Screen not set.\n"); RET;}
#define CHK_SCR_V DO_CHK_SCR(return)
#define CHK_SCR DO_CHK_SCR(return FALSE)


/*{{{ Font */


EXTL_EXPORT
bool set_font(const char *fname)
{
	WFontPtr fnt;
	CHK_SCR;
	
	fnt=load_font(wglobal.dpy, fname);
	
	if(fnt!=NULL){
		if(tmp_rootwin->grdata.font!=NULL)
			free_font(wglobal.dpy, tmp_rootwin->grdata.font);
		tmp_rootwin->grdata.font=fnt;
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
		if(tmp_rootwin->grdata.tab_font!=NULL)
			free_font(wglobal.dpy, tmp_rootwin->grdata.tab_font);
		tmp_rootwin->grdata.tab_font=fnt;
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
		tmp_rootwin->grdata.w_unit=MAX_FONT_WIDTH(term_font);
		tmp_rootwin->grdata.h_unit=MAX_FONT_HEIGHT(term_font);
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
	return do_border(&(tmp_rootwin->grdata.frame_border), tl, br, ipad);
}

EXTL_EXPORT
bool set_tab_border(int tl, int br, int ipad)
{
	CHK_SCR;
	return do_border(&(tmp_rootwin->grdata.tab_border), tl, br, ipad);
}

EXTL_EXPORT
bool set_input_border(int tl, int br, int ipad)
{
	CHK_SCR;
	return do_border(&(tmp_rootwin->grdata.input_border), tl, br, ipad);
}


/*}}}*/

			
/*{{{ Colors */

			
static bool do_colorgroup(WRootWin *rootwin, WColorGroup *cg,
						  const char *hl, const char *sh,
						  const char *bg, const char *fg)
{
	int cnt=0;
	
	if(hl==NULL || sh==NULL || bg==NULL || fg==NULL){
		warn("Color specification contains NULL strings");
		return FALSE;
	}

	/* alloc_color wil free cg->xx */
	cnt+=alloc_color(rootwin, hl, &(cg->hl));
	cnt+=alloc_color(rootwin, sh, &(cg->sh));
	cnt+=alloc_color(rootwin, bg, &(cg->bg));
	cnt+=alloc_color(rootwin, fg, &(cg->fg));
	
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
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.frame_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_tab_colors(const char *hl, const char *sh, const char *bg,
					const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.tab_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_tab_sel_colors(const char *hl, const char *sh, const char *bg,
						const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.tab_sel_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_act_frame_colors(const char *hl, const char *sh, const char *bg,
						  const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.act_frame_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_act_tab_colors(const char *hl, const char *sh, const char *bg,
						const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.act_tab_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_act_tab_sel_colors(const char *hl, const char *sh, const char *bg,
							const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.act_tab_sel_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_input_colors(const char *hl, const char *sh, const char *bg,
					  const char *fg)
{
	CHK_SCR;
	return do_colorgroup(tmp_rootwin, &(tmp_rootwin->grdata.input_colors),
						 hl, sh, bg, fg);
}


EXTL_EXPORT
bool set_background_color(const char *bg)
{
	CHK_SCR;
	if(bg==NULL){
		warn("Color set to NULL.");
		return FALSE;
	}
	   
	if(!alloc_color(tmp_rootwin, bg, &(tmp_rootwin->grdata.frame_bgcolor))){
		warn("Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}


EXTL_EXPORT
bool set_selection_colors(const char *bg, const char *fg)
{
	CHK_SCR;
	
	if(bg==NULL || fg==NULL){
		warn("Color set to NULL.");
		return FALSE;
	}

	if(!alloc_color(tmp_rootwin, bg, &(tmp_rootwin->grdata.selection_bgcolor)) ||
	   !alloc_color(tmp_rootwin, fg, &(tmp_rootwin->grdata.selection_fgcolor))){
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
	
	tmp_rootwin->grdata.spacing=spacing;
	
	return TRUE;
}


EXTL_EXPORT
void enable_ion_bar_inside_frame(bool enable)
{
	CHK_SCR_V;
	tmp_rootwin->grdata.bar_inside_frame=enable;
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
	
	tmp_rootwin->grdata.pwm_tab_min_width=i;
	tmp_rootwin->grdata.pwm_bar_max_width_q=j;
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc. */


EXTL_EXPORT
void enable_transparent_background(bool enable)
{
	CHK_SCR_V;
	tmp_rootwin->grdata.transparent_background=enable;
}


/*}}}*/


bool read_draw_config(WRootWin *rootwin)
{
	bool ret;
	
	tmp_rootwin=rootwin;
	ret=read_config_for_scr("draw", rootwin->xscr);
	tmp_rootwin=NULL;
	
	return ret;
}

