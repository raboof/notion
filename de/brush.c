/*
 * ion/de/brush.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/extlconv.h>
#include <ioncore/ioncore.h>

#include "brush.h"
#include "style.h"
#include "font.h"
#include "colour.h"
#include "private.h"


/*{{{ Brush creation and releasing */


bool debrush_init(DEBrush *brush, Window win,
                  const char *stylename, DEStyle *style)
{
    brush->d=style;
    brush->extras_fn=NULL;
    brush->indicator_w=0;
    brush->win=win;
    brush->clip_set=FALSE;
    
    style->usecount++;

    if(!grbrush_init(&(brush->grbrush))){
        style->usecount--;
        return FALSE;
    }
    
    if(MATCHES("tab-frame", stylename) || MATCHES("tab-info", stylename)){
        brush->extras_fn=debrush_tab_extras;
        if(!style->tabbrush_data_ok)
            destyle_create_tab_gcs(style);
    }else if(MATCHES("tab-menuentry", stylename)){
        brush->extras_fn=debrush_menuentry_extras;
        brush->indicator_w=grbrush_get_text_width((GrBrush*)brush, 
                                                  DE_SUB_IND, 
                                                  DE_SUB_IND_LEN);
    }
    
    return TRUE;
}


DEBrush *create_debrush(Window win, const char *stylename, DEStyle *style)
{
    CREATEOBJ_IMPL(DEBrush, debrush, (p, win, stylename, style));
}


static DEBrush *do_get_brush(Window win, WRootWin *rootwin, 
                             const char *stylename, bool slave)
{
    DEStyle *style=de_get_style(rootwin, stylename);
    DEBrush *brush;
    
    if(style==NULL)
        return NULL;
    
    brush=create_debrush(win, stylename, style);
    
    /* Set background colour */
    if(brush!=NULL && !slave){
        grbrush_enable_transparency(&(brush->grbrush), 
                                    GR_TRANSPARENCY_DEFAULT);
    }
    
    return brush;
}


DEBrush *de_get_brush(Window win, WRootWin *rootwin, const char *stylename)
{
    return do_get_brush(win, rootwin, stylename, FALSE);
}


DEBrush *debrush_get_slave(DEBrush *master, WRootWin *rootwin, 
                           const char *stylename)
{
    return do_get_brush(master->win, rootwin, stylename, TRUE);
}


void debrush_deinit(DEBrush *brush)
{
    destyle_unref(brush->d);
    brush->d=NULL;
    grbrush_deinit(&(brush->grbrush));
}


void debrush_release(DEBrush *brush)
{
    destroy_obj((Obj*)brush);
}


/*}}}*/


/*{{{ Border widths and extra information */


void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw)
{
    DEStyle *style=brush->d;
    DEBorder *bd=&(style->border);
    uint tmp;
    
    switch(bd->style){
    case DEBORDER_RIDGE:
    case DEBORDER_GROOVE:
        tmp=bd->sh+bd->hl+bd->pad;
        bdw->top=tmp; bdw->bottom=tmp; bdw->left=tmp; bdw->right=tmp;
        break;
    case DEBORDER_INLAID:
        tmp=bd->sh+bd->pad; bdw->top=tmp; bdw->left=tmp;
        tmp=bd->hl+bd->pad; bdw->bottom=tmp; bdw->right=tmp;
        break;
    case DEBORDER_ELEVATED:
    default:
        tmp=bd->hl+bd->pad; bdw->top=tmp; bdw->left=tmp;
        tmp=bd->sh+bd->pad; bdw->bottom=tmp; bdw->right=tmp;
        break;
    }
    
    bdw->tb_ileft=bdw->left;
    bdw->tb_iright=bdw->right;
    bdw->spacing=style->spacing;
    
    bdw->right+=brush->indicator_w;
    bdw->tb_iright+=brush->indicator_w;
}


bool debrush_get_extra(DEBrush *brush, const char *key, char type, void *data)
{
    DEStyle *style=brush->d;
    while(style!=NULL){
        if(extl_table_get(style->data_table, 's', type, key, data))
            return TRUE;
        style=style->based_on;
    }
    return FALSE;
}



/*}}}*/


/*{{{ Class implementation */


static DynFunTab debrush_dynfuntab[]={
    {grbrush_release, debrush_release},
    {grbrush_draw_border, debrush_draw_border},
    {grbrush_draw_borderline, debrush_draw_borderline},
    {grbrush_get_border_widths, debrush_get_border_widths},
    {grbrush_draw_string, debrush_draw_string},
    {debrush_do_draw_string, debrush_do_draw_string_default},
    {grbrush_get_font_extents, debrush_get_font_extents},
    {(DynFun*)grbrush_get_text_width, (DynFun*)debrush_get_text_width},
    {grbrush_draw_textbox, debrush_draw_textbox},
    {grbrush_draw_textboxes, debrush_draw_textboxes},
    {grbrush_set_window_shape, debrush_set_window_shape},
    {grbrush_enable_transparency, debrush_enable_transparency},
    {grbrush_clear_area, debrush_clear_area},
    {grbrush_fill_area, debrush_fill_area},
    {(DynFun*)grbrush_get_extra, (DynFun*)debrush_get_extra},
    {(DynFun*)grbrush_get_slave, (DynFun*)debrush_get_slave},
    {grbrush_begin, debrush_begin},
    {grbrush_end, debrush_end},
    END_DYNFUNTAB
};


IMPLCLASS(DEBrush, GrBrush, debrush_deinit, debrush_dynfuntab);


/*}}}*/



