/*
 * wmcore/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "global.h"
#include "draw.h"
#include "font.h"
#include "drawp.h"
#include "imports.h"
#include "conf-draw.h"


/*{{{ Primitives */


/* Draw a border at x, y with outer width w x h. Top and left 'tl' pixels
 * wide with color 'tlc' and bottom and right 'br' pixels with colors 'brc'.
 */
void do_draw_border(Window win, GC gc, int x, int y, int w, int h,
					int tl, int br, WColor tlc, WColor brc)
{
	XPoint points[3];
	int i=0, a=0;
	
	w--;
	h--;

	set_foreground(wglobal.dpy, gc, tlc);

	for(i=a=0; i<tl; i++){
		points[0].x=x+i;		points[0].y=y+h-a+1;
		points[1].x=x+i;		points[1].y=y+i;
		points[2].x=x+w-a+1;	points[2].y=y+i;

		if(i<br)
			a++;
	
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}

	
	set_foreground(wglobal.dpy, gc, brc);

	for(i=a=0; i<br; i++){	
		points[0].x=x+w-i;		points[0].y=y+a;
		points[1].x=x+w-i;		points[1].y=y+h-i;
		points[2].x=x+a+1;		points[2].y=y+h-i;
	
		if(i<tl)
			a++;
		
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}
}


void draw_border(DrawInfo *dinfo)
{
	do_draw_border(WIN, XGC, X, Y, W, H, BORDER->tl, BORDER->br, 
				   COLORS->hl, COLORS->sh);
}


void draw_box(DrawInfo *dinfo, bool fill)
{
	if(fill){
		set_foreground(wglobal.dpy, XGC, COLORS->bg);
		XFillRectangle(wglobal.dpy, WIN, XGC, C_X, C_Y, C_W, C_H);
	}

	draw_border(dinfo);
}


void draw_textbox(DrawInfo *dinfo,
				  const char *str, int align, bool fill)
{
	int len;
	int tw, tx, ty;
	
	draw_box(dinfo, fill);

	if(str==NULL)
		return;

	len=strlen(str);
	
	if(align!=ALIGN_LEFT){
		tw=text_width(FONT, str, len);
		
		if(align==ALIGN_CENTER)
			tx=I_X+I_W/2-tw/2;
		else
			tx=I_X+I_W-tw;
	}else{
		tx=I_X;
	}
	
	ty=I_Y+I_H/2-FONT_HEIGHT(FONT)/2+FONT_BASELINE(FONT);
	
#ifdef CF_XFT
	XftDrawString8(DRAW, &COLORS->fg, FONT, tx, ty, (XftChar8 *) str, len);
#else
	XSetForeground(wglobal.dpy, XGC, COLORS->fg);
	XDrawString(wglobal.dpy, WIN, XGC, tx, ty, str, len);
#endif
}


void draw_image_string(DrawInfo *dinfo, int x, int y, const char *str, int len,
					   WColor *fg, WColor *bg)
{
#ifdef CF_XFT
	XGlyphInfo extents;
	XftTextExtents8(wglobal.dpy, FONT, (XftChar8 *) str, len, &extents);
	XftDrawRect(DRAW, bg, x, y - FONT->ascent, extents.xOff, FONT->height);
	XftDrawString8(DRAW, fg, FONT, x, y, (XftChar8 *) str, len);
#else
	set_foreground(wglobal.dpy, XGC, *fg);
	set_background(wglobal.dpy, XGC, *bg);
	XDrawImageString(wglobal.dpy, WIN, XGC, x, y, str, len);
#endif
}


/* */


void copy_masked(const WGRData *grdata, Drawable src, Drawable dst,
				 int src_x, int src_y, int w, int h,
				 int dst_x, int dst_y)
{

	XSetClipMask(wglobal.dpy, grdata->copy_gc, src);
	XSetClipOrigin(wglobal.dpy, grdata->copy_gc, dst_x, dst_y);
	XCopyPlane(wglobal.dpy, src, dst, grdata->copy_gc, src_x, src_y,
			   w, h, dst_x, dst_y, 1);
}


/*}}}*/


/*{{{ Color alloc */


bool alloc_color(WScreen *scr, const char *name, WColor *cret)
{
#ifdef CF_XFT
	return XftColorAllocName(wglobal.dpy, DefaultVisual(wglobal.dpy, scr->xscr),
			  scr->default_cmap, (char *) name, cret);
#else
	XColor c;
	bool ret=FALSE;

	if(*cret!=scr->grdata.black && *cret!=scr->grdata.white)
		XFreeColors(wglobal.dpy, scr->default_cmap, cret, 1, 0);
	
	if(XParseColor(wglobal.dpy, scr->default_cmap, name, &c)){
		ret=XAllocColor(wglobal.dpy, scr->default_cmap, &c);
		*cret=c.pixel;
	}
	return ret;
#endif
}


