/*
 * ion/de/init.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libextl/readconfig.h>
#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/rootwin.h>
#include <ioncore/extlconv.h>
#include <ioncore/ioncore.h>

#include "brush.h"
#include "font.h"
#include "colour.h"
#include "misc.h"
#include "init.h"


/*{{{ Borders */


#define CF_BORDER_VAL_SANITY_CHECK 16

void de_get_border_val(uint *val, ExtlTab tab, const char *what)
{
    int g;
    
    if(extl_table_gets_i(tab, what, &g)){
        if(g>CF_BORDER_VAL_SANITY_CHECK || g<0)
            warn(TR("Border attribute %s sanity check failed."), what);
        else
            *val=g;
    }
}


void de_get_border_style(uint *ret, ExtlTab tab)
{
    char *style=NULL;
    
    if(!extl_table_gets_s(tab, "border_style", &style))
        return;
    
    if(strcmp(style, "inlaid")==0)
        *ret=DEBORDER_INLAID;
    else if(strcmp(style, "elevated")==0)
        *ret=DEBORDER_ELEVATED;
    else if(strcmp(style, "groove")==0)
        *ret=DEBORDER_GROOVE;
    else if(strcmp(style, "ridge")==0)
        *ret=DEBORDER_RIDGE;
    else
        warn(TR("Unknown border style \"%s\"."), style);
    
    free(style);
}


void de_get_border(DEBorder *border, ExtlTab tab)
{
    de_get_border_val(&(border->sh), tab, "shadow_pixels");
    de_get_border_val(&(border->hl), tab, "highlight_pixels");
    de_get_border_val(&(border->pad), tab, "padding_pixels");
    de_get_border_style(&(border->style), tab);
}


/*}}}*/


/*{{{ Colours */


bool de_get_colour(WRootWin *rootwin, DEColour *ret, 
                   ExtlTab tab, DEStyle *based_on,
                   const char *what, DEColour substitute)
{
    char *name=NULL;
    bool ok=FALSE;
    
    if(extl_table_gets_s(tab, what, &name)){
        ok=de_alloc_colour(rootwin, ret, name);
    
        if(!ok)
            warn(TR("Unable to allocate colour \"%s\"."), name);

        free(name);
    }
    
    if(!ok && based_on!=NULL){
        return de_get_colour(rootwin, ret, based_on->data_table,
                             based_on->based_on, what, substitute);
    }else if(!ok){
        return de_duplicate_colour(rootwin, substitute, ret);
    }
    
    return ok;
}


void de_get_colour_group(WRootWin *rootwin, DEColourGroup *cg, 
                         ExtlTab tab, DEStyle *based_on)
{
    de_get_colour(rootwin, &(cg->hl), tab, based_on, "highlight_colour",
                  DE_WHITE(rootwin));
    de_get_colour(rootwin, &(cg->sh), tab, based_on, "shadow_colour",
                  DE_WHITE(rootwin));
    de_get_colour(rootwin, &(cg->bg), tab, based_on, "background_colour",
                  DE_BLACK(rootwin));
    de_get_colour(rootwin, &(cg->fg), tab, based_on, "foreground_colour",
                  DE_WHITE(rootwin));
    de_get_colour(rootwin, &(cg->pad), tab, based_on, "padding_colour", 
                  cg->bg);
}


void de_get_extra_cgrps(WRootWin *rootwin, DEStyle *style, ExtlTab tab)
{
    
    uint i=0, nfailed=0, n=extl_table_get_n(tab);
    char *name;
    ExtlTab sub;
    
    if(n==0)
        return;
    
    style->extra_cgrps=ALLOC_N(DEColourGroup, n);
    
    if(style->extra_cgrps==NULL)
        return;

    for(i=0; i<n-nfailed; i++){
        if(!extl_table_geti_t(tab, i+1, &sub))
            goto err;
        if(!extl_table_gets_s(sub, "substyle_pattern", &name)){
            extl_unref_table(sub);
            goto err;
        }
        
        /*de_init_colour_group(rootwin, style->extra_cgrps+i-nfailed);*/
        style->extra_cgrps[i-nfailed].spec=name;
        de_get_colour_group(rootwin, style->extra_cgrps+i-nfailed, sub, 
                            style);
        
        extl_unref_table(sub);
        continue;
        
    err:
        warn(TR("Corrupt substyle table %d."), i);
        nfailed++;
    }
    
    if(n-nfailed==0){
        free(style->extra_cgrps);
        style->extra_cgrps=NULL;
    }
    
    style->n_extra_cgrps=n-nfailed;
}


/*}}}*/


/*{{{ Misc. */


