/*
 * ion/query/input.c
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
#include <ioncore/window.h>
#include <ioncore/global.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>
#include <ioncore/strings.h>
#include "menu.h"
#include "menup.h"


/*{{{ Drawing routines */


static void get_outer_geom(WMenu *menu, WRectangle *geom)
{
	geom->x=0;
	geom->y=0;
	geom->w=REGION_GEOM(menu).w;
	geom->h=REGION_GEOM(menu).h;
}


static void get_inner_geom(WMenu *menu, WRectangle *geom)
{
	GrBorderWidths bdw;
	
	get_outer_geom(menu, geom);
	
	if(menu->brush!=NULL){
		grbrush_get_border_widths(menu->brush, &bdw);
		geom->x+=bdw.left;
		geom->y+=bdw.top;
		geom->w-=bdw.left+bdw.right;
		geom->h-=bdw.top+bdw.bottom;
	}
}


void menu_draw_entries(WMenu *menu, bool complete)
{
	WRectangle geom;
	uint i;
	
	if(menu->entry_brush==NULL)
		return;
	
	get_inner_geom(menu, &geom);
	geom.h=menu->entry_h;
	
	for(i=0; i<menu->n_entries; i++){
		/* TODO: submenus */
		const char *attr=(menu->selected_entry==i
						  ? "selected"
						  : "unselected");
		grbrush_draw_textbox(menu->entry_brush, MENU_WIN(menu), &geom,
							 menu->entry_titles[i], attr, complete);
		geom.y+=menu->entry_h+menu->entry_spacing;
	}
}


void menu_draw(WMenu *menu, bool complete)
{
	WRectangle geom;
	
	if(menu->brush==NULL)
		return;
	
	get_outer_geom(menu, &geom);
	
	grbrush_draw_border(menu->brush, MENU_WIN(menu), &geom, NULL);
	
	menu_draw_entries(menu, FALSE);
}


/*}}}*/


/*{{{ Dynfuns */


const char *menu_style(WMenu *menu)
{
	const char *ret="input-menu";
	CALL_DYN_RET(ret, const char*, menu_style, menu, (menu));
	return ret;
}


const char *menu_entry_style(WMenu *menu)
{
	const char *ret="input-menu-entry";
	CALL_DYN_RET(ret, const char*, menu_entry_style, menu, (menu));
	return ret;
}


/*}}}*/


/*{{{ Resize */


static void menu_calc_size(WMenu *menu, int maxw, int maxh, 
						   int *w_ret, int *h_ret)
{
	GrBorderWidths bdw, e_bdw;
	ExtlTab entry;
	char *str;
	uint i;
	int nath, maxew=menu->max_entry_w;
	
	grbrush_get_border_widths(menu->brush, &bdw);
	grbrush_get_border_widths(menu->entry_brush, &e_bdw);
	
	if(maxew>maxw-(int)bdw.left-(int)bdw.right){
		maxew=maxw-bdw.left-bdw.right;
		*w_ret=maxw;
	}else{
		*w_ret=maxew+bdw.left+bdw.right;
	}
	
	nath=bdw.top+bdw.bottom+(menu->n_entries*menu->entry_h);
	if(menu->n_entries>0)
		nath+=(menu->n_entries-1)*e_bdw.spacing;
	
	if(nath>maxh)
		*h_ret=maxh;
	else
		*h_ret=nath;
	
	/* Calculate new shortened entry names */
	maxew-=e_bdw.left+e_bdw.right;
#if 0
	if(menu->title!=NULL){
		free(menu->title);
		menu->title=NULL;
	}
	
	if(extl_table_gets_s(tab, title, &str)){
		menu->title=make_label(menu->title_brush, str, maxew);
		free(str);
	}
#endif
	
	for(i=0; i<menu->n_entries; i++){
		if(menu->entry_titles[i]){
			free(menu->entry_titles[i]);
			menu->entry_titles[i]=NULL;
		}
		if(maxew<=0)
			continue;
		
		if(extl_table_geti_t(menu->tab, i+1, &entry)){
			if(extl_table_gets_s(entry, "name", &str)){
				menu->entry_titles[i]=make_label(menu->entry_brush, str, 
												 maxew);
				free(str);
			}
		}
	}
}


