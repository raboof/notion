/*
 * ion/query/wedln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/drawp.h>
#include <ioncore/thingp.h>
#include <ioncore/font.h>
#include <ioncore/xic.h>
#include <ioncore/selection.h>
#include <ioncore/event.h>
#include <ioncore/regbind.h>
#include "edln.h"
#include "wedln.h"
#include "inputp.h"


#define TEXT_AREA_HEIGHT(GRDATA) \
	(MAX_FONT_HEIGHT(INPUT_FONT(GRDATA))+INPUT_BORDER_SIZE(GRDATA))


static void wedln_calc_size(WEdln *wedln, WRectangle *geom);
static void wedln_scrollup_completions(WEdln *edln);
static void wedln_scrolldown_completions(WEdln *edln);
static void wedln_insstr(WEdln *edln, const char *buf, size_t n);
static void wedln_draw_config_updated(WEdln *edln);

static DynFunTab wedln_dynfuntab[]={
	{draw_window,		wedln_draw},
	{input_calc_size, 	wedln_calc_size},
	{input_scrollup, 	wedln_scrollup_completions},
	{input_scrolldown,	wedln_scrolldown_completions},
	{window_insstr,		wedln_insstr},
	{region_draw_config_updated, wedln_draw_config_updated},
	END_DYNFUNTAB
};


IMPLOBJ(WEdln, WInput, deinit_wedln, wedln_dynfuntab, &query_edln_funclist)


/*{{{ Drawing primitives */


static int wedln_draw_strsect(DrawInfo *dinfo, int x, const char *str,
							  int len, int col)
{
	int ty=I_Y+I_H/2-MAX_FONT_HEIGHT(FONT)/2+FONT_BASELINE(FONT);
	WColor *fg, *bg;

	if(len==0)
		return 0;
	
	if(col==2){
		fg = &COLORS->bg;
		bg = &COLORS->fg;
	}else if(col==1){
		fg = &GRDATA->selection_fgcolor;
		bg = &GRDATA->selection_bgcolor;
	}else{
		fg = &COLORS->fg;
		bg = &COLORS->bg;
	}
	
	draw_image_string(dinfo, I_X+x, ty, str, len, fg, bg);
	
	return text_width(FONT, str, len);
}

#define DSTRSECT(LEN, INV) \
	{tx+=wedln_draw_strsect(dinfo, tx, str, LEN, INV); str+=LEN; len-=LEN;}


static void wedln_do_draw_str_box(DrawInfo *dinfo, const char *str,
								  int cursor, int mark, int tx)
{
	int len=strlen(str), ll;
	XRectangle rect;
	
	rect.x=I_X; rect.y=I_Y; rect.width=I_W; rect.height=I_H;
	XSetClipRectangles(wglobal.dpy, XGC, 0, 0, &rect, 1, Unsorted);

	if(mark<=cursor){
		if(mark>=0){
			DSTRSECT(mark, 0);
			DSTRSECT(cursor-mark, 1);
		}else{
			DSTRSECT(cursor, 0);
		}
		if(len==0){
			tx+=wedln_draw_strsect(dinfo, tx, " ", 1, 2);
		}else{
			ll=str_nextoff(str);
			DSTRSECT(ll, 2);
		}
	}else{
		DSTRSECT(cursor, 0);
		ll=str_nextoff(str);
		DSTRSECT(ll, 2);
		DSTRSECT(mark-cursor-ll, 1);
	}
	DSTRSECT(len, 0);

	if(tx<I_W){
		set_foreground(wglobal.dpy, XGC, COLORS->bg);
		XFillRectangle(wglobal.dpy, WIN, XGC, I_X+tx, I_Y, I_W-tx, I_H);
	}
	XSetClipMask(wglobal.dpy, XGC, None);
}


static void wedln_draw_str_box(DrawInfo *dinfo, int vstart, const char *str,
							   int dstart, int point, int mark)
{
	int tx=0;

	if(mark>=0){
		mark-=vstart+dstart;
		if(mark<0)
			mark=0;
	}
	
	point-=vstart+dstart;
	
	if(dstart!=0)
		tx=text_width(FONT, str+vstart, dstart);
	
	wedln_do_draw_str_box(dinfo, str+vstart+dstart, point, mark, tx);
}


static bool wedln_update_cursor(WEdln *wedln, WFontPtr font, int iw)
{
	int cx, l;
	int vstart=wedln->vstart;
	int point=wedln->edln.point;
	int len=wedln->edln.psize;
	int mark=wedln->edln.mark;
	const char *str=wedln->edln.p;
	bool ret;
	
	if(point<wedln->vstart)
		wedln->vstart=point;
	
	if(wedln->vstart==point)
		return FALSE;
	
	while(vstart<point){
		if(point==len){
			cx=text_width(font, str+vstart, point-vstart);
			cx+=text_width(font, " ", 1);
		}else{
			cx=text_width(font, str+vstart, point-vstart+1);
		}
		l=cx;
		
		if(l<iw)
			break;
		
		vstart++;
	}
	
	ret=(wedln->vstart!=vstart);
	wedln->vstart=vstart;
	
	return ret;
}


