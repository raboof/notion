/*
 * ion/de/style.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/extl.h>
#include <ioncore/extlconv.h>
#include <ioncore/ioncore.h>

#include "brush.h"
#include "font.h"
#include "colour.h"
#include "misc.h"
#include "style.h"


/*{{{ GC creation */


static void create_normal_gc(DEStyle *style, WRootWin *rootwin)
{
    XGCValues gcv;
    ulong gcvmask;
    GC gc;

    /* Create normal gc */
    gcv.line_style=LineSolid;
    gcv.line_width=1;
    gcv.join_style=JoinBevel;
    gcv.cap_style=CapButt;
    gcv.fill_style=FillSolid;
    gcvmask=(GCLineStyle|GCLineWidth|GCFillStyle|
             GCJoinStyle|GCCapStyle);
    
    style->normal_gc=XCreateGC(ioncore_g.dpy, WROOTWIN_ROOT(rootwin), 
                               gcvmask, &gcv);
}


void destyle_create_tab_gcs(DEStyle *style)
{
    Display *dpy=ioncore_g.dpy;
    WRootWin *rootwin=style->rootwin;
    Window root=WROOTWIN_ROOT(rootwin);
    Pixmap stipple_pixmap;
    XGCValues gcv;
    ulong gcvmask;
    GC tmp_gc;

    /* Create a temporary 1-bit GC for drawing the tag and stipple pixmaps */
    stipple_pixmap=XCreatePixmap(dpy, root, 2, 2, 1);
    gcv.foreground=1;
    tmp_gc=XCreateGC(dpy, stipple_pixmap, GCForeground, &gcv);

    /* Create stipple pattern and stipple GC */
    XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 0);
    XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 1);
    XSetForeground(dpy, tmp_gc, 0);
    XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 0);
    XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 1);
    
    gcv.fill_style=FillStippled;
    /*gcv.function=GXclear;*/
    gcv.stipple=stipple_pixmap;
    gcvmask=GCFillStyle|GCStipple/*|GCFunction*/;
    if(style->font->fontstruct!=NULL){
		gcv.font=style->font->fontstruct->fid;
		gcvmask|=GCFont;
	}

    style->stipple_gc=XCreateGC(dpy, root, gcvmask, &gcv);
    XCopyGC(dpy, style->normal_gc, 
            GCLineStyle|GCLineWidth|GCJoinStyle|GCCapStyle,
            style->stipple_gc);
            
    XFreePixmap(dpy, stipple_pixmap);
    
    /* Create tag pixmap and copying GC */
    style->tag_pixmap_w=5;
    style->tag_pixmap_h=5;
    style->tag_pixmap=XCreatePixmap(dpy, root, 5, 5, 1);
    
    XSetForeground(dpy, tmp_gc, 0);
    XFillRectangle(dpy, style->tag_pixmap, tmp_gc, 0, 0, 5, 5);
    XSetForeground(dpy, tmp_gc, 1);
    XFillRectangle(dpy, style->tag_pixmap, tmp_gc, 0, 0, 5, 2);
    XFillRectangle(dpy, style->tag_pixmap, tmp_gc, 3, 2, 2, 3);
    
    gcv.foreground=DE_BLACK(rootwin);
    gcv.background=DE_WHITE(rootwin);
    gcv.line_width=2;
    gcvmask=GCLineWidth|GCForeground|GCBackground;
    
    style->copy_gc=XCreateGC(dpy, root, gcvmask, &gcv);
    
    XFreeGC(dpy, tmp_gc);
    
    style->tabbrush_data_ok=TRUE;
}


/*}}}*/


/*{{{ Style lookup */


static DEStyle *styles=NULL;


DEStyle *de_get_style(WRootWin *rootwin, const char *stylename)
{
    DEStyle *style, *maxstyle=NULL;
    int score, maxscore=0;
    
    for(style=styles; style!=NULL; style=style->next){
        if(style->rootwin!=rootwin)
            continue;
        score=gr_stylespec_score(style->style, stylename);
        if(score>maxscore){
            maxstyle=style;
            maxscore=score;
        }
    }
    
    return maxstyle;
}


/*}}}*/


/*{{{ Style initialisation and deinitialisation */


void destyle_unref(DEStyle *style)
{
    style->usecount--;
    if(style->usecount==0){
        destyle_deinit(style);
        free(style);
    }
}


