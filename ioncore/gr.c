/*
 * ion/ioncore/gr.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include "common.h"
#include "global.h"
#include "hooks.h"
#include "modules.h"
#include "readconfig.h"
#include "gr.h"


/*{{{ Lookup and registration */

INTRSTRUCT(GrEngine);

DECLSTRUCT(GrEngine){
	char *name;
	GrGetBrushFn *fn;
	GrGetValuesFn *vfn;
	GrEngine *next, *prev;
};


static GrEngine *engines=NULL, *current_engine=NULL;


bool gr_register_engine(const char *engine, GrGetBrushFn *fn,
						GrGetValuesFn *vfn)
{
	GrEngine *eng;
	
	if(engine==NULL || fn==NULL || vfn==NULL)
		return FALSE;
	
	eng=ALLOC(GrEngine);
	
	if(eng==NULL){
		warn_err();
		return FALSE;
	}
	
	eng->name=scopy(engine);
	
	if(eng->name==NULL){
		warn_err();
		free(eng);
		return FALSE;
	}
	
	eng->fn=fn;
	eng->vfn=vfn;
	
	LINK_ITEM(engines, eng, next, prev);

	return TRUE;
}


void gr_unregister_engine(const char *engine)
{
	GrEngine *eng;
	
	for(eng=engines; eng!=NULL; eng=eng->next){
		if(strcmp(eng->name, engine)==0)
			break;
	}
	
	if(eng==NULL)
		return;
	
	UNLINK_ITEM(engines, eng, next, prev);
	free(eng->name);
	if(current_engine==eng)
		current_engine=NULL;
	free(eng);
}


static bool gr_do_select_engine(const char *engine)
{
	GrEngine *eng;

	for(eng=engines; eng!=NULL; eng=eng->next){
		if(strcmp(eng->name, engine)==0){
			current_engine=eng;
			return TRUE;
		}
	}
	
	return FALSE;
}


/*EXTL_DOC
 * Future requests for ''brushes'' are to be forwarded to the drawing engine
 * \var{engine}. If no engine of such name is known, a module with that name
 * is attempted to be loaded. This function is only intended to be called from
 * colour scheme etc. configuration files and can not be used to change the
 * look of existing objects; for that use \fnref{reread_draw_config}.
 */
EXTL_EXPORT
bool gr_select_engine(const char *engine)
{
	if(engine==NULL)
		return FALSE;
	
	if(gr_do_select_engine(engine))
		return TRUE;
	
	if(!load_module(engine))
		return FALSE;
	
	if(!gr_do_select_engine(engine)){
		warn("Drawing engine %s not registered!", engine);
		return FALSE;
	}
	
	return TRUE;
}


GrBrush *gr_get_brush(WRootWin *rootwin, Window win, const char *style)
{
	GrEngine *eng=(current_engine!=NULL ? current_engine : engines);
	GrBrush *ret;
	
	if(eng==NULL || eng->fn==NULL)
		return NULL;
	
	ret=(eng->fn)(rootwin, win, style);
	
	if(ret==NULL)
		warn("Unable to find brush for '%s'\n", style);
		
	return ret;
}


bool gr_get_brush_values(WRootWin *rootwin, const char *style,
						 GrBorderWidths *bdw, GrFontExtents *fnte,
						 ExtlTab *tab)
{
	GrEngine *eng=(current_engine!=NULL ? current_engine : engines);
	
	if(eng==NULL || eng->fn==NULL)
		return FALSE;
	
	return (eng->vfn)(rootwin, style, bdw, fnte, tab);
}


/*}}}*/


/*{{{ Scoring */


uint gr_stylespec_score2(const char *spec, const char *attrib, 
						 const char *attrib_p2)
{
	uint score=0;
	uint a=0;
	uint mult=1;

	if(attrib==NULL){
		if(spec==NULL || strcmp(spec, "*")==0)
			return 1;
		return 0;
	}
	
	while(1){
		if(*spec=='*'){
			score=score+mult;
			spec++;
			attrib=strchr(attrib, '-');
		}else{
			while(1){
				if(*attrib=='\0'){
					attrib=NULL;
					break;
				}
				if(*attrib=='-')
					break;
				if(*spec!=*attrib)
					return 0;
				attrib++;
				spec++;
			}
			score=score+2*mult;
		}
		
		if(*spec=='\0')
			return score;
		else if(*spec!='-')
			return 0;
		
		if(attrib==NULL){
			if(a==0 && attrib_p2!=NULL){
				attrib=attrib_p2;
				a++;
			}else{
				return 0;
			}
		}else{
			attrib++;
		}

		spec++;
		mult=mult*3;
	}
}


uint gr_stylespec_score(const char *spec, const char *attrib)
{
	return gr_stylespec_score2(spec, attrib, NULL);
}


/*}}}*/


/*{{{ Init, deinit */


bool grbrush_init(GrBrush *brush)
{
	return TRUE;
}


void grbrush_deinit(GrBrush *brush)
{
}


