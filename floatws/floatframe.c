/*
 * ion/floatws/floatframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include <X11/extensions/shape.h>
#include <string.h>

#include <ioncore/genframe.h>
#include <ioncore/genframep.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/objp.h>
#include <ioncore/thingp.h>
#include <ioncore/drawp.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>
#include <ioncore/resize.h>
#include "floatframe.h"
#include "funtabs.h"


/*{{{ Static function declarations, dynfuntab and class implementation */


static void deinit_floatframe(WFloatFrame *frame);
static void floatframe_draw(const WFloatFrame *frame, bool complete);
static void floatframe_recalc_bar(WFloatFrame *frame, bool draw);
static bool floatframe_save_to_file(WFloatFrame *frame, FILE *file, int lvl);
static void floatframe_border_geom(const WFloatFrame *frame, WRectangle *geom);
static void floatframe_managed_geom(const WFloatFrame *frame, WRectangle *geom);
static void floatframe_bar_geom(const WFloatFrame *frame, WRectangle *geom);
static void floatframe_border_inner_geom(const WFloatFrame *frame,
										 WRectangle *geom);
static void floatframe_size_changed(WFloatFrame *frame, bool wchg, bool hchg);
static void floatframe_set_shape(WFloatFrame *frame);
static void floatframe_request_managed_geom(WFloatFrame *frame, WRegion *sub,
											WRectangle geom,
											WRectangle *geomret,
											bool tryonly);

static DynFunTab floatframe_dynfuntab[]={
	{draw_window, floatframe_draw},
	{genframe_size_changed, floatframe_size_changed},
	{genframe_recalc_bar, floatframe_recalc_bar},

	{genframe_managed_geom, floatframe_managed_geom},
	{genframe_bar_geom, floatframe_bar_geom},
	{genframe_border_inner_geom, floatframe_border_inner_geom},
	{region_remove_managed, floatframe_remove_managed},
	
	{region_request_managed_geom, floatframe_request_managed_geom},
	
	/*{(DynFun*)region_save_to_file, (DynFun*)floatframe_save_to_file},*/
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WFloatFrame, WGenFrame, deinit_floatframe, floatframe_dynfuntab,
		&floatframe_funclist)

/*}}}*/
	
	
/*{{{ Destroy/create frame */


static bool init_floatframe(WFloatFrame *frame, WWindow *parent,
							WRectangle geom, int id)
{
	if(!init_genframe((WGenFrame*)frame, parent, geom, id))
		return FALSE;

	frame->bar_w=geom.w;
	frame->genframe.tab_spacing=0;
	
	genframe_recalc_bar((WGenFrame*)frame, FALSE);
	
	region_add_bindmap((WRegion*)frame, &(floatframe_bindmap));
	
	return TRUE;
}


WFloatFrame *create_floatframe(WWindow *parent, WRectangle geom, int id)
{
	CREATETHING_IMPL(WFloatFrame, floatframe, (p, parent, geom, id));
}


static void deinit_floatframe(WFloatFrame *frame)
{
	deinit_genframe((WGenFrame*)frame);
}


/*}}}*/
						  

/*{{{ Geometry */


static WRectangle floatframe_to_managed_geom(WGRData *grdata, WRectangle geom)
{
	geom.x=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad;
	geom.w-=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad);
	geom.y=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad
		+grdata->bar_h;
	geom.h-=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad)
		+grdata->bar_h;
	
	return geom;
}


WRectangle managed_to_floatframe_geom(WGRData *grdata, WRectangle geom)
{
	geom.x-=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad;
	geom.w+=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad);
	geom.y-=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad
		+grdata->bar_h;
	geom.h+=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad)
		+grdata->bar_h;
	
	return geom;
}


static void floatframe_border_geom(const WFloatFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);

	geom->x=0;
	geom->y=grdata->bar_h;
	geom->w=REGION_GEOM(frame).w;
	geom->h=REGION_GEOM(frame).h-grdata->bar_h;
}