void destyle_deinit(DEStyle *style)
{
    int i;
    
    UNLINK_ITEM(styles, style, next, prev);
    
    if(style->style!=NULL)
        free(style->style);
    
    if(style->font!=NULL){
        de_free_font(style->font);
        style->font=NULL;
    }
    
    if(style->cgrp_alloced)
        de_free_colour_group(style->rootwin, &(style->cgrp));
    
    for(i=0; i<style->n_extra_cgrps; i++)
        de_free_colour_group(style->rootwin, style->extra_cgrps+i);
    
    if(style->extra_cgrps!=NULL)
        free(style->extra_cgrps);
    
    extl_unref_table(style->data_table);
    
    XFreeGC(ioncore_g.dpy, style->normal_gc);
    
    if(style->tabbrush_data_ok){
        XFreeGC(ioncore_g.dpy, style->copy_gc);
        XFreeGC(ioncore_g.dpy, style->stipple_gc);
        XFreePixmap(ioncore_g.dpy, style->tag_pixmap);
    }
    
    XSync(ioncore_g.dpy, False);
    
    if(style->based_on!=NULL){
        destyle_unref(style->based_on);
        style->based_on=NULL;
    }
}


static void dump_style(DEStyle *style)
{
    /* Allow the style still be used but get if off the list. */
    UNLINK_ITEM(styles, style, next, prev);
    destyle_unref(style);
}


bool destyle_init(DEStyle *style, WRootWin *rootwin, const char *name)
{
    style->style=scopy(name);
    if(style->style==NULL){
        warn_err();
        return FALSE;
    }
    
    style->based_on=NULL;
    
    style->usecount=1;
    /* Fallback brushes are not released on de_reset() */
    style->is_fallback=FALSE;
    
    style->rootwin=rootwin;
    
    style->border.sh=1;
    style->border.hl=1;
    style->border.pad=1;
    style->border.style=DEBORDER_INLAID;

    style->spacing=0;
    
    style->textalign=DEALIGN_CENTER;

    style->cgrp_alloced=FALSE;
    style->cgrp.spec=NULL;
    style->cgrp.bg=DE_BLACK(rootwin);
    style->cgrp.pad=DE_BLACK(rootwin);
    style->cgrp.fg=DE_WHITE(rootwin);
    style->cgrp.hl=DE_WHITE(rootwin);
    style->cgrp.sh=DE_WHITE(rootwin);
    
    style->font=NULL;
    
    style->transparency_mode=GR_TRANSPARENCY_NO;
    
    style->n_extra_cgrps=0;
    style->extra_cgrps=NULL;
    
    style->data_table=extl_table_none();
    
    create_normal_gc(style, rootwin);
    
    style->tabbrush_data_ok=FALSE;
    
    return TRUE;
}


static DEStyle *do_create_style(WRootWin *rootwin, const char *name)
{
    DEStyle *style=ALLOC(DEStyle);
    if(style!=NULL){
        if(!destyle_init(style, rootwin, name)){
            free(style);
            return NULL;
        }
    }
    return style;
}


DEStyle *de_create_style(WRootWin *rootwin, const char *name)
{
    DEStyle *oldstyle, *style;

    style=do_create_style(rootwin, name);
    
    if(style==NULL)
        return NULL;
    
    for(oldstyle=styles; oldstyle!=NULL; oldstyle=oldstyle->next){
        if(oldstyle->rootwin==rootwin && oldstyle->style!=NULL && 
           strcmp(oldstyle->style, name)==0){
            break;
        }
    }
    
    if(oldstyle!=NULL && !oldstyle->is_fallback)
        dump_style(oldstyle);
    
    LINK_ITEM_FIRST(styles, style, next, prev);
    
    return style;
}



/*EXTL_DOC
 * Clear all styles from drawing engine memory.
 */
EXTL_EXPORT
void de_reset()
{
    DEStyle *style, *next;
    for(style=styles; style!=NULL; style=next){
        next=style->next;
        if(!style->is_fallback)
            dump_style(style);
    }
}


void de_deinit_styles()
{
    DEStyle *style, *next;
    for(style=styles; style!=NULL; style=next){
        next=style->next;
        if(style->usecount>1){
            WARN_FUNC("Style %s still in use [%d] but the module "
                      "is being unloaded!", style->style, style->usecount);
        }
        dump_style(style);
    }
}


/*}}}*/