static void free_cg(WScreen *scr, WColorGroup *cg)
{
#ifdef CF_XFT
	XftColorFree (wglobal.dpy, DefaultVisual(wglobal.dpy, scr->xscr),
				  scr->default_cmap, &cg->bg);
	XftColorFree (wglobal.dpy, DefaultVisual(wglobal.dpy, scr->xscr),
				  scr->default_cmap, &cg->fg);
	XftColorFree (wglobal.dpy, DefaultVisual(wglobal.dpy, scr->xscr),
				  scr->default_cmap, &cg->hl);
	XftColorFree (wglobal.dpy, DefaultVisual(wglobal.dpy, scr->xscr),
				  scr->default_cmap, &cg->sh);
#else
	WColor pixels[4];
	
	pixels[0]=cg->bg;
	pixels[1]=cg->fg;
	pixels[2]=cg->hl;
	pixels[3]=cg->sh;
	
	XFreeColors(wglobal.dpy, scr->default_cmap, pixels, 4, 0);
#endif
}

	
void setup_color_group(WScreen *scr, WColorGroup *cg,
					   WColor hl, WColor sh, WColor bg, WColor fg)
{
	free_cg(scr, cg);
					
	cg->bg=bg;
	cg->hl=hl;
	cg->sh=sh;
	cg->fg=fg;
}


/*}}}*/


/*{{{ Initialization code */


/* This should be called before reading the configuration file */
void preinit_graphics(WScreen *scr)
{
	WGRData *grdata=&(scr->grdata);
	WColor black, white;
	
#ifdef CF_XFT
	black.pixel=BlackPixel(wglobal.dpy, scr->xscr);
	black.color.red=0;
	black.color.green=0;
	black.color.blue=0;
	black.color.alpha=0xffff;
	white.pixel=WhitePixel(wglobal.dpy, scr->xscr);
	white.color.red=0xffff;
	white.color.green=0xffff;
	white.color.blue=0xffff;
	white.color.alpha=0xffff;
#else
	black=BlackPixel(wglobal.dpy, scr->xscr);
	white=WhitePixel(wglobal.dpy, scr->xscr);
#endif
	grdata->black=black;
	grdata->white=white;

#define INIT_CG(CG, HL, SH, BG, FG) CG.bg=BG; CG.hl=HL; CG.sh=SH; CG.fg=FG;
										/* hl, sh,    bg,    fg */
	INIT_CG(grdata->tab_sel_colors, 	white, white, black, white);
	INIT_CG(grdata->tab_colors, 		black, black, black, white);
	INIT_CG(grdata->frame_colors,		white, white, black, white);

	INIT_CG(grdata->act_tab_sel_colors, black, black, white, black);
	INIT_CG(grdata->act_tab_colors, 	white, white, white, black);
	INIT_CG(grdata->act_frame_colors,	white, white, black, white);

	INIT_CG(grdata->input_colors,		white, white, black, white);
	
#undef INIT_CG

	grdata->frame_bgcolor=black;
	grdata->transparent_background=FALSE;
	grdata->selection_bgcolor=white;
	grdata->selection_fgcolor=black;
	
#define INIT_BD(BD, TL, BR, IPAD) BD.tl=TL; BD.br=BR; BD.ipad=IPAD;
	
	INIT_BD(grdata->frame_border, 1, 1, 0);
	INIT_BD(grdata->tab_border, 1, 1, 1);
	INIT_BD(grdata->input_border, 1, 1, 1);

#undef INIT_BD

	grdata->bar_inside_frame=FALSE;
	grdata->spacing=1;
	grdata->font=NULL;
	grdata->tab_font=NULL;
}


static int max_width(WFont *font, const char *str)
{
	int maxw=0, w;
	
	while(*str!='\0'){
		w=text_width(font, str, 1);
		if(w>maxw)
		maxw=w;
		str++;
	}
	
	return maxw;
}


static int chars_for_num(int d)
{
	int n=0;
	
	do{
		n++;
		d/=10;
	}while(d);
	
	return n;
}


