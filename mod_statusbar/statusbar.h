/*
 * ion/mod_statusbar/statusbar.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_STATUSBAR_STATUSBAR_H
#define ION_MOD_STATUSBAR_STATUSBAR_H

#include <libtu/ptrlist.h>
#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/manage.h>
#include <ioncore/sizehint.h>


#define STATUSBAR_NX_STR "?"


typedef enum{
    WSBELEM_ALIGN_LEFT=0,
    WSBELEM_ALIGN_CENTER=1,
    WSBELEM_ALIGN_RIGHT=2
} WSBElemAlign;


typedef enum{
    WSBELEM_NONE=0,
    WSBELEM_TEXT=1,
    WSBELEM_METER=2,
    WSBELEM_STRETCH=3,
    WSBELEM_FILLER=4,
    WSBELEM_SYSTRAY=5
} WSBElemType;
  

INTRSTRUCT(WSBElem);

DECLSTRUCT(WSBElem){
    int type;
    int align;
    int stretch;
    int text_w;
    char *text;
    int max_w;
    char *tmpl;
    StringId meter;
    StringId attr;
    int zeropad;
    int x;
    PtrList *traywins;
};

INTRCLASS(WStatusBar);

DECLCLASS(WStatusBar){
    WWindow wwin;
    GrBrush *brush;
    WSBElem *elems;
    int nelems;
    int natural_w, natural_h;
    int filleridx;
    WStatusBar *sb_next, *sb_prev;
    PtrList *traywins;
    bool systray_enabled;
};

extern bool statusbar_init(WStatusBar *p, WWindow *parent, 
                           const WFitParams *fp);
extern WStatusBar *create_statusbar(WWindow *parent, const WFitParams *fp);
extern void statusbar_deinit(WStatusBar *p);

extern WRegion *statusbar_load(WWindow *par, const WFitParams *fp, 
                               ExtlTab tab);

extern void statusbar_set_natural_w(WStatusBar *p, const char *str);
extern void statusbar_size_hints(WStatusBar *p, WSizeHints *h);
extern void statusbar_updategr(WStatusBar *p);
extern void statusbar_set_contents(WStatusBar *sb, ExtlTab t);

extern void statusbar_set_template(WStatusBar *sb, const char *tmpl);
extern void statusbar_set_template_table(WStatusBar *sb, ExtlTab t);
extern ExtlTab statusbar_get_template_table(WStatusBar *sb);

extern WStatusBar *mod_statusbar_find_suitable(WClientWin *cwin,
                                               const WManageParams *param);

#endif /* ION_MOD_STATUSBAR_STATUSBAR_H */