/*}}}*/


/*{{{ Size/location calc */


static void get_textarea_geom(WEdln *wedln, DrawInfo *dinfo)
{
	int th=TEXT_AREA_HEIGHT(GRDATA);
	WRectangle geom=REGION_GEOM(wedln);
	
	if(geom.h<th)
		th=geom.h;
	
	dinfo->geom.x=wedln->prompt_w;
	dinfo->geom.y=geom.h-th;
	dinfo->geom.w=geom.w-wedln->prompt_w;
	dinfo->geom.h=th;
}


static void get_geom(WEdln *wedln, DrawInfo *dinfo,
					 bool complist, bool max)
{
	WRectangle geom=(max ? wedln->input.max_geom : REGION_GEOM(wedln));
	int th=TEXT_AREA_HEIGHT(GRDATA);

	if(geom.h<th)
		th=geom.h;
	
	geom.x=0;
	geom.y=0;
	
	if(!complist){
		geom.y=geom.h-th;
		geom.h=th;
	}else{
		geom.h-=th+GRDATA->spacing;
	}
	
	dinfo->geom=geom;
}


static void setup_wedln_dinfo(WEdln *wedln, DrawInfo *dinfo,
							  bool complist, bool max)
{
	setup_input_dinfo((WInput*)wedln, dinfo);
	get_geom(wedln, dinfo, complist, max);
}


static void wedln_calc_size(WEdln *wedln, WRectangle *geom)
{
	WScreen *scr=SCREEN_OF(wedln);
	WGRData *grdata=&(scr->grdata);
	int h, th;
	WRectangle max_geom=*geom;
	DrawInfo dinfo_;
	
	th=TEXT_AREA_HEIGHT(grdata);
	
	if(wedln->complist.strs==NULL){
		if(max_geom.h<th)
			geom->h=max_geom.h;
		else
			geom->h=th;
	}else{
		setup_wedln_dinfo(wedln, &dinfo_, TRUE, TRUE);
		fit_listing(&dinfo_, &(wedln->complist));

		h=wedln->complist.toth;
		th+=scr->grdata.spacing+INPUT_BORDER_SIZE(grdata);
		
		if(h+th>max_geom.h)
			h=max_geom.h-th;
		geom->h=h+th;
	}
	
	geom->w=max_geom.w;
	geom->y=max_geom.y+max_geom.h-geom->h;
	geom->x=max_geom.x;

	wedln_update_cursor(wedln, INPUT_FONT(grdata),
						geom->w-wedln->prompt_w-INPUT_BORDER_SIZE(grdata));
}


/*}}}*/


/*{{{ Draw */


static void wedln_update_handler(WEdln *wedln, int from, bool moved)
{
	DrawInfo dinfo_, *dinfo=&dinfo_;
	
	setup_wedln_dinfo(wedln, &dinfo_, FALSE, FALSE);
	get_textarea_geom(wedln, &dinfo_);
	
	from-=wedln->vstart;
	
	if(moved){
		if(wedln_update_cursor(wedln, FONT, I_W))
			from=0;
	}
	
	if(from<0)
		from=0;

	wedln_draw_str_box(dinfo, wedln->vstart, wedln->edln.p, from,
					   wedln->edln.point, wedln->edln.mark);
}


void wedln_draw_completions(WEdln *wedln, bool complete)
{
	DrawInfo dinfo_;
	if(wedln->complist.strs!=NULL){
		setup_wedln_dinfo(wedln, &dinfo_, TRUE, FALSE);
		draw_listing(&dinfo_, &(wedln->complist), complete);
	}
}

	
void wedln_draw_textarea(WEdln *wedln, bool complete)
{
	DrawInfo dinfo_, *dinfo=&dinfo_;
	int ty;

	setup_wedln_dinfo(wedln, dinfo, FALSE, FALSE);
	draw_box(dinfo, FALSE);
	
	if(wedln->prompt!=NULL){
		ty=I_Y+I_H/2-MAX_FONT_HEIGHT(FONT)/2+FONT_BASELINE(FONT);
		draw_image_string(dinfo, I_X, ty, wedln->prompt, wedln->prompt_len,
				          &COLORS->fg, &COLORS->bg);
	}

	get_textarea_geom(wedln, dinfo);
	wedln_draw_str_box(dinfo, wedln->vstart, wedln->edln.p, 0,
					   wedln->edln.point, wedln->edln.mark);
}


void wedln_draw(WEdln *wedln, bool complete)
{
	wedln_draw_completions(wedln, complete);
	wedln_draw_textarea(wedln, complete);
}


/*}}}*/


/*{{{ Completions */