void menu_refit(WMenu *menu)
{
	WRectangle geom;
	menu_calc_size(menu, menu->max_geom.w, menu->max_geom.h, 
				   &(geom.w), &(geom.h));
	
	geom.x=menu->max_geom.x;
	geom.y=menu->max_geom.y+menu->max_geom.h-geom.h;
	
	window_fit(&menu->win, &geom);
}


void menu_fit(WMenu *menu, const WRectangle *geom)
{
	menu->max_geom=*geom;
	menu_refit(menu);
}


/*}}}*/


/*{{{ Brush update */


static void calc_entry_dimens(WMenu *menu)
{
	int i, n=extl_table_get_n(menu->tab);
	GrFontExtents fnte;
	GrBorderWidths bdw;
	ExtlTab entry, sub;
	int maxw=0;
	char *str;
	
#if 0	
	if(extl_table_gets_s(menu->tab, title, &str)){
		maxw=grbrush_get_text_width(title_brush, str, strlen(str));
		free(str);
	}
#endif
	
	for(i=1; i<=n; i++){
		if(extl_table_geti_t(menu->tab, i, &entry)){
			if(extl_table_gets_s(entry, "name", &str)){
				int w=grbrush_get_text_width(menu->entry_brush, 
											 str, strlen(str));
				if(w>maxw)
					maxw=w;
				free(str);
			}
			extl_unref_table(entry);
		}
	}
	
	grbrush_get_border_widths(menu->entry_brush, &bdw);
	grbrush_get_font_extents(menu->entry_brush, &fnte);
	
	menu->max_entry_w=maxw+bdw.left+bdw.right;
	menu->entry_h=fnte.max_height+bdw.top+bdw.bottom;
	menu->entry_spacing=bdw.spacing;
}


static bool menu_init_gr(WMenu *menu, WRootWin *rootwin, Window win)
{
	GrBrush *brush, *entry_brush;
	
	brush=gr_get_brush(rootwin, win, menu_style(menu));
	
	if(brush==NULL)
		return FALSE;

	entry_brush=grbrush_get_slave(brush, rootwin, win,
								  menu_entry_style(menu));
	
	if(entry_brush==NULL){
		grbrush_release(brush, win);
		return FALSE;
	}
	
	if(menu->entry_brush!=NULL)
		grbrush_release(menu->entry_brush, win);
	if(menu->brush!=NULL)
		grbrush_release(menu->brush, win);

	menu->brush=brush;
	menu->entry_brush=entry_brush;
	
	calc_entry_dimens(menu);
	
	return TRUE;
}


void menu_draw_config_updated(WMenu *menu)
{
	if(!menu_init_gr(menu, ROOTWIN_OF(menu), MENU_WIN(menu)))
		return;
	
	menu_refit(menu);
	
	region_default_draw_config_updated((WRegion*)menu);
	
	window_draw((WWindow*)menu, TRUE);
}


static void menu_release_gr(WMenu *menu, Window win)
{
	if(menu->brush!=NULL)
		grbrush_release(menu->brush, win);
	if(menu->entry_brush!=NULL)
		grbrush_release(menu->entry_brush, win);
}


/*}}}*/


/*{{{ Init/deinit */


static void preprocess_menus(ExtlTab tab, bool *submenus, uint *n_entries)
{
	int i, n=extl_table_get_n(tab);
	ExtlTab entry, sub;
	
	*submenus=FALSE;
	*n_entries=n;
	
	/* Check submenus */
	for(i=1; i<=n && !(*submenus); i++){
		if(extl_table_geti_t(tab, i, &entry)){
			if(extl_table_gets_t(entry, "submenu", &sub)){
				*submenus=TRUE;
				extl_unref_table(sub);
			}
			extl_unref_table(entry);
		}
	}
}



