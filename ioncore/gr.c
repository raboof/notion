/*
 * ion/ioncore/gr.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libextl/readconfig.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "global.h"
#include "modules.h"
#include "gr.h"


/*{{{ Lookup and registration */

INTRSTRUCT(GrEngine);

DECLSTRUCT(GrEngine){
    char *name;
    GrGetBrushFn *fn;
    GrEngine *next, *prev;
};


static GrEngine *engines=NULL, *current_engine=NULL;


bool gr_register_engine(const char *engine, GrGetBrushFn *fn)
{
    GrEngine *eng;
    
    if(engine==NULL || fn==NULL)
        return FALSE;
    
    eng=ALLOC(GrEngine);
    
    if(eng==NULL)
        return FALSE;
    
    eng->name=scopy(engine);
    
    if(eng->name==NULL){
        free(eng);
        return FALSE;
    }
    
    eng->fn=fn;
    
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
 * Future requests for ``brushes'' are to be forwarded to the drawing engine
 * \var{engine}. If no engine of such name is known, a module with that name
 * is attempted to be loaded. This function is only intended to be called from
 * colour scheme etc. configuration files and can not be used to change the
 * look of existing objects; for that use \fnref{gr.read_config}.
 */
EXTL_EXPORT_AS(gr, select_engine)
bool gr_select_engine(const char *engine)
{
    if(engine==NULL)
        return FALSE;
    
    if(gr_do_select_engine(engine))
        return TRUE;
    
    if(!ioncore_load_module(engine))
        return FALSE;
    
    if(!gr_do_select_engine(engine)){
        warn(TR("Drawing engine %s is not registered!"), engine);
        return FALSE;
    }
    
    return TRUE;
}


GrBrush *gr_get_brush(Window win, WRootWin *rootwin, const char *style)
{
    GrEngine *eng=(current_engine!=NULL ? current_engine : engines);
    GrBrush *ret;
    
    if(eng==NULL || eng->fn==NULL)
        return NULL;
    
    ret=(eng->fn)(win, rootwin, style);
    
    if(ret==NULL)
        warn(TR("Unable to find brush for style '%s'."), style);
        
    return ret;
}


/*}}}*/


/*{{{ Scoring */


static GrAttr star_id=STRINGID_NONE;


static int cmp(const void *a_, const void *b_)
{
    StringId a=*(const StringId*)a_;
    StringId b=((const GrAttrScore*)b_)->attr;
    
    return (a < b ? -1 : ((a == b) ? 0 : 1));
}


static uint scorefind(const GrStyleSpec *attr, const GrAttrScore *spec)
{
    GrAttrScore *res;
    
    if(attr->attrs==NULL)
        return 0;
    
    if(star_id==STRINGID_NONE)
        star_id=stringstore_alloc("*");
    
    if(spec->attr==star_id){
        /* Since every item occurs only once on the list, with a score,
         * return the score of the star in the spec, instead of one.
         */
        return spec->score;
    }
    
    res=bsearch(&spec->attr, attr->attrs, attr->n, sizeof(GrAttrScore), cmp);
    
    return (res==NULL ? 0 : 2*res->score);
}


uint gr_stylespec_score2(const GrStyleSpec *spec, const GrStyleSpec *attr1, 
                         const GrStyleSpec *attr2)
{
    uint score=0;
    uint i;
    
    for(i=0; i<spec->n; i++){
        uint sc=scorefind(attr1, &spec->attrs[i]);
        
        if(attr2!=NULL)
            sc=MAXOF(sc, scorefind(attr2, &spec->attrs[i]));
        
        if(sc==0){
            score=0;
            break;
        }
        
        score+=sc;
    }
    
    return score;
}


uint gr_stylespec_score(const GrStyleSpec *spec, const GrStyleSpec *attr)
{
    return gr_stylespec_score2(spec, attr, NULL);
}


static uint count_dashes(const char *str)
{
    uint n=0;
    
    if(str!=NULL){
        while(1){
            const char *p=strchr(str, '-');
            if(p==NULL)
                break;
            n++;
            str=p+1;
        }
    }
    
    return n;
}
    
    
bool gr_stylespec_load_(GrStyleSpec *spec, const char *str, bool no_order_score)
{
    uint score=(no_order_score ? 1 : count_dashes(str)+1);
    
    gr_stylespec_init(spec);
    
    while(str!=NULL){
        GrAttr a;
        const char *p=strchr(str, '-');
    
        if(p==NULL){
            a=stringstore_alloc(str);
            str=p;
        }else{
            a=stringstore_alloc_n(str, p-str);
            str=p+1;
        }
        
        if(a==STRINGID_NONE)
            goto fail;
            
        if(!gr_stylespec_add(spec, a, score))
            goto fail;
            
        stringstore_free(a);
        
        if(!no_order_score)
            score--;
    }
    
    return TRUE;
    
fail:
    gr_stylespec_unalloc(spec);
    
    return FALSE;
}


bool gr_stylespec_load(GrStyleSpec *spec, const char *str)
{
    return gr_stylespec_load_(spec, str, FALSE);
}


void gr_stylespec_unalloc(GrStyleSpec *spec)
{
    uint i;
    
    for(i=0; i<spec->n; i++)
        stringstore_free(spec->attrs[i].attr);
    
    if(spec->attrs!=NULL){
        free(spec->attrs);
        spec->attrs=NULL;
    }
    
    spec->n=0;
}


void gr_stylespec_init(GrStyleSpec *spec)
{
    spec->attrs=NULL;
    spec->n=0;
}


static bool gr_stylespec_find_(const GrStyleSpec *spec, GrAttr a, int *idx_ge)
{
    bool found=FALSE;
    uint i;
    
    for(i=0; i<spec->n; i++){
        if(spec->attrs[i].attr>=a){
            found=(spec->attrs[i].attr==a);
            break;
        }
    }
    
    *idx_ge=i;
    return found;
}


bool gr_stylespec_isset(const GrStyleSpec *spec, GrAttr a)
{
    int idx_ge;
    
    return gr_stylespec_find_(spec, a, &idx_ge);
}


bool gr_stylespec_add(GrStyleSpec *spec, GrAttr a, uint score)
{
    static const uint sz=sizeof(GrAttrScore);
    GrAttrScore *idsn;
    int idx_ge;
    
    if(a==GRATTR_NONE || score==0)
        return TRUE;
    
    if(gr_stylespec_find_(spec, a, &idx_ge)){
        spec->attrs[idx_ge].score+=score;
        return TRUE;
    }
    
    idsn=(GrAttrScore*)realloc(spec->attrs, (spec->n+1)*sz);
    
    if(idsn==NULL)
        return FALSE;
        
    stringstore_ref(a);
        
    memmove(idsn+idx_ge+1, idsn+idx_ge, (spec->n-idx_ge)*sz);
    
    idsn[idx_ge].attr=a;
    idsn[idx_ge].score=score;
    spec->attrs=idsn;
    spec->n++;
    
    return TRUE;
}


bool gr_stylespec_set(GrStyleSpec *spec, GrAttr a)
{
    return gr_stylespec_add(spec, a, 1);
}


void gr_stylespec_unset(GrStyleSpec *spec, GrAttr a)
{
    static const uint sz=sizeof(GrAttrScore);
    GrAttrScore *idsn;
    int idx_ge;
    
    if(a==GRATTR_NONE)
        return;
    
    if(!gr_stylespec_find_(spec, a, &idx_ge))
        return;
    
    stringstore_free(spec->attrs[idx_ge].attr);
    
    memmove(spec->attrs+idx_ge, spec->attrs+idx_ge+1,
            (spec->n-idx_ge-1)*sz);
            
    spec->n--;
    
    idsn=(GrAttrScore*)realloc(spec->attrs, (spec->n)*sz);
    
    if(idsn!=NULL || spec->n==0)
        spec->attrs=idsn;
}


static bool gr_stylespec_do_init_from(GrStyleSpec *dst, const GrStyleSpec *src)
{
    uint i;
    
    if(src->n==0)
        return TRUE;
        
    dst->attrs=ALLOC_N(GrAttrScore, src->n);
    
    if(dst->attrs==NULL)
        return FALSE;
    
    for(i=0; i<src->n; i++){
        dst->attrs[i]=src->attrs[i];
        stringstore_ref(dst->attrs[i].attr);
    }
    
    dst->n=src->n;
    
    return TRUE;
}


bool gr_stylespec_append(GrStyleSpec *dst, const GrStyleSpec *src)
{
    uint i;
    bool ok=TRUE;
    
    if(dst->attrs==NULL){
        ok=gr_stylespec_do_init_from(dst, src);
    }else{
        for(i=0; i<src->n; i++){
            if(!gr_stylespec_add(dst, src->attrs[i].attr, src->attrs[i].score))
                ok=FALSE;
        }
    }
    
    return ok;
}


bool gr_stylespec_equals(const GrStyleSpec *s1, const GrStyleSpec *s2)
{
    uint i;
    
    if(s1->n!=s2->n)
        return FALSE;
        
    for(i=0; i<s1->n; i++){
        if(s1->attrs[i].attr!=s2->attrs[i].attr)
            return FALSE;
    }
    
    return TRUE;
}
    

/*}}}*/


/*{{{ Init, deinit */


bool grbrush_init(GrBrush *UNUSED(brush))
{
    return TRUE;
}


void grbrush_deinit(GrBrush *UNUSED(brush))
{
}


void grbrush_release(GrBrush *brush)
{
    CALL_DYN(grbrush_release, brush, (brush));
}


GrBrush *grbrush_get_slave(GrBrush *brush, WRootWin *rootwin, 
                           const char *style)
{
    GrBrush *slave=NULL;
    CALL_DYN_RET(slave, GrBrush*, grbrush_get_slave, brush,
                 (brush, rootwin, style));
    return slave;
}


/*}}}*/


/*{{{ Dynfuns/begin/end/replay */


void grbrush_begin(GrBrush *brush, const WRectangle *geom, int flags)
{
    CALL_DYN(grbrush_begin, brush, (brush, geom, flags));
}


void grbrush_end(GrBrush *brush)
{
    CALL_DYN(grbrush_end, brush, (brush));
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


DYNFUN bool grbrush_get_extra(GrBrush *brush, const char *key, 
                              char type, void *data)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, grbrush_get_extra, brush,
                 (brush, key, type, data));
    return ret;
}