void wedln_show_completions(WEdln *wedln, char **strs, int nstrs)
{
	setup_listing(&(wedln->complist), INPUT_FONT(GRDATA_OF(wedln)),
				  strs, nstrs);
	input_refit((WInput*)wedln);
	/*?*/ wedln_draw_completions(wedln, TRUE);
}


void wedln_hide_completions(WEdln *wedln)
{
	if(wedln->complist.strs!=NULL){
		deinit_listing(&(wedln->complist));
		input_refit((WInput*)wedln);
	}
}
	

void wedln_scrollup_completions(WEdln *wedln)
{
	if(wedln->complist.strs==NULL)
		return;
	if(scrollup_listing(&(wedln->complist)))
		wedln_draw_completions(wedln, TRUE);
}


void wedln_scrolldown_completions(WEdln *wedln)
{
	if(wedln->complist.strs==NULL)
		return;
	if(scrolldown_listing(&(wedln->complist)))
		wedln_draw_completions(wedln, TRUE);
}

	
/*}}}*/


/*{{{ Init, deinit and config update */


static bool wedln_init_prompt(WEdln *wedln, WScreen *scr, const char *prompt)
{
	char *p;
	
	if(prompt!=NULL){
		p=scat(prompt, "  ");
	
		if(p==NULL){
			warn_err();
			return FALSE;
		}
		wedln->prompt=p;
		wedln->prompt_len=strlen(p);
		wedln->prompt_w=text_width(INPUT_FONT(&(scr->grdata)),
								   p, wedln->prompt_len);
	}else{
		wedln->prompt=NULL;
		wedln->prompt_len=0;
		wedln->prompt_w=0;
	}
	
	return TRUE;
}


bool init_wedln(WEdln *wedln, WWindow *par, WRectangle geom,
				WEdlnHandler *handler, const char *prompt, const char *dflt)
{
	wedln->vstart=0;
	wedln->handler=handler;
	wedln->userdata=NULL;

	if(!wedln_init_prompt(wedln, SCREEN_OF(par), prompt))
		return FALSE;
	
	if(!edln_init(&(wedln->edln), dflt)){
		free(wedln->prompt);
		return FALSE;
	}
	
	wedln->edln.uiptr=wedln;
	wedln->edln.ui_update=(EdlnUpdateHandler*)wedln_update_handler;
	wedln->edln.ui_show_completions=(EdlnShowComplHandler*)wedln_show_completions;
	wedln->edln.ui_hide_completions=(EdlnHideComplHandler*)wedln_hide_completions;

	init_listing(&(wedln->complist));
	
	if(!init_input((WInput*)wedln, par, geom)){
		edln_deinit(&(wedln->edln));
		free(wedln->prompt);
		return FALSE;
	}

	wedln->input.win.xic=create_xic(wedln->input.win.win);

	region_add_bindmap((WRegion*)wedln, &query_edln_bindmap, FALSE);
	
	return TRUE;
}


WEdln *create_wedln(WWindow *par, WRectangle geom, WEdlnCreateParams *fnp)
{
	CREATETHING_IMPL(WEdln, wedln, (p, par, geom, fnp->handler,
									fnp->prompt, fnp->dflt));
}


static void wedln_draw_config_updated(WEdln *wedln)
{
	WFontPtr fnt=INPUT_FONT(GRDATA_OF(wedln));
	listing_set_font(&(wedln->complist), fnt);
	wedln->prompt_w=text_width(fnt, wedln->prompt, wedln->prompt_len);
	input_draw_config_updated((WInput*)wedln);
}


void deinit_wedln(WEdln *wedln)
{
	if(wedln->prompt!=NULL)
		free(wedln->prompt);

	if(wedln->userdata!=NULL)
		free(wedln->userdata);
	
	if(wedln->complist.strs!=NULL)
		deinit_listing(&(wedln->complist));

	edln_deinit(&(wedln->edln));
	deinit_input((WInput*)wedln);
}


void wedln_finish(WEdln *wedln)
{
	WThing *parent;
	WEdlnHandler *handler;
	char *p;
	char *userdata;
	
	handler=wedln->handler;
	parent=((WThing*)wedln)->t_parent;
	p=edln_finish(&(wedln->edln));
	userdata=wedln->userdata;
	wedln->userdata=NULL;
	
	destroy_thing((WThing*)wedln);
	
	if(handler!=NULL)
		handler(parent, p, userdata);
	
	if(userdata!=NULL)
		free(userdata);
}


/*}}}*/


/*{{{ The rest */


void wedln_paste(WEdln *wedln)
{
	request_selection(wedln->input.win.win);
}


void wedln_insstr(WEdln *wedln, const char *buf, size_t n)
{
	edln_insstr_n(&(wedln->edln), buf, n);
	/*wedln_draw(wedln, FALSE);*/
}


void wedln_set_completion_handler(WEdln *wedln, EdlnCompletionHandler *h,
								  void *d)
{
	wedln->edln.completion_handler=h;
	wedln->edln.completion_handler_data=d;
}


/*}}}*/