bool menu_init(WMenu *menu, WWindow *par, const WRectangle *geom,
			   const WMenuCreateParams *params)
{
	Window win;
	uint i;
	bool submenus;

	preprocess_menus(params->tab, &(submenus), &(menu->n_entries));
	
	if(menu->n_entries==0){
		warn("Empty menu");
		return FALSE;
	}

	menu->entry_titles=ALLOC_N(char*, menu->n_entries);  
		
	if(menu->entry_titles==NULL){
		warn_err();
		return FALSE;
	}

	for(i=0; i<menu->n_entries; i++)
		menu->entry_titles[i]=NULL;

	menu->tab=extl_ref_table(params->tab);
	menu->handler=extl_ref_fn(params->handler);

	menu->max_geom=*geom;
	menu->selected_entry=0;
	menu->max_entry_w=0;
	menu->entry_h=0;
	menu->brush=NULL;
	menu->entry_brush=NULL;
	menu->entry_spacing=0;
	
	win=create_simple_window(ROOTWIN_OF(par), par->win, geom);
	
	if(!menu_init_gr(menu, ROOTWIN_OF(par), win))
		goto fail;

	if(!window_init((WWindow*)menu, par, win, geom)){
		menu_release_gr(menu, win);
		goto fail;
	}

	menu_refit(menu);
	
	XSelectInput(wglobal.dpy, win, MENU_MASK);
	region_add_bindmap((WRegion*)menu, &menu_bindmap);
	
	return TRUE;

fail:
	XDestroyWindow(wglobal.dpy, win);
	extl_unref_table(menu->tab);
	extl_unref_fn(menu->handler);
	free(menu->entry_titles);
	return FALSE;
}


WMenu *create_menu(WWindow *par, const WRectangle *geom, 
				   const WMenuCreateParams *params)
{
	CREATEOBJ_IMPL(WMenu, menu, (p, par, geom, params));
}



void menu_deinit(WMenu *menu)
{
	uint i;
	
	extl_unref_table(menu->tab);
	extl_unref_fn(menu->handler);
	
	for(i=0; i<menu->n_entries; i++)
		free(menu->entry_titles[i]);
	free(menu->entry_titles);
	
	menu_release_gr(menu, MENU_WIN(menu));
	window_deinit((WWindow*)menu);
}


/*}}}*/


/*{{{ Exports */


/*EXTL_DOC
 * Select previous entry in menu.
 */
EXTL_EXPORT_MEMBER
void menu_select_prev(WMenu *menu)
{
	menu->selected_entry=(menu->selected_entry == 0 
						  ? menu->n_entries-1
						  : menu->selected_entry-1);
	menu_draw_entries(menu, TRUE);
}


/*EXTL_DOC
 * Select next entry in menu.
 */
EXTL_EXPORT_MEMBER
void menu_select_next(WMenu *menu)
{
	menu->selected_entry=(menu->selected_entry+1)%menu->n_entries;
	menu_draw_entries(menu, TRUE);
}


/*EXTL_DOC
 * Select \var{n}:th entry in menu.
 */
EXTL_EXPORT_MEMBER
void menu_select_nth(WMenu *menu, int n)
{
	menu->selected_entry=((uint)n)%menu->n_entries;
	menu_draw_entries(menu, TRUE);
}


static void menu_do_finish(WMenu *menu)
{
	ExtlFn handler;
	ExtlTab tab;
	bool ok;
	
	handler=menu->handler;
	menu->handler=extl_fn_none();
	
	ok=extl_table_geti_t(menu->tab, menu->selected_entry+1, &tab);
	
	destroy_obj((WObj*)menu);
	
	if(ok)
		extl_call(handler, "t", NULL, tab);

	extl_unref_fn(handler);
	extl_unref_table(tab);
}


/*EXTL_DOC
 * Destroy the menu and call handler for selected entry.
 */
EXTL_EXPORT_MEMBER
void menu_finish(WMenu *menu)
{
	defer_action((WObj*)menu, (DeferredAction*)menu_do_finish);
}



/*EXTL_DOC
 * Close \var{menu} not calling any possible finish handlers.
 */
EXTL_EXPORT_MEMBER
void menu_cancel(WMenu *menu)
{
	defer_destroy((WObj*)menu);
}


/*EXTL_DOC
 * Same as \fnref{WMenu.cancel}.
 */
EXTL_EXPORT_MEMBER
void menu_close(WMenu *menu)
{
	menu_cancel(menu);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab menu_dynfuntab[]={
	{region_fit, menu_fit},
	{region_draw_config_updated, menu_draw_config_updated},
	{region_close, menu_close},
	{window_draw, menu_draw},
	END_DYNFUNTAB
};


IMPLOBJ(WMenu, WWindow, menu_deinit, menu_dynfuntab);

	
/*}}}*/

