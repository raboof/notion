/*
 * ion/ionws/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <libtu/parser.h>
#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/genframep.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include <ioncore/objp.h>
#include <ioncore/thingp.h>
#include <ioncore/drawp.h>
#include <ioncore/resize.h>
#include <ioncore/targetid.h>
#include "ionframe.h"

#include "bindmaps.h"
#include "splitframe.h"
#include "funtabs.h"


/*{{{ Static function declarations, dynfuntab and class implementation */


static void deinit_ionframe(WIonFrame *frame);
static void ionframe_recalc_bar(WIonFrame *frame, bool draw);
static bool ionframe_save_to_file(WIonFrame *frame, FILE *file, int lvl);
static void ionframe_border_geom(const WIonFrame *frame, WRectangle *geom);
static void ionframe_managed_geom(const WIonFrame *frame, WRectangle *geom);
static void ionframe_bar_geom(const WIonFrame *frame, WRectangle *geom);
static void ionframe_border_inner_geom(const WIonFrame *frame, WRectangle *geom);
static void ionframe_resize_hints(WIonFrame *frame, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret);

static void set_tab_spacing(WIonFrame *frame);


static DynFunTab ionframe_dynfuntab[]={
	{draw_window, ionframe_draw},
	{genframe_draw_bar, ionframe_draw_bar},
	{genframe_recalc_bar, ionframe_recalc_bar},
	
	{genframe_managed_geom, ionframe_managed_geom},
	{genframe_bar_geom, ionframe_bar_geom},
	{genframe_border_inner_geom, ionframe_border_inner_geom},
	{region_resize_hints, ionframe_resize_hints},

	{region_draw_config_updated, ionframe_draw_config_updated},
		
	{(DynFun*)region_save_to_file, (DynFun*)ionframe_save_to_file},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WIonFrame, WGenFrame, deinit_ionframe, ionframe_dynfuntab,
		&ionframe_funclist)

	
/*}}}*/
	
	
/*{{{ Destroy/create frame */


static bool init_ionframe(WIonFrame *frame, WWindow *parent, WRectangle geom,
						  int id, int flags)
{
	if(!init_genframe((WGenFrame*)frame, parent, geom, id))
		return FALSE;
	
	frame->genframe.flags|=flags;
	set_tab_spacing(frame);
	
	region_add_bindmap((WRegion*)frame, &(ionframe_bindmap));
	
	return TRUE;
}


WIonFrame *create_ionframe(WWindow *parent, WRectangle geom, int id, int flags)
{
	CREATETHING_IMPL(WIonFrame, ionframe, (p, parent, geom, id, flags));
}


WIonFrame* create_ionframe_simple(WWindow *parent, WRectangle geom)
{
	return create_ionframe(parent, geom, 0, 0);
}


static void deinit_ionframe(WIonFrame *frame)
{
	deinit_genframe((WGenFrame*)frame);
}


/*}}}*/
						  

/*{{{ Geometry */


static WBorder frame_border(WGRData *grdata)
{
	WBorder bd=grdata->frame_border;
	bd.ipad/=2;
	return bd;
}


static void ionframe_border_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);

	geom->x=0;
	geom->y=0;
	geom->w=REGION_GEOM(frame).w;
	geom->h=REGION_GEOM(frame).h;
	
	if(!grdata->bar_inside_frame && !(frame->genframe.flags&WGENFRAME_TAB_HIDE)){
		WBorder bd=frame_border(grdata);
		geom->y+=grdata->bar_h+bd.ipad;
		geom->h-=grdata->bar_h+bd.ipad;
	}
}


static void ionframe_bar_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	if(grdata->bar_inside_frame){
		ionframe_border_geom(frame, geom);
		geom->x+=BORDER_TL_TOTAL(&bd)+grdata->spacing;
		geom->y+=BORDER_TL_TOTAL(&bd)+grdata->spacing;
		geom->w-=BORDER_TOTAL(&bd)+2*grdata->spacing;
	}else{
		geom->x=bd.ipad;
		geom->y=bd.ipad;
		geom->w=REGION_GEOM(frame).w-2*bd.ipad;
	}

	geom->h=(frame->genframe.flags&WGENFRAME_TAB_HIDE ? 0 : grdata->bar_h);
}