static void floatframe_bar_geom(const WFloatFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	geom->x=0;
	geom->y=0;
	geom->w=frame->bar_w;
	geom->h=grdata->bar_h;
}


static void floatframe_managed_geom(const WFloatFrame *frame, WRectangle *geom)
{
	*geom=floatframe_to_managed_geom(GRDATA_OF(frame), REGION_GEOM(frame));
}


static void floatframe_border_inner_geom(const WFloatFrame *frame,
										 WRectangle *geom)
{
	*geom=floatframe_to_managed_geom(GRDATA_OF(frame), REGION_GEOM(frame));
}


WRectangle initial_to_floatframe_geom(WGRData *grdata, WRectangle geom,
									  int gravity)
{
	/* TODO: gravity */
	
	/*geom.x-=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad;*/
	geom.w+=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad);
	/*geom.y-=BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad
		+grdata->bar_h;*/
	geom.h+=2*(BORDER_TOTAL(&grdata->frame_border)-grdata->frame_border.ipad)
		+grdata->bar_h;
	
	return geom;
}


static void floatframe_request_managed_geom(WFloatFrame *frame, WRegion *sub,
											WRectangle geom,
											WRectangle *geomret,
											bool tryonly)
{
	WRectangle fgeom;
	fgeom=managed_to_floatframe_geom(GRDATA_OF(frame), geom);
	fgeom.x+=REGION_GEOM(frame).x;
	fgeom.y+=REGION_GEOM(frame).y;
	
	region_request_geom((WRegion*)frame, fgeom, &fgeom, tryonly);
	
	if(geomret!=NULL)
		*geomret=floatframe_to_managed_geom(GRDATA_OF(frame), fgeom);
}

	
/*}}}*/


/*{{{ Drawing routines and such */


static void floatframe_set_shape(WFloatFrame *frame)
{
	WRectangle g;
	XRectangle r[2];
	
	floatframe_bar_geom(frame, &g);
	r[0].x=g.x; r[0].y=g.y; r[0].width=g.w; r[0].height=g.h;
	r[1].width=REGION_GEOM(frame).w;
	r[1].height=REGION_GEOM(frame).h-g.h;
	r[1].x=0;
	r[1].y=g.h;
	XShapeCombineRectangles(wglobal.dpy, WGENFRAME_WIN(frame), ShapeBounding,
							0, 0, r, 2, ShapeSet, YXBanded);
}


#define CF_TAB_MAX_TEXT_X_OFF 10

static void floatframe_recalc_bar(WFloatFrame *frame, bool draw)
{
	WGRData *grdata=GRDATA_OF(frame);
	int bar_w=0, maxw=0, tmaxw=0;
	int tab_w, textw, tmp;
	WRegion *sub;
	char *p;
	
	if(frame->genframe.managed_count!=0){
		FOR_ALL_MANAGED_ON_LIST(frame->genframe.managed_list, sub){
			p=region_full_name(sub);
			if(p!=NULL){
				tab_w=(text_width(grdata->tab_font, p, strlen(p))
					   +BORDER_TL_TOTAL(&(grdata->tab_border))
					   +BORDER_BR_TOTAL(&(grdata->tab_border)));
				if(tab_w>tmaxw)
					tmaxw=tab_w;
				free(p);
			}
		}
		
		maxw=grdata->pwm_bar_max_width_q*REGION_GEOM(frame).w;
		if(maxw<grdata->pwm_tab_min_width &&
		   REGION_GEOM(frame).w>grdata->pwm_tab_min_width)
			maxw=grdata->pwm_tab_min_width;
		
		tmp=maxw-tmaxw*frame->genframe.managed_count;
		if(tmp>0){
			/* No label truncation needed. good. See how much can be padded */
			tmp/=frame->genframe.managed_count*2;
			if(tmp>CF_TAB_MAX_TEXT_X_OFF)
				tmp=CF_TAB_MAX_TEXT_X_OFF;
			tab_w=tmaxw+tmp*2;
			bar_w=tab_w*frame->genframe.managed_count;
		}else{
			/* Some labels must be truncated */
			bar_w=maxw;
		}
	}else{
		bar_w=grdata->pwm_tab_min_width;
		if(bar_w>grdata->pwm_bar_max_width_q*REGION_GEOM(frame).w)
			bar_w=grdata->pwm_bar_max_width_q*REGION_GEOM(frame).w;
	}

	if(frame->bar_w!=bar_w){
		frame->bar_w=bar_w;
		floatframe_set_shape(frame);
	}

	if(frame->genframe.managed_count!=0){
		int n=0;
		FOR_ALL_MANAGED_ON_LIST(frame->genframe.managed_list, sub){
			tab_w=genframe_nth_tab_w((WGenFrame*)frame, n++);
			textw=BORDER_IW(&(grdata->tab_border), tab_w);
			REGION_LABEL(sub)=region_make_label(sub, textw, grdata->tab_font);
		}
	}
	
	if(draw)
		genframe_draw_bar((WGenFrame*)frame, TRUE);
}