void grbrush_release(GrBrush *brush, Window win)
{
	CALL_DYN(grbrush_release, brush, (brush, win));
}


GrBrush *grbrush_get_slave(GrBrush *brush, WRootWin *rootwin, 
						   Window win, const char *style)
{
	GrBrush *slave=NULL;
	CALL_DYN_RET(slave, GrBrush*, grbrush_get_slave, brush,
				 (brush, rootwin, win, style));
	return slave;
}


/*}}}*/


/*{{{ Dynfuns/values */


void grbrush_get_font_extents(GrBrush *brush, GrFontExtents *fnte)
{
	CALL_DYN(grbrush_get_font_extents, brush, (brush, fnte));
}


void grbrush_get_border_widths(GrBrush *brush, GrBorderWidths *bdw)
{
	CALL_DYN(grbrush_get_border_widths, brush, (brush, bdw));
}


void grbrush_get_extra_values(GrBrush *brush, ExtlTab *tab)
{
	*tab=extl_table_none();
	{
		CALL_DYN(grbrush_get_extra_values, brush, (brush, tab));
	}
}


/*}}}*/


/*{{{ Dynfuns/Borders */


void grbrush_draw_border(GrBrush *brush, Window win, 
						 const WRectangle *geom,
						 const char *attrib)
{
	CALL_DYN(grbrush_draw_border, brush, (brush, win, geom, attrib));
}


/*}}}*/


/*{{{ Dynfuns/Strings */


void grbrush_draw_string(GrBrush *brush, Window win, int x, int y,
						 const char *str, int len, bool needfill,
						 const char *attrib)
{
	CALL_DYN(grbrush_draw_string, brush, 
			 (brush, win, x, y, str, len, needfill, attrib));
}


uint grbrush_get_text_width(GrBrush *brush, const char *text, uint len)
{
	uint ret=0;
	CALL_DYN_RET(ret, uint, grbrush_get_text_width, brush, 
				 (brush, text, len));
	return ret;
}


/*}}}*/


/*{{{ Dynfuns/Textboxes */


void grbrush_draw_textbox(GrBrush *brush, Window win, 
						  const WRectangle *geom,
						  const char *text, 
						  const char *attr,
						  bool needfill)
{
	CALL_DYN(grbrush_draw_textbox, brush, 
			 (brush, win, geom, text, attr, needfill));
}

void grbrush_draw_textboxes(GrBrush *brush, Window win, 
							const WRectangle *geom,
							int n, const GrTextElem *elem, 
							bool needfill, const char *common_attrib)
{
	CALL_DYN(grbrush_draw_textboxes, brush, 
			 (brush, win, geom, n, elem, needfill, common_attrib));
}


/*}}}*/


/*{{{ Dynfuns/Misc */


void grbrush_set_clipping_rectangle(GrBrush *brush, Window win,
									const WRectangle *geom)
{
	CALL_DYN(grbrush_set_clipping_rectangle, brush, (brush, win, geom));
}


void grbrush_clear_clipping_rectangle(GrBrush *brush, Window win)
{
	CALL_DYN(grbrush_clear_clipping_rectangle, brush, (brush, win));
}


void grbrush_set_window_shape(GrBrush *brush, Window win,bool rough,
							  int n, const WRectangle *rects)
{
	CALL_DYN(grbrush_set_window_shape, brush, (brush, win, rough, n, rects));
}


void grbrush_enable_transparency(GrBrush *brush, Window win, 
								 GrTransparency tr)
{
	CALL_DYN(grbrush_enable_transparency, brush, (brush, win, tr));
}


void grbrush_fill_area(GrBrush *brush, Window win, const WRectangle *geom,
					   const char *attr)
{
	CALL_DYN(grbrush_fill_area, brush, (brush, win, geom, attr));
}


void grbrush_clear_area(GrBrush *brush, Window win, const WRectangle *geom)
{
	CALL_DYN(grbrush_clear_area, brush, (brush, win, geom));
}


/*}}}*/


/*{{{ read_config/refresh */


/*EXTL_DOC
 * Read drawing engine configuration file \file{draw.lua}.
 */
EXTL_EXPORT
void gr_read_config()
{
	read_config_for("draw");
	
	/* If nothing has been loaded, try the default engine with
	 * default settings.
	 */
	if(engines==NULL){
		warn("No drawing engines loaded, trying \"de\".");
		gr_select_engine("de");
	}
}


/*EXTL_DOC
 * Refresh objects' brushes to update them to use newly loaded style.
 */
EXTL_EXPORT
void gr_refresh()
{
	WRootWin *rootwin;
	
	FOR_ALL_ROOTWINS(rootwin){
		region_draw_config_updated((WRegion*)rootwin);
	}
}


/*}}}*/


/*{{{ Class implementation */


static DynFunTab grbrush_dynfuntab[]={
	END_DYNFUNTAB
};
									   

IMPLOBJ(GrBrush, WObj, grbrush_deinit, grbrush_dynfuntab);


/*}}}*/