static void ionframe_managed_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	ionframe_border_inner_geom(frame, geom);
	geom->x+=grdata->spacing;
	geom->y+=grdata->spacing;
	geom->w-=2*grdata->spacing;
	geom->h-=2*grdata->spacing;
	
	if(grdata->bar_inside_frame && !(frame->genframe.flags&WGENFRAME_TAB_HIDE)){
		geom->y+=grdata->bar_h+grdata->spacing;
		geom->h-=grdata->bar_h+grdata->spacing;
	}
}


static void ionframe_border_inner_geom(const WIonFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	ionframe_border_geom(frame, geom);

	geom->x+=BORDER_TL_TOTAL(&bd);
	geom->y+=BORDER_TL_TOTAL(&bd);
	geom->w-=BORDER_TOTAL(&bd);
	geom->h-=BORDER_TOTAL(&bd);
}


static void set_tab_spacing(WIonFrame *frame)
{
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	if(!grdata->bar_inside_frame)
		frame->genframe.tab_spacing=bd.ipad;
	else
		frame->genframe.tab_spacing=grdata->spacing;
}


static void ionframe_resize_hints(WIonFrame *frame, XSizeHints *hints_ret,
								  uint *relw_ret, uint *relh_ret)
{
	WScreen *scr=SCREEN_OF(frame);
	
	genframe_resize_hints((WGenFrame*)frame, hints_ret, relw_ret, relh_ret);
	
	if(!(hints_ret->flags&PResizeInc)){
		hints_ret->flags|=PResizeInc;
		hints_ret->width_inc=scr->w_unit;
		hints_ret->height_inc=scr->h_unit;
	}
	
	hints_ret->flags|=PMinSize;
	hints_ret->min_width=1;
	hints_ret->min_height=1;
}


/*}}}*/


/*{{{ Drawing routines and such */


static void ionframe_recalc_bar(WIonFrame *frame, bool draw)
{
	WScreen *scr=SCREEN_OF(frame);
	int bar_w, tab_w, textw, n;
	WRegion *sub;

	{
		WRectangle geom;
		genframe_bar_geom((WGenFrame*)frame, &geom);
		bar_w=geom.w;
	}
	
	if(frame->genframe.managed_count!=0){
		n=0;
		FOR_ALL_MANAGED_ON_LIST(frame->genframe.managed_list, sub){
			tab_w=genframe_nth_tab_w((WGenFrame*)frame, n++);
			textw=BORDER_IW(&(scr->grdata.tab_border), tab_w);
			REGION_LABEL(sub)=region_make_label(sub, textw,
												scr->grdata.tab_font);
		}
	}
	
	if(draw)
		genframe_draw_bar((WGenFrame*)frame, TRUE);
}



void ionframe_draw(const WIonFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(frame);
	WBorder bd=frame_border(grdata);
	
	dinfo->win=WGENFRAME_WIN(frame);
	dinfo->draw=WGENFRAME_DRAW(frame);
	dinfo->grdata=grdata;
	dinfo->gc=grdata->gc;
	dinfo->border=&bd;
	
	ionframe_border_geom(frame, &(dinfo->geom));
	
	if(REGION_IS_ACTIVE(frame))
		dinfo->colors=&(grdata->act_frame_colors);
	else
		dinfo->colors=&(grdata->frame_colors);
	
	if(complete)
		XClearWindow(wglobal.dpy, WGENFRAME_WIN(frame));
	
	draw_border_inverted(dinfo, TRUE);

	genframe_draw_bar((WGenFrame*)frame,
					  !complete || !grdata->bar_inside_frame);
}