static void floatframe_size_changed(WFloatFrame *frame, bool wchg, bool hchg)
{
	int bar_w=frame->bar_w;
	
	if(wchg)
		floatframe_recalc_bar(frame, FALSE);
	if(hchg || (wchg && bar_w==frame->bar_w))
		floatframe_set_shape(frame);
}


static void floatframe_draw(const WFloatFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(frame);
	int a;

	dinfo->win=WGENFRAME_WIN(frame);
	dinfo->draw=WGENFRAME_DRAW(frame);

	dinfo->grdata=grdata;
	dinfo->gc=grdata->gc;
	dinfo->border=&(grdata->frame_border);
	floatframe_border_geom(frame, &(dinfo->geom));
	
	if(REGION_IS_ACTIVE(frame))
		dinfo->colors=&(grdata->act_frame_colors);
	else
		dinfo->colors=&(grdata->frame_colors);
	
	if(complete)
		XClearWindow(wglobal.dpy, WGENFRAME_WIN(frame));
	
	draw_border(dinfo);
	
	dinfo->geom.x+=BORDER->tl;
	dinfo->geom.y+=BORDER->tl;
	dinfo->geom.w-=BORDER->tl+BORDER->br;
	dinfo->geom.h-=BORDER->tl+BORDER->br;
	
	draw_border_inverted(dinfo, TRUE);

	genframe_draw_bar((WGenFrame*)frame, FALSE);
}


/*}}}*/


/*{{{ Add/remove */


void floatframe_remove_managed(WFloatFrame *frame, WRegion *reg)
{
	genframe_remove_managed((WGenFrame*)frame, reg);
	if(frame->genframe.managed_count==0)
		defer_destroy((WThing*)frame);
}


/*}}}*/


/*{{{ Raise/lower */


void floatframe_raise(WFloatFrame *frame)
{
	region_restack((WRegion*)frame, None, Above);
}


void floatframe_lower(WFloatFrame *frame)
{
	region_restack((WRegion*)frame, None, Below);
}


/*}}}*/


#if 0

/*{{{ Save/load */


static bool floatframe_save_to_file(WFloatFrame *floatframe, FILE *file,
									int lvl)
{
	begin_saved_region((WRegion*)floatframe, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "target_id %d\n", floatframe->genframe.target_id);
	end_saved_region((WRegion*)floatframe, file, lvl);
	return TRUE;
}


static int load_target_id;


static bool opt_target_id(Tokenizer *tokz, int n, Token *toks)
{
	load_target_id=TOK_LONG_VAL(&(toks[1]));
	return TRUE;
}


static ConfOpt floatframe_opts[]={
	{"target_id", "l", opt_target_id, NULL},
	END_CONFOPTS
};


WRegion *floatframe_load(WRegion *par, WRectangle geom, Tokenizer *tokz)
{
	load_target_id=0;
	
	if(!parse_config_tokz(tokz, floatframe_opts))
		return NULL;
	
	return (WRegion*)create_floatframe(par, geom, load_target_id);
}


/*}}}*/

#endif