static void create_wm_windows(WScreen *scr)
{
	WGRData *grdata=&(scr->grdata);
	XSetWindowAttributes attr;
	int w, h;
	
	/* Create move/resize positwmcore/size display window */
	w=3;
	w+=chars_for_num(REGION_GEOM(scr).w);
	w+=chars_for_num(REGION_GEOM(scr).h);
	w*=max_width(grdata->font, "0123456789x+"); 	
	w+=(BORDER_TL_TOTAL(&(grdata->tab_border))+
		BORDER_BR_TOTAL(&(grdata->tab_border)));
	
	h=(FONT_HEIGHT(grdata->tab_font)+
	   BORDER_TL_TOTAL(&(grdata->tab_border))+
	   BORDER_BR_TOTAL(&(grdata->tab_border)));

	grdata->moveres_geom.x=CF_MOVERES_WIN_X;
	grdata->moveres_geom.y=CF_MOVERES_WIN_Y;
	grdata->moveres_geom.w=w;
	grdata->moveres_geom.h=h;

	attr.save_under=True;
	attr.background_pixel=COLOR_PIXEL(grdata->tab_sel_colors.bg);

	grdata->moveres_win=
		XCreateWindow(wglobal.dpy, scr->root.win,
					  CF_MOVERES_WIN_X, CF_MOVERES_WIN_Y, w, h, 0,
					  CopyFromParent, InputOutput, CopyFromParent,
					  CWSaveUnder|CWBackPixel, &attr);
#ifdef CF_XFT
	grdata->moveres_draw=XftDrawCreate(wglobal.dpy, grdata->moveres_win,
									   DefaultVisual(wglobal.dpy, scr->xscr),
									   scr->default_cmap);
#endif

	/* Create tab drag window */
	grdata->drag_geom.x=0;
	grdata->drag_geom.y=0;
	grdata->drag_geom.w=16;
	grdata->drag_geom.h=16;
	
	attr.background_pixel=COLOR_PIXEL(grdata->frame_colors.bg);
	
	grdata->drag_win=
		XCreateWindow(wglobal.dpy, scr->root.win,
					  0, 0, 16, 16, 0,
					  CopyFromParent, InputOutput, CopyFromParent,
					  CWSaveUnder|CWBackPixel, &attr);
#ifdef CF_XFT
	grdata->drag_draw=XftDrawCreate(wglobal.dpy, grdata->drag_win,
								    DefaultVisual(wglobal.dpy, scr->xscr),
								    scr->default_cmap);
#endif
		
	XSelectInput(wglobal.dpy, grdata->drag_win, ExposureMask);
}



/* This should be called after reading the configuration file */
void postinit_graphics(WScreen *scr)
{
	Display *dpy=wglobal.dpy;
	WGRData *grdata=&(scr->grdata);
	Window root=scr->root.win;
	WColor black, white;
	XGCValues gcv;
	ulong gcvmask;
	Pixmap stipple_pixmap;
	GC tmp_gc;

	black=grdata->black;
	white=grdata->white;

	/* font */
	if(grdata->font==NULL)
		grdata->font=load_font(dpy, CF_FALLBACK_FONT_NAME);
	
	if(grdata->tab_font==NULL)
		grdata->tab_font=load_font(dpy, CF_FALLBACK_FONT_NAME);

	/* Create normal gc */
	gcv.line_style=LineSolid;
	gcv.line_width=1;
	gcv.join_style=JoinBevel;
	gcv.cap_style=CapButt;
	gcv.fill_style=FillSolid;
#ifndef CF_XFT
	gcv.font=grdata->font->fid;
#endif

	gcvmask=(GCLineStyle|GCLineWidth|GCFillStyle|
			 GCJoinStyle|GCCapStyle);
#ifndef CF_XFT
	gcvmask|=GCFont;
#endif
	
	grdata->gc=XCreateGC(dpy, root, gcvmask, &gcv);

	/* Create tab gc (other font) */
#ifndef CF_XFT
	gcv.font=grdata->tab_font->fid;
#endif
	
	grdata->tab_gc=XCreateGC(dpy, root, gcvmask, &gcv);

	/* Create stipple pattern and stipple GC */
	stipple_pixmap=XCreatePixmap(dpy, root, 2, 2, 1);
	gcv.foreground=1;
	tmp_gc=XCreateGC(dpy, stipple_pixmap, GCForeground, &gcv);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 1);
	XSetForeground(dpy, tmp_gc, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 1);
	
	if(!grdata->bar_inside_frame)
		gcv.foreground=COLOR_PIXEL(grdata->frame_bgcolor);
	
	gcv.fill_style=FillStippled;
	gcv.stipple=stipple_pixmap;
	
	grdata->stipple_gc=XCreateGC(dpy, root, gcvmask|GCStipple, &gcv);
	
	XFreePixmap(dpy, stipple_pixmap);
	
	/* Create stick pixmap */
	grdata->stick_pixmap_w=7;
	grdata->stick_pixmap_h=7;
	grdata->stick_pixmap=XCreatePixmap(dpy, root, 7, 7, 1);
	
	XSetForeground(wglobal.dpy, tmp_gc, 0);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 0, 0, 7, 7);
	XSetForeground(wglobal.dpy, tmp_gc, 1);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 0, 2, 5, 2);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 3, 4, 2, 3);
	
	XFreeGC(dpy, tmp_gc);

	/* Create copy gc */
	gcv.foreground=COLOR_PIXEL(black);
	gcv.background=COLOR_PIXEL(white);
	gcv.line_width=2;
	grdata->copy_gc=XCreateGC(dpy, root, GCLineWidth|GCForeground|GCBackground,
							  &gcv);

	/* Create XOR gc (for resize) */
	gcv.subwindow_mode=IncludeInferiors;
	gcv.function=GXxor;
	gcv.foreground=~0L;
	gcv.fill_style=FillSolid;
	gcvmask|=GCFunction|GCSubwindowMode|GCForeground;

	grdata->xor_gc=XCreateGC(dpy, root, gcvmask, &gcv);

	/* the rest */
	
	calc_grdata(scr);
	
	create_wm_windows(scr);
}


