/*
 * ion/ioncore/infowin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "window.h"
#include "infowin.h"
#include "gr.h"


/*{{{ Init/deinit */


bool infowin_init(WInfoWin* p, WWindow *parent, const WRectangle *geom,
				  const char *style)
{
	XSetWindowAttributes attr;
	
	if(!window_init_new(&(p->wwin), parent, geom))
		return FALSE;
	
	p->buffer=ALLOC_N(char, WINFOWIN_BUFFER_LEN);
	if(p->buffer==NULL)
		goto fail;
	p->buffer[0]='\0';
	p->attr=NULL;

	p->brush=gr_get_brush(ROOTWIN_OF(parent), p->wwin.win, style);
	if(p->brush==NULL)
		goto fail2;
	
	/* Enable save unders */
	attr.save_under=True;
	XChangeWindowAttributes(wglobal.dpy, p->wwin.win,
							CWSaveUnder, &attr);
	XSelectInput(wglobal.dpy, p->wwin.win, ExposureMask);
	
	return TRUE;

fail2:	
	free(p->buffer);
fail:	
	window_deinit(&(p->wwin));
	return FALSE;
}


WInfoWin *create_infowin(WWindow *parent, const WRectangle *geom,
						 const char *style)
{
	CREATEOBJ_IMPL(WInfoWin, infowin, (p, parent, geom, style));
}


void infowin_deinit(WInfoWin *p)
{
	if(p->buffer!=NULL)
		free(p->buffer);
	if(p->attr!=NULL)
		free(p->attr);
	if(p->brush!=NULL)
		grbrush_release(p->brush, p->wwin.win);
	
	window_deinit(&(p->wwin));
}


/*}}}*/


/*{{{ Content-setting */


bool infowin_set_attr2(WInfoWin *p, const char *attr1, const char *attr2)
{
	char *p2=NULL;
	
	if(attr1!=NULL){
		if(attr2==NULL)
			p2=scopy(attr1);
		else
			libtu_asprintf(&p2, "%s-%s", attr1, attr2);
		if(p2==NULL)
			return FALSE;
	}
	
	if(p->attr)
		free(p->attr);
	
	p->attr=p2;
	
	return TRUE;
}


/*}}}*/


/*{{{ Drawing */


void infowin_draw(WInfoWin *p, bool complete)
{
	WRectangle g;
	
	if(p->brush==NULL)
		return;
	
	g.x=0;
	g.y=0;
	g.w=REGION_GEOM(p).w;
	g.h=REGION_GEOM(p).h;
	
	grbrush_draw_textbox(p->brush, p->wwin.win, &g, p->buffer, p->attr,
						 complete);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab infowin_dynfuntab[]={
	{window_draw, infowin_draw},
	END_DYNFUNTAB
};


IMPLOBJ(WInfoWin, WWindow, infowin_deinit, infowin_dynfuntab);

	
/*}}}*/

