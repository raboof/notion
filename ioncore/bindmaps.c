/*
 * ion/ioncore/bindmaps.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <X11/XKBlib.h>
#include <libtu/rb.h>
#include <libextl/extl.h>
#include "common.h"
#include "conf-bindings.h"
#include "binding.h"
#include "framep.h"
#include "bindmaps.h"
#include "global.h"
#include "regbind.h"


/*
 * This file contains higher-level bindmap management code
 */


WBindmap *ioncore_screen_bindmap=NULL;
WBindmap *ioncore_mplex_bindmap=NULL;
WBindmap *ioncore_mplex_toplevel_bindmap=NULL;
WBindmap *ioncore_frame_bindmap=NULL;
WBindmap *ioncore_frame_toplevel_bindmap=NULL;
WBindmap *ioncore_frame_floating_bindmap=NULL;
WBindmap *ioncore_frame_tiled_bindmap=NULL;
WBindmap *ioncore_frame_transient_bindmap=NULL;
WBindmap *ioncore_moveres_bindmap=NULL;
WBindmap *ioncore_group_bindmap=NULL;
WBindmap *ioncore_groupcw_bindmap=NULL;
WBindmap *ioncore_groupws_bindmap=NULL;
WBindmap *ioncore_clientwin_bindmap=NULL;

static Rb_node known_bindmaps=NULL;

static StringIntMap frame_areas[]={
    {"border",      FRAME_AREA_BORDER},
    {"tab",         FRAME_AREA_TAB},
    {"empty_tab",   FRAME_AREA_TAB},
    {"client",      FRAME_AREA_CLIENT},
    END_STRINGINTMAP
};


