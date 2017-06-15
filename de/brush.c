/*
 * ion/de/brush.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
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


#define MATCHES(S, A) (gr_stylespec_score(&(S), A)>0)

#define ENSURE_INITSPEC(S, NM) \
    if((S).attrs==NULL) gr_stylespec_load(&(S), NM);


static GrStyleSpec tabframe_spec=GR_STYLESPEC_INIT;
static GrStyleSpec tabinfo_spec=GR_STYLESPEC_INIT;
static GrStyleSpec tabmenuentry_spec=GR_STYLESPEC_INIT;


bool debrush_init(DEBrush *brush, Window win,
                  const GrStyleSpec *spec, DEStyle *style)
{
    brush->d=style;
    brush->extras_fn=NULL;
    brush->indicator_w=0;
    brush->win=win;
    brush->clip_set=FALSE;

    gr_stylespec_init(&brush->current_attr);

#ifdef HAVE_X11_XFT
    brush->draw=NULL;
#endif /* HAVE_X11_XFT */
    style->usecount++;

    if(!grbrush_init(&(brush->grbrush))){
        style->usecount--;
        return FALSE;
    }

    ENSURE_INITSPEC(tabframe_spec, "tab-frame");
    ENSURE_INITSPEC(tabinfo_spec, "tab-info");
    ENSURE_INITSPEC(tabmenuentry_spec, "tab-menuentry");

    if(MATCHES(tabframe_spec, spec) || MATCHES(tabinfo_spec, spec)){
        brush->extras_fn=debrush_tab_extras;
        if(!style->tabbrush_data_ok)
            destyle_create_tab_gcs(style);
    }else if(MATCHES(tabmenuentry_spec, spec)){
        brush->extras_fn=debrush_menuentry_extras;
        brush->indicator_w=grbrush_get_text_width((GrBrush*)brush,
                                                  DE_SUB_IND,
                                                  DE_SUB_IND_LEN);
    }

    return TRUE;
}


DEBrush *create_debrush(Window win, const GrStyleSpec *spec, DEStyle *style)
{
    CREATEOBJ_IMPL(DEBrush, debrush, (p, win, spec, style));
}


static DEBrush *do_get_brush(Window win, WRootWin *rootwin,
                             const char *stylename, bool slave)
{
    DEStyle *style;
    DEBrush *brush;
    GrStyleSpec spec;

    if(!gr_stylespec_load(&spec, stylename))
        return NULL;

    style=de_get_style(rootwin, &spec);

    if(style==NULL){
        gr_stylespec_unalloc(&spec);
        return NULL;
    }

    brush=create_debrush(win, &spec, style);

    gr_stylespec_unalloc(&spec);

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
#ifdef HAVE_X11_XFT
    if(brush->draw!=NULL)
        XftDrawDestroy(brush->draw);
#endif /* HAVE_X11_XFT */
    gr_stylespec_unalloc(&brush->current_attr);
    grbrush_deinit(&(brush->grbrush));
}


void debrush_release(DEBrush *brush)
{
    destroy_obj((Obj*)brush);
}


#ifdef HAVE_X11_XFT
XftDraw *debrush_get_draw(DEBrush *brush, Drawable d)
{
    if(brush->draw==NULL)
        brush->draw=XftDrawCreate(ioncore_g.dpy, d,
                                  XftDEDefaultVisual(),
                                  DefaultColormap(ioncore_g.dpy,
                                  0));
    else
        XftDrawChange(brush->draw, d);

    return brush->draw;
}
#endif

/*}}}*/


/*{{{ Attributes */


void debrush_init_attr(DEBrush *brush, const GrStyleSpec *spec)
{
    gr_stylespec_unalloc(&brush->current_attr);

    if(spec!=NULL)
        gr_stylespec_append(&brush->current_attr, spec);
}


void debrush_set_attr(DEBrush *brush, GrAttr attr)
{
    gr_stylespec_set(&brush->current_attr, attr);
}


void debrush_unset_attr(DEBrush *brush, GrAttr attr)
{
    gr_stylespec_unset(&brush->current_attr, attr);
}


GrStyleSpec *debrush_get_current_attr(DEBrush *brush)
{
    return &brush->current_attr;
}


/*}}}*/



/*{{{ Border widths and extra information */


void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw)
{
    DEStyle *style=brush->d;
    DEBorder *bd=&(style->border);
    uint tmp=0;
    uint tbf=1, lrf=1;
    uint pad=bd->pad;
    uint spc=style->spacing;

    switch(bd->sides){
    case DEBORDER_TB:
        lrf=0;
        break;
    case DEBORDER_LR:
        tbf=0;
        break;
    }

    /* Ridge/groove styles use 'padding' for the spacing between the
     * 'highlight' and 'shadow' portions of the border, and 'spacing'
     * between the border and contents. Inlaid style also uses 'spacing'
     * between the contents and the border, and padding as its outer
     * component. Elevated style does not use spacing.
     */
    switch(bd->style){
    case DEBORDER_RIDGE:
    case DEBORDER_GROOVE:
        tmp=bd->sh+bd->hl+pad;
        bdw->top=tbf*tmp+spc; bdw->bottom=tbf*tmp+spc;
        bdw->left=lrf*tmp+spc; bdw->right=lrf*tmp+spc;
        break;
    case DEBORDER_INLAID:
        tmp=bd->sh+pad; bdw->top=tbf*tmp+spc; bdw->left=lrf*tmp+spc;
        tmp=bd->hl+pad; bdw->bottom=tbf*tmp+spc; bdw->right=lrf*tmp+spc;
        break;
    case DEBORDER_ELEVATED:
    default:
        tmp=bd->hl; bdw->top=tbf*tmp+pad; bdw->left=lrf*tmp+pad;
        tmp=bd->sh; bdw->bottom=tbf*tmp+pad; bdw->right=lrf*tmp+pad;
        break;
    }

    bdw->right+=brush->indicator_w;

    bdw->tb_ileft=bdw->left;
    bdw->tb_iright=bdw->right;
    bdw->spacing=style->spacing;
}


bool debrush_get_extra(DEBrush *brush, const char *key, char type, void *data)
{
    DEStyle *style=brush->d;
    while(style!=NULL){
        if(extl_table_get(style->extras_table, 's', type, key, data))
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
    {grbrush_init_attr, debrush_init_attr},
    {grbrush_set_attr, debrush_set_attr},
    {grbrush_unset_attr, debrush_unset_attr},
    END_DYNFUNTAB
};


IMPLCLASS(DEBrush, GrBrush, debrush_deinit, debrush_dynfuntab);


/*}}}*/