/*}}}*/


/*{{{ Rubberband */


void draw_rubberbox(WScreen *scr, WRectangle rect)
{
	XPoint fpts[5];
	
	fpts[0].x=rect.x;
	fpts[0].y=rect.y;
	fpts[1].x=rect.x+rect.w;
	fpts[1].y=rect.y;
	fpts[2].x=rect.x+rect.w;
	fpts[2].y=rect.y+rect.h;
	fpts[3].x=rect.x;
	fpts[3].y=rect.y+rect.h;
	fpts[4].x=rect.x;
	fpts[4].y=rect.y;
	
	XDrawLines(wglobal.dpy, scr->root.win, scr->grdata.xor_gc, fpts, 5,
			   CoordModeOrigin);
}


void draw_rubberband(WScreen *scr, WRectangle rect, bool vertical)
{
	Window win=scr->root.win;
	GC gc=scr->grdata.xor_gc;
	
	if(vertical){
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x, rect.y, rect.x+rect.w, rect.y);
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x, rect.y+rect.h, rect.x+rect.w, rect.y+rect.h);
	}else{
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x, rect.y, rect.x, rect.y+rect.h);
		XDrawLine(wglobal.dpy, win, gc,
				  rect.x+rect.w, rect.y, rect.x+rect.w, rect.y+rect.h);
	}
}


/*}}}*/


/*{{{ Move/resize positwmcore/size display */


static char moveres_tmpstr[CF_MAX_MOVERES_STR_SIZE];


static void do_draw_moveres(WScreen *scr, const char *str)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=&(scr->grdata);
	
	dinfo->win=grdata->moveres_win;
#ifdef CF_XFT
	dinfo->draw = grdata->moveres_draw;
#endif
	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom.x=0;
	dinfo->geom.y=0;
	dinfo->geom.w=grdata->moveres_geom.w;
	dinfo->geom.h=grdata->moveres_geom.h;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;
	dinfo->colors=&(grdata->tab_colors);

	draw_textbox(dinfo, str, ALIGN_CENTER, TRUE);
}


void set_moveres_pos(WScreen *scr, int x, int y)
{
	sprintf(moveres_tmpstr, "%+d %+d", x, y);
	do_draw_moveres(scr, moveres_tmpstr);
}


void set_moveres_size(WScreen *scr, int w, int h)
{
	sprintf(moveres_tmpstr, "%dx%d", w, h);
	do_draw_moveres(scr, moveres_tmpstr);
}


/*}}}*/


/*{{{ Clipping */


void set_clipping_rectangle(DrawInfo *dinfo, int x, int y, int w, int h)
{
	XRectangle rect;
#ifdef CF_XFT
	Region rgn;
#endif
	
	rect.x=x; rect.y=y; rect.width=w; rect.height=h;
	
	XSetClipRectangles(wglobal.dpy, XGC, 0, 0, &rect, 1, Unsorted);
#ifdef CF_XFT
	rgn=XCreateRegion();
	XUnionRectWithRegion(&rect, rgn, rgn);
	XftDrawSetClip(DRAW, rgn);
	XDestroyRegion(rgn);
#endif
}


void clear_clipping(DrawInfo *dinfo)
{
	XSetClipMask(wglobal.dpy, XGC, None);
#ifdef CF_XFT
	XftDrawSetClip(DRAW, 0);
#endif
}


/*}}}*/


/*{{{ reread_draw_config */


void reread_draw_config()
{
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		read_draw_config(scr);
#ifndef CF_XFT
		XSetFont(wglobal.dpy, scr->grdata.gc, 
				 scr->grdata.font->fid);
		XSetFont(wglobal.dpy, scr->grdata.tab_gc, 
				 scr->grdata.tab_font->fid);
#endif		
		calc_grdata(scr);
		region_draw_config_updated((WRegion*)scr);
	}
}


/*}}}*/

