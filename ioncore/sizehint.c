/*
 * ion/ioncore/sizehint.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "global.h"


/*{{{ correct_size */


static void do_correct_aspect(int max_w, int max_h, int ax, int ay,
							  int *wret, int *hret)
{
	int w=*wret, h=*hret;

	if(ax>ay){
		h=(w*ay)/ax;
		if(max_h>0 && h>max_h){
			h=max_h;
			w=(h*ax)/ay;
		}
	}else{
		w=(h*ax)/ay;
		if(max_w>0 && w>max_w){
			w=max_w;
			h=(w*ay)/ax;
		}
	}
	
	*wret=w;
	*hret=h;
}


static void correct_aspect(int max_w, int max_h, const XSizeHints *hints,
						   int *wret, int *hret)
{
	if(!(hints->flags&PAspect))
		return;
	
	if(*wret*hints->max_aspect.y>*hret*hints->max_aspect.x){
		do_correct_aspect(max_w, max_h,
						  hints->min_aspect.x, hints->min_aspect.y,
						  wret, hret);
	}

	if(*wret*hints->min_aspect.y<*hret*hints->min_aspect.x){
		do_correct_aspect(max_w, max_h,
						  hints->max_aspect.x, hints->max_aspect.y,
						  wret, hret);
	}
}


void correct_size(int *wp, int *hp, const XSizeHints *hints, bool min)
{
	int w=*wp;
	int h=*hp;
	int bs=0;
	
	if(min){
		if(w<hints->min_width)
			w=hints->min_width;
		if(h<hints->min_height)
			h=hints->min_height;
		
		correct_aspect(w, h, hints, &w, &h);
	}else{
		if(w>=hints->min_width && h>=hints->min_height)
			correct_aspect(w, h, hints, &w, &h);
	}
	
	if(hints->flags&PMaxSize){
		if(w>hints->max_width)
			w=hints->max_width;
		if(h>hints->max_height)
			h=hints->max_height;
	}

	if(hints->flags&PResizeInc){
		/* base size should be set to 0 if none given by user program */
		bs=(hints->flags&PBaseSize ? hints->base_width : 0);
		if(w>bs)
			w=((w-bs)/hints->width_inc)*hints->width_inc+bs;
		bs=(hints->flags&PBaseSize ? hints->base_height : 0);
		if(h>bs)
			h=((h-bs)/hints->height_inc)*hints->height_inc+bs;
	}
	
	*wp=w;
	*hp=h;
}


/*}}}*/


/*{{{ get_sizehints */

#define CWIN_MIN_W 1
#define CWIN_MIN_H 1

void get_sizehints(WScreen *scr, Window win, XSizeHints *hints)
{
	int minh, minw;
	long supplied=0;
	
	memset(hints, 0, sizeof(*hints));
	XGetWMNormalHints(wglobal.dpy, win, hints, &supplied);

	if(!(hints->flags&PMinSize) || hints->min_width<CWIN_MIN_W)
		hints->min_width=CWIN_MIN_W;
	if(!(hints->flags&PMinSize) || hints->min_height<CWIN_MIN_H)
		hints->min_height=CWIN_MIN_H;
	hints->flags|=PMinSize;
	
	if(!(hints->flags&PBaseSize)){
		hints->flags|=PBaseSize;
		hints->base_width=hints->min_width;
		hints->base_height=hints->min_height;
	}

	if(hints->flags&PMaxSize){
		if(hints->max_width<hints->min_width)
			hints->max_width=hints->min_width;
		if(hints->max_height<hints->min_height)
			hints->max_height=hints->min_height;
	}
	
	/*hints->flags|=PBaseSize;|PMinSize;*/

	if(hints->flags&PResizeInc){
		if(hints->width_inc<=0 || hints->height_inc<=0){
			warn("Invalid client-supplied width/height increment");
			hints->flags&=~PResizeInc;
		}
	}
	
	if(hints->flags&PAspect){
		if(hints->min_aspect.x<=0 || hints->min_aspect.y<=0 ||
		   hints->min_aspect.x<=0 || hints->min_aspect.y<=0){
			warn("Invalid client-supplied aspect-ratio");
			hints->flags&=~PAspect;
		}
	}
	
	if(!(hints->flags&PWinGravity))
		hints->win_gravity=ForgetGravity;
}


/*}}}*/

