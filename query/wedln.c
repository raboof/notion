/*
 * ion/query/wedln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/drawp.h>
#include <ioncore/objp.h>
#include <ioncore/font.h>
#include <ioncore/xic.h>
#include <ioncore/selection.h>
#include <ioncore/event.h>
#include <ioncore/regbind.h>
#include <ioncore/extl.h>
#include <ioncore/defer.h>
#include "edln.h"
#include "wedln.h"
#include "inputp.h"
#include "complete.h"


#define TEXT_AREA_HEIGHT(GRDATA) \
	(MAX_FONT_HEIGHT(INPUT_FONT(GRDATA))+INPUT_BORDER_SIZE(GRDATA))


	
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
	WGRData *grdata=GRDATA_OF(wedln);
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
		th+=grdata->spacing+INPUT_BORDER_SIZE(grdata);
		
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


static void wedln_show_completions(WEdln *wedln, char **strs, int nstrs)
{
	setup_listing(&(wedln->complist), INPUT_FONT(GRDATA_OF(wedln)),
				  strs, nstrs, FALSE);
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


static void wedln_completion_handler(WEdln *wedln, const char *nam)
{
	extl_call(wedln->completor, "os", NULL, wedln, nam);
}


/*EXTL_DOC
 * This function should be called in completors (such given as
 * parameters to \code{query_query}) to return the set of completions
 * found. The numerical indexes of \var{completions} list the found
 * completions. If the entry \var{common_part} exists, it gives an
 * extra common prefix of all found completions.
 */
EXTL_EXPORT_MEMBER
void wedln_set_completions(WEdln *wedln, ExtlTab completions)
{
	int n=0, i=0;
	char **ptr,  *p, *beg=NULL;
	
	n=extl_table_get_n(completions);
	
	if(n==0){
		wedln_hide_completions(wedln);
		return;
	}
	
	ptr=ALLOC_N(char*, n);
	if(ptr==NULL){
		warn_err();
		goto allocfail;
	}
	for(i=0; i<n; i++){
		if(!extl_table_geti_s(completions, i+1, &p)){
			goto allocfail;
		}
		ptr[i]=p;
	}

	if(extl_table_gets_s(completions, "common_part", &p))
		beg=p;
	
	n=edln_do_completions(&(wedln->edln), ptr, n, beg);
	i=n;

	if(beg!=NULL)
		free(beg);
	
	if(n>1){
		wedln_show_completions(wedln, ptr, n);
		return;
	}
	
allocfail:
	wedln_hide_completions(wedln);
	while(i>0){
		i--;
		free(ptr[i]);
	}
	free(ptr);
}

/*}}}*/


/*{{{ Init, deinit and config update */


static bool wedln_init_prompt(WEdln *wedln, WGRData *grdata, const char *prompt)
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
		wedln->prompt_w=text_width(INPUT_FONT(grdata),
								   p, wedln->prompt_len);
	}else{
		wedln->prompt=NULL;
		wedln->prompt_len=0;
		wedln->prompt_w=0;
	}
	
	return TRUE;
}


static bool wedln_init(WEdln *wedln, WWindow *par, WRectangle geom, 
					   WEdlnCreateParams *params)
{
	wedln->vstart=0;

	if(!wedln_init_prompt(wedln, GRDATA_OF(par), params->prompt))
		return FALSE;
	
	if(!edln_init(&(wedln->edln), params->dflt)){
		free(wedln->prompt);
		return FALSE;
	}
	
	wedln->edln.uiptr=wedln;
	wedln->edln.ui_update=(EdlnUpdateHandler*)wedln_update_handler;
	wedln->edln.completion_handler=(EdlnCompletionHandler*)wedln_completion_handler;

	init_listing(&(wedln->complist));
	
	if(!input_init((WInput*)wedln, par, geom)){
		edln_deinit(&(wedln->edln));
		free(wedln->prompt);
		return FALSE;
	}

	wedln->input.win.xic=create_xic(wedln->input.win.win);
	
	wedln->handler=extl_ref_fn(params->handler);
	wedln->completor=extl_ref_fn(params->completor);

	region_add_bindmap((WRegion*)wedln, &query_wedln_bindmap);
	
	return TRUE;
}


WEdln *create_wedln(WWindow *par, WRectangle geom, WEdlnCreateParams *params)
{
	CREATEOBJ_IMPL(WEdln, wedln, (p, par, geom, params));
}


static void wedln_draw_config_updated(WEdln *wedln)
{
	WFontPtr fnt=INPUT_FONT(GRDATA_OF(wedln));
	listing_set_font(&(wedln->complist), fnt);
	wedln->prompt_w=text_width(fnt, wedln->prompt, wedln->prompt_len);
	input_draw_config_updated((WInput*)wedln);
}


static void wedln_deinit(WEdln *wedln)
{
	if(wedln->prompt!=NULL)
		free(wedln->prompt);

	if(wedln->complist.strs!=NULL)
		deinit_listing(&(wedln->complist));

	extl_unref_fn(wedln->completor);
	extl_unref_fn(wedln->handler);
	
	edln_deinit(&(wedln->edln));
	input_deinit((WInput*)wedln);
}


static void wedln_do_finish(WEdln *wedln)
{
	ExtlFn handler;
	char *p;
	
	handler=wedln->handler;
	wedln->handler=extl_fn_none();
	p=edln_finish(&(wedln->edln));
	
	destroy_obj((WObj*)wedln);
	
	if(p!=NULL)
		extl_call(handler, "s", NULL, p);
	
	free(p);
	extl_unref_fn(handler);
}


/*EXTL_DOC
 * Close \var{wedln} and call any handlers.
 */
EXTL_EXPORT_MEMBER
void wedln_finish(WEdln *wedln)
{
	defer_action((WObj*)wedln, (DeferredAction*)wedln_do_finish);
}


/*}}}*/


/*{{{ The rest */


/*EXTL_DOC
 * Request selection from application holding such.
 */
EXTL_EXPORT_MEMBER
void wedln_paste(WEdln *wedln)
{
	request_selection(wedln->input.win.win);
}


void wedln_insstr(WEdln *wedln, const char *buf, size_t n)
{
	edln_insstr_n(&(wedln->edln), buf, n);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab wedln_dynfuntab[]={
	{draw_window,		wedln_draw},
	{input_calc_size, 	wedln_calc_size},
	{input_scrollup, 	wedln_scrollup_completions},
	{input_scrolldown,	wedln_scrolldown_completions},
	{window_insstr,		wedln_insstr},
	{region_draw_config_updated, wedln_draw_config_updated},
	END_DYNFUNTAB
};


IMPLOBJ(WEdln, WInput, wedln_deinit, wedln_dynfuntab);

	
/*}}}*/
