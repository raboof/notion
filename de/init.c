/*
 * ion/de/init.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
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
#include "private.h"
#include "init.h"
#include "exports.h"


/*{{{ Style specifications */


static bool get_spec(ExtlTab tab, const char *name, GrStyleSpec *spec,
                     char **pat_ret)
{
    char *str;
    bool res;
    
    if(!extl_table_gets_s(tab, name, &str))
        return FALSE;
        
    res=gr_stylespec_load(spec, str);
    
    if(pat_ret==NULL)
        free(str);
    else
        *pat_ret=str;
    
    return res;
}


/*}}}*/


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


void de_get_border_sides(uint *ret, ExtlTab tab)
{
    char *style=NULL;
    
    if(!extl_table_gets_s(tab, "border_sides", &style))
        return;
    
    if(strcmp(style, "all")==0)
        *ret=DEBORDER_ALL;
    else if(strcmp(style, "tb")==0)
        *ret=DEBORDER_TB;
    else if(strcmp(style, "lr")==0)
        *ret=DEBORDER_LR;
    else
        warn(TR("Unknown border side configuration \"%s\"."), style);
    
    free(style);
}


void de_get_border(DEBorder *border, ExtlTab tab)
{
    de_get_border_val(&(border->sh), tab, "shadow_pixels");
    de_get_border_val(&(border->hl), tab, "highlight_pixels");
    de_get_border_val(&(border->pad), tab, "padding_pixels");
    de_get_border_style(&(border->style), tab);
    de_get_border_sides(&(border->sides), tab);
}


/*}}}*/


/*{{{ Colours */


static bool de_get_colour_(WRootWin *rootwin, DEColour *ret, 
                           ExtlTab tab, const char *what, 
                           DEColour substitute, DEColour inherit)
{
    char *name=NULL;
    bool set=FALSE;
    
    if(extl_table_gets_s(tab, what, &name)){
        if(strcmp(name, "inherit")==0){
            set=de_duplicate_colour(rootwin, inherit, ret);
        }else{
            set=de_alloc_colour(rootwin, ret, name);
    
            if(!set)
                warn(TR("Unable to allocate colour \"%s\"."), name);
        }
        free(name);
    }
    
    if(!set)
        de_duplicate_colour(rootwin, substitute, ret);
    
    return set;
}


static bool de_get_colour(WRootWin *rootwin, DEColour *ret, 
                          ExtlTab tab, const char *what, DEColour substitute)
{
    return de_get_colour_(rootwin, ret, tab, what, substitute, substitute);
}
                          