/*}}}*/


/*{{{ Dynfuns/Borders */


void grbrush_draw_border(GrBrush *brush, const WRectangle *geom)
{
    CALL_DYN(grbrush_draw_border, brush, (brush, geom));
}


void grbrush_draw_borderline(GrBrush *brush, const WRectangle *geom,
                             GrBorderLine line)
{
    CALL_DYN(grbrush_draw_borderline, brush, (brush, geom, line));
}


/*}}}*/


/*{{{ Dynfuns/Strings */


void grbrush_draw_string(GrBrush *brush, int x, int y,
                         const char *str, int len, bool needfill)
{
    CALL_DYN(grbrush_draw_string, brush, (brush, x, y, str, len, needfill));
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


void grbrush_draw_textbox(GrBrush *brush, const WRectangle *geom,
                          const char *text, bool needfill)
{
    CALL_DYN(grbrush_draw_textbox, brush, (brush, geom, text, needfill));
}

void grbrush_draw_textboxes(GrBrush *brush, const WRectangle *geom,
                            int n, const GrTextElem *elem, 
                            bool needfill)
{
    CALL_DYN(grbrush_draw_textboxes, brush, (brush, geom, n, elem, needfill));
}


/*}}}*/


/*{{{ Dynfuns/Misc */


void grbrush_set_window_shape(GrBrush *brush, bool rough,
                              int n, const WRectangle *rects)
{
    CALL_DYN(grbrush_set_window_shape, brush, (brush, rough, n, rects));
}


void grbrush_enable_transparency(GrBrush *brush, GrTransparency tr)
{
    CALL_DYN(grbrush_enable_transparency, brush, (brush, tr));
}


void grbrush_fill_area(GrBrush *brush, const WRectangle *geom)
{
    CALL_DYN(grbrush_fill_area, brush, (brush, geom));
}


void grbrush_clear_area(GrBrush *brush, const WRectangle *geom)
{
    CALL_DYN(grbrush_clear_area, brush, (brush, geom));
}


void grbrush_init_attr(GrBrush *brush, const GrStyleSpec *spec)
{
    CALL_DYN(grbrush_init_attr, brush, (brush, spec));
}


void grbrush_set_attr(GrBrush *brush, GrAttr attr)
{
    CALL_DYN(grbrush_set_attr, brush, (brush, attr));
}


void grbrush_unset_attr(GrBrush *brush, GrAttr attr)
{
    CALL_DYN(grbrush_unset_attr, brush, (brush, attr));
}


/*}}}*/


/*{{{ ioncore_read_config/refresh */


/*EXTL_DOC
 * Read drawing engine configuration file \file{look.lua}.
 */
EXTL_EXPORT_AS(gr, read_config)
void gr_read_config()
{
    extl_read_config("look", NULL, TRUE);
    
    /* If nothing has been loaded, try the default engine with
     * default settings.
     */
    if(engines==NULL){
        warn(TR("No drawing engines loaded, trying \"de\"."));
        gr_select_engine("de");
    }
}


/*EXTL_DOC
 * Refresh objects' brushes to update them to use newly loaded style.
 */
EXTL_EXPORT_AS(gr, refresh)
void gr_refresh()
{
    WRootWin *rootwin;
    
    FOR_ALL_ROOTWINS(rootwin){
        region_updategr((WRegion*)rootwin);
    }
}


/*}}}*/


/*{{{ Class implementation */


static DynFunTab grbrush_dynfuntab[]={
    END_DYNFUNTAB
};
                                       

IMPLCLASS(GrBrush, Obj, grbrush_deinit, grbrush_dynfuntab);


/*}}}*/