void de_get_text_align(int *alignret, ExtlTab tab)
{
    char *align=NULL;
    
    if(!extl_table_gets_s(tab, "text_align", &align))
        return;
    
    if(strcmp(align, "left")==0)
        *alignret=DEALIGN_LEFT;
    else if(strcmp(align, "right")==0)
        *alignret=DEALIGN_RIGHT;
    else if(strcmp(align, "center")==0)
        *alignret=DEALIGN_CENTER;
    else
        warn(TR("Unknown text alignment \"%s\"."), align);
    
    free(align);
}


void de_get_transparent_background(uint *mode, ExtlTab tab)
{
    bool b;
    
    if(extl_table_gets_b(tab, "transparent_background", &b))
        *mode=b;
}


/*}}}*/


/*{{{ de_defstyle */


void de_get_nonfont(WRootWin *rootwin, DEStyle *style, ExtlTab tab)
{
    DEStyle *based_on=style->based_on;
    
    style->data_table=extl_ref_table(tab);

    if(based_on!=NULL){
        style->border=based_on->border;
        style->transparency_mode=based_on->transparency_mode;
        style->textalign=based_on->textalign;
        style->spacing=based_on->spacing;
    }
    
    de_get_border(&(style->border), tab);
    de_get_border_val(&(style->spacing), tab, "spacing");

    de_get_text_align(&(style->textalign), tab);

    de_get_transparent_background(&(style->transparency_mode), tab);

    style->cgrp_alloced=TRUE;
    de_get_colour_group(rootwin, &(style->cgrp), tab, based_on);
    de_get_extra_cgrps(rootwin, style, tab);
}


/*EXTL_DOC
 * Define a style for the root window \var{rootwin}. 
 */
EXTL_EXPORT
bool de_defstyle_rootwin(WRootWin *rootwin, const char *name, ExtlTab tab)
{
    DEStyle *style;
    char *fnt;
    uint n;
    DEStyle *based_on=NULL;
    char *based_on_name=NULL;

    if(name==NULL)
        return FALSE;
    
    style=de_create_style(rootwin, name);

    if(style==NULL)
        return FALSE;

    if(extl_table_gets_s(tab, "based_on", &based_on_name)){
        based_on=de_get_style(rootwin, based_on_name);
        if(based_on==style){
            warn(TR("'based_on' for %s points back to the style itself."),
                 name);
        }else if(based_on==NULL){
            warn(TR("Unknown base style \"%s\"."), based_on);
        }else{
            style->based_on=based_on;
            based_on->usecount++;
            /* Copy simple parameters */
        }
        free(based_on_name);
    }

    de_get_nonfont(rootwin, style, tab);

    if(extl_table_gets_s(tab, "font", &fnt)){
        de_load_font_for_style(style, fnt);
        free(fnt);
    }else if(based_on!=NULL){
        de_set_font_for_style(style, based_on->font);
    }
    
    if(style->font==NULL)
        de_load_font_for_style(style, CF_FALLBACK_FONT_NAME);
    
    return TRUE;
}


/*EXTL_DOC
 * Define a style.
 */
EXTL_EXPORT
bool de_defstyle(const char *name, ExtlTab tab)
{
    bool ok=TRUE;
    WRootWin *rw;
    
    FOR_ALL_ROOTWINS(rw){
        if(!de_defstyle_rootwin(rw, name, tab))
            ok=FALSE;
    }
    
    return ok;
}


/*EXTL_DOC
 * Define a substyle.
 */
EXTL_EXPORT
ExtlTab de_substyle(const char *pattern, ExtlTab tab)
{
    extl_table_sets_s(tab, "substyle_pattern", pattern);
    return extl_ref_table(tab);
}


/*}}}*/


/*{{{ Module initialisation */


#include "../version.h"

char de_ion_api_version[]=ION_API_VERSION;


extern bool de_register_exports();
extern void de_unregister_exports();


bool de_init()
{
    WRootWin *rootwin;
    DEStyle *style;
    
    if(!de_register_exports())
        return FALSE;
    
    if(!gr_register_engine("de", (GrGetBrushFn*)&de_get_brush))
        goto fail;
    
    /* Create fallback brushes */
    FOR_ALL_ROOTWINS(rootwin){
        style=de_create_style(rootwin, "*");
        if(style!=NULL){
            style->is_fallback=TRUE;
            de_load_font_for_style(style, CF_FALLBACK_FONT_NAME);
        }
    }
    
    return TRUE;
    
fail:
    de_unregister_exports();
    return FALSE;
}


void de_deinit()
{
    gr_unregister_engine("de");
    de_unregister_exports();
    de_deinit_styles();
}


/*}}}*/