void ionframe_draw_bar(const WIonFrame *frame, bool complete)
{
	WGRData *grdata=GRDATA_OF(frame);

	if(frame->genframe.flags&WGENFRAME_TAB_HIDE)
		return;
	
	if(complete && !grdata->bar_inside_frame){
		WBorder bd=frame_border(grdata);

		if(REGION_IS_ACTIVE(frame)){
			set_foreground(wglobal.dpy, grdata->tab_gc,
						   grdata->act_frame_colors.bg);
		}else{
			set_foreground(wglobal.dpy, grdata->tab_gc,
						   grdata->frame_colors.bg);
		}
		
		XFillRectangle(wglobal.dpy, WGENFRAME_WIN(frame), grdata->tab_gc,
					   0, 0, REGION_GEOM(frame).w, grdata->bar_h+bd.ipad);
		complete=FALSE;
	}
	
	genframe_draw_bar_default((WGenFrame*)frame, complete);
}


void ionframe_draw_config_updated(WIonFrame *frame)
{
	set_tab_spacing(frame);
	genframe_draw_config_updated((WGenFrame*)frame);
}


/*}}}*/


/*{{{ Save/load */


static bool ionframe_save_to_file(WIonFrame *ionframe, FILE *file, int lvl)
{
	WRegion *sub;
	
	begin_saved_region((WRegion*)ionframe, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "target_id %d\n", ionframe->genframe.target_id);
	save_indent_line(file, lvl);
	fprintf(file, "flags %x\n", ionframe->genframe.flags);
	
	FOR_ALL_MANAGED_ON_LIST(ionframe->genframe.managed_list, sub){
		region_save_to_file((WRegion*)sub, file, lvl+1);
	}
	
	end_saved_region((WRegion*)ionframe, file, lvl);
	
	return TRUE;
}


static WIonFrame *tmp_frame;
static int tmp_flags=0;
static int tmp_target_id=0;
static WWindow *tmp_par=NULL;
static WRectangle tmp_geom;


static bool opt_target_id(Tokenizer *tokz, int n, Token *toks)
{
	if(tmp_frame!=NULL){
		warn("Will not set target id after frame has been created.");
		return FALSE;
	}
			
	tmp_target_id=TOK_LONG_VAL(&(toks[1]));
	return TRUE;
}


static bool opt_frame_flags(Tokenizer *tokz, int n, Token *toks)
{
	if(tmp_frame!=NULL){
		warn("Will not set flags after frame has been created.");
		return FALSE;
	}
	tmp_flags=TOK_LONG_VAL(&(toks[1]));
	tmp_flags&=WGENFRAME_TAB_HIDE;
	return TRUE;
}


static bool opt_region(Tokenizer *tokz, int n, Token *toks)
{
	WRegion *reg;
	WRectangle geom;
	
	if(tmp_frame==NULL){
		tmp_frame=create_ionframe(tmp_par, tmp_geom, tmp_target_id, tmp_flags);
		if(tmp_frame==NULL){
			warn_err();
			return FALSE;
		}
	}
	
	pgeom("bar: ", REGION_GEOM(tmp_frame));
	pgeom("quk: ", tmp_geom);
	genframe_managed_geom((WGenFrame*)tmp_frame, &geom);
	pgeom("foo: ", geom);
	reg=load_create_region((WWindow*)tmp_frame, geom, tokz, n, toks);

	if(reg==NULL)
		return FALSE;

	region_add_managed_doit((WRegion*)tmp_frame, reg, 0);
	
	return TRUE;
}


static ConfOpt ionframe_opts[]={
	{"target_id",	"l", opt_target_id, 	NULL},
	{"flags",		"l", opt_frame_flags,	NULL},
	{"region", 		"s?s", opt_region, CONFOPTS_NOT_SET},
	END_CONFOPTS
};


WRegion *ionframe_load(WWindow *par, WRectangle geom, Tokenizer *tokz)
{
	tmp_target_id=TARGET_ID_ALLOCANY;
	tmp_flags=0;
	tmp_par=par;
	tmp_frame=NULL;
	tmp_geom=geom;
	
	if(!parse_config_tokz(tokz, ionframe_opts)){
		destroy_thing((WThing*)tmp_frame);
		return NULL;
	}

	if(tmp_frame==NULL)
		tmp_frame=create_ionframe(par, geom, tmp_target_id, tmp_flags);
	
	return (WRegion*)tmp_frame;
}


/*}}}*/