void de_get_colour_group(WRootWin *rootwin, DEColourGroup *cg, 
                         ExtlTab tab, DEStyle *based_on)
{
    bool bgset;
    DEColour padinh;
    
    de_get_colour(rootwin, &(cg->hl), tab, "highlight_colour",
                  (based_on ? based_on->cgrp.hl : DE_WHITE(rootwin)));
    de_get_colour(rootwin, &(cg->sh), tab, "shadow_colour",
                  (based_on ? based_on->cgrp.sh : DE_WHITE(rootwin)));
    de_get_colour(rootwin, &(cg->fg), tab, "foreground_colour",
                  (based_on ? based_on->cgrp.fg : DE_WHITE(rootwin)));
    bgset=de_get_colour(rootwin, &(cg->bg), tab, "background_colour",
                        (based_on ? based_on->cgrp.bg : DE_BLACK(rootwin)));
                        
    padinh=(based_on ? based_on->cgrp.pad : DE_WHITE(rootwin));
    
    de_get_colour_(rootwin, &(cg->pad), tab, "padding_colour", 
                   (bgset ? cg->bg : padinh), padinh);
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
        GrStyleSpec spec;
        
        if(!extl_table_geti_t(tab, i+1, &sub))
            goto err;
        
        if(!get_spec(sub, "substyle_pattern", &spec, NULL)){
            extl_unref_table(sub);
            goto err;
        }
        
        style->extra_cgrps[i-nfailed].spec=spec;
        
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


/*{{{ Extras filter/copy */


static const char * const known_values[]={
    "based_on",
    "font",
    "shadow_pixels",
    "highlight_pixels",
    "padding_pixels",
    "border_style",
    "border_sides",
    "spacing",
    "foreground_colour",
    "background_colour",
    "shadow_colour",
    "highlight_colour",
    "padding_colour",
    "text_align",
    NULL
};


static bool filter_extras_iter_fn(ExtlAny k, ExtlAny v, void *p)
{
    ExtlTab *tgt=(ExtlTab*)p;
    const char *s;
    int i;
    
    if(k.type!='s' && k.type!='S')
        return TRUE;
    
    for(i=0; known_values[i]; i++){
        if(strcmp(known_values[i], k.value.s)==0)
            return TRUE;
    }
    
    if(*tgt==extl_table_none())
        *tgt=extl_create_table();
        
    extl_table_set(*tgt, 'a', 'a', k, v);
    
    return TRUE;
}


static void filter_extras(ExtlTab *tgt, ExtlTab src)
{
    /* Copy any unknown string-keyed values from src to tgt,
     * possibly creating tgt.
     */
    extl_table_iter(src, filter_extras_iter_fn, tgt);
}


/*}}}*/


/*{{{ de_defstyle */


void de_get_nonfont(WRootWin *rootwin, DEStyle *style, ExtlTab tab)
{
    DEStyle *based_on=style->based_on;
    
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
    DEStyle *style, *based_on=NULL;
    int based_on_score=-1;
    char *fnt, *bss;
    uint n;

    if(name==NULL)
        return FALSE;
    
    style=de_create_style(rootwin, name);
    
    if(style==NULL)
        return FALSE;
    
    if(extl_table_gets_s(tab, "based_on", &bss)){
        GrStyleSpec bs;
        
        gr_stylespec_load(&bs, bss);
        
        based_on=de_get_style(rootwin, &bs);
        
        gr_stylespec_unalloc(&bs);
        free(bss);
    }else{
        based_on=de_get_style(rootwin, &style->spec);
    }
    
    if(based_on!=NULL){
        style->based_on=based_on;
        based_on->usecount++;
    }
    
    de_get_nonfont(rootwin, style, tab);

    if(extl_table_gets_s(tab, "font", &fnt)){
        de_load_font_for_style(style, fnt);
        free(fnt);
    }else if(based_on!=NULL && based_on->font!=NULL){
        de_set_font_for_style(style, based_on->font);
    }
    
    if(style->font==NULL)
        de_load_font_for_style(style, CF_FALLBACK_FONT_NAME);
    
    if(based_on!=NULL && 
       gr_stylespec_equals(&based_on->spec, &style->spec)){
       
        /* The new style replaces based_on, so it may be dumped. */
        if(!based_on->is_fallback)
            destyle_dump(based_on);
        
        if(based_on->usecount==1){
            uint nb=based_on->n_extra_cgrps;
            uint ns=style->n_extra_cgrps;
            /* Nothing else is using based_on: optimise and move
             * extra colour groups here, so that based_on can be freed.
             */
            
            if(nb>0){
                DEColourGroup *cgs=ALLOC_N(DEColourGroup, nb+ns);
                
                if(cgs!=NULL){
                    memcpy(cgs, based_on->extra_cgrps, sizeof(DEColourGroup)*nb);
                    memcpy(cgs+nb, style->extra_cgrps, sizeof(DEColourGroup)*ns);
                
                    free(style->extra_cgrps);
                    style->extra_cgrps=cgs;
                    style->n_extra_cgrps=nb+ns;
                
                    free(based_on->extra_cgrps);
                    based_on->extra_cgrps=NULL;
                    based_on->n_extra_cgrps=0;
                    
                }
            }
            
            /* style->extras_table should be none still */
            style->extras_table=based_on->extras_table;
            based_on->extras_table=extl_table_none();
            
            style->based_on=based_on->based_on;
            based_on->based_on=NULL;
            
            destyle_unref(based_on);
        }
        
    }
    
    filter_extras(&style->extras_table, tab);
    
    destyle_add(style);
    
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
EXTL_SAFE
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