#define DO_FREE(X, Y)                                       \
    if(ioncore_ ## X ## _bindmap!=NULL){                    \
        ioncore_free_bindmap(Y, ioncore_ ## X ## _bindmap); \
        ioncore_ ## X ## _bindmap=NULL;                     \
    }

void ioncore_deinit_bindmaps()
{
    DO_FREE(screen, "WScreen");
    DO_FREE(mplex, "WMPlex");
    DO_FREE(mplex_toplevel, "WMPlex.toplevel");
    DO_FREE(frame, "WFrame");
    DO_FREE(frame_toplevel, "WFrame.toplevel");
    DO_FREE(frame_floating, "WFrame.floating");
    DO_FREE(frame_tiled, "WFrame.tiled");
    DO_FREE(frame_transient, "WFrame.transient");
    DO_FREE(moveres, "WMoveresMode");
    DO_FREE(group, "WGroup");
    DO_FREE(groupcw, "WGroupCW");
    DO_FREE(groupws, "WGroupWS");
    DO_FREE(clientwin, "WClientWin");
    rb_free_tree(known_bindmaps);
    known_bindmaps=NULL;
}


#define DO_ALLOC(X, Y, Z)                                  \
    ioncore_ ## X ## _bindmap=ioncore_alloc_bindmap(Y, Z); \
    if(ioncore_ ## X ## _bindmap==NULL)                    \
        return FALSE;

bool ioncore_init_bindmaps()
{
    known_bindmaps=make_rb();

    if(known_bindmaps==NULL)
        return FALSE;

    DO_ALLOC(screen, "WScreen", NULL);
    DO_ALLOC(mplex, "WMPlex", NULL);
    DO_ALLOC(mplex_toplevel, "WMPlex.toplevel", NULL);
    DO_ALLOC(frame, "WFrame", frame_areas);
    DO_ALLOC(frame_toplevel, "WFrame.toplevel", frame_areas);
    DO_ALLOC(frame_floating, "WFrame.floating", frame_areas);
    DO_ALLOC(frame_tiled, "WFrame.tiled", frame_areas);
    DO_ALLOC(frame_transient, "WFrame.transient", frame_areas);
    DO_ALLOC(moveres, "WMoveresMode", NULL);
    DO_ALLOC(group, "WGroup", NULL);
    DO_ALLOC(groupcw, "WGroupCW", NULL);
    DO_ALLOC(groupws, "WGroupWS", NULL);
    DO_ALLOC(clientwin, "WClientWin", NULL);

    return TRUE;
}



void ioncore_refresh_bindmaps()
{
    Rb_node node;

    ioncore_update_modmap();

    rb_traverse(node,known_bindmaps){
        bindmap_refresh((WBindmap*)rb_val(node));
    }
}


WBindmap *ioncore_alloc_bindmap(const char *name, const StringIntMap *areas)
{
    WBindmap *bm=create_bindmap();

    if(bm==NULL)
        return NULL;

    bm->areamap=areas;

    if(!rb_insert(known_bindmaps, name, bm)){
        bindmap_destroy(bm);
        return NULL;
    }

    return bm;
}


WBindmap *ioncore_alloc_bindmap_frame(const char *name)
{
    return ioncore_alloc_bindmap(name, frame_areas);
}


void ioncore_free_bindmap(const char *name, WBindmap *bm)
{
    int found=0;
    Rb_node node;

    node=rb_find_key_n(known_bindmaps, name, &found);
    assert(found!=0 && rb_val(node)==(void*)bm);

    rb_delete_node(node);
    bindmap_destroy(bm);
}


WBindmap *ioncore_lookup_bindmap(const char *name)
{
    int found=0;
    Rb_node node;

    node=rb_find_key_n(known_bindmaps, name, &found);

    if(found==0)
        return NULL;

    return (WBindmap*)rb_val(node);
}


EXTL_EXPORT
bool ioncore_do_defbindings(const char *name, ExtlTab tab)
{
    WBindmap *bm=ioncore_lookup_bindmap(name);
fprintf(stderr, "For %s\n", name);
    if(bm==NULL){
        warn("Unknown bindmap %s.", name);
        return FALSE;
    }
    return bindmap_defbindings(bm, tab, FALSE);
}


EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_do_getbindings()
{
    Rb_node node;
    ExtlTab tab;

    tab=extl_create_table();

    rb_traverse(node, known_bindmaps){
        ExtlTab bmtab=bindmap_getbindings((WBindmap*)rb_val(node));
        extl_table_sets_t(tab, (const char*)node->k.key, bmtab);
        extl_unref_table(bmtab);
    }

    return tab;
}


WBindmap *ioncore_create_cycle_bindmap(uint kcb, uint state,
                                       ExtlFn cycle, ExtlFn bcycle)
{
    WBindmap *bindmap=create_bindmap();
    WBinding b;

    if(bindmap==NULL)
        return NULL;

    b.ksb=XkbKeycodeToKeysym(ioncore_g.dpy, kcb, 0, 0);
    b.kcb=kcb;
    b.state=state;
    b.act=BINDING_KEYPRESS;
    b.area=0;
    b.wait=FALSE;
    b.submap=NULL;
    b.func=extl_ref_fn(cycle);

    if(!bindmap_add_binding(bindmap, &b)){
        extl_unref_fn(b.func);
        bindmap_destroy(bindmap);
        return NULL;
    }

    if((b.state&ShiftMask)==0 && bcycle!=extl_fn_none()){
        b.func=extl_ref_fn(bcycle);
        b.state|=ShiftMask;
        bindmap_add_binding(bindmap, &b);
    }

    return bindmap;
}


WBindmap *region_add_cycle_bindmap(WRegion *reg, uint kcb, uint state,
                                   ExtlFn cycle, ExtlFn bcycle)
{
    WBindmap *bindmap=ioncore_create_cycle_bindmap(kcb, state, cycle, bcycle);

    if(bindmap!=NULL){
        if(!region_add_bindmap(reg, bindmap)){
            bindmap_destroy(bindmap);
            return NULL;
        }
    }

    return bindmap;
}

