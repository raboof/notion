/*
 * ion/ioncore/mplex.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 * 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_MPLEX_H
#define ION_IONCORE_MPLEX_H

#include <libextl/extl.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "window.h"
#include "attach.h"
#include "manage.h"
#include "rectangle.h"
#include "pholder.h"
#include "sizepolicy.h"


#define MPLEX_ADD_TO_END 0x0001
#define MPLEX_MANAGED_UNVIEWABLE 0x0002

#define MPLEX_MGD_UNVIEWABLE(MPLEX) \
    ((MPLEX)->flags&MPLEX_MANAGED_UNVIEWABLE)


#define MPLEX_ATTACH_SWITCHTO     0x0001 /* switch to region */
#define MPLEX_ATTACH_UNNUMBERED   0x0002 /* do not put on mut.ex list */
#define MPLEX_ATTACH_HIDDEN       0x0004 /* should be hidden */
#define MPLEX_ATTACH_PSEUDOMODAL  0x0008 /* pseudomodal (if modal) */
#define MPLEX_ATTACH_LEVEL        0x0010 /* level field set */
#define MPLEX_ATTACH_GEOM         0x0020 /* geometry field is set */
#define MPLEX_ATTACH_SIZEPOLICY   0x0040 /* size policy field is set */
#define MPLEX_ATTACH_INDEX        0x0080 /* index field is set */
#define MPLEX_ATTACH_WHATEVER     0x0100 /* set REGION_FIT_WHATEVER */
#define MPLEX_ATTACH_PASSIVE      0x0200 /* sets SKIP_FOCUS */


enum{
    MPLEX_CHANGE_SWITCHONLY=0,
    MPLEX_CHANGE_REORDER=1,
    MPLEX_CHANGE_ADD=2,
    MPLEX_CHANGE_REMOVE=3
};


enum{
    MPLEX_STDISP_TL,
    MPLEX_STDISP_TR,
    MPLEX_STDISP_BL,
    MPLEX_STDISP_BR
};


INTRSTRUCT(WMPlexSTDispInfo);
INTRSTRUCT(WMPlexChangedParams);
INTRSTRUCT(WMPlexAttachParams);


DECLSTRUCT(WMPlexSTDispInfo){
    int pos;
    bool fullsize;
};


DECLSTRUCT(WMPlexChangedParams){
    WMPlex *reg;
    int mode;
    bool sw;
    WRegion *sub;
};


#define MPLEXATTACHPARAMS_INIT {0, 0, {0, 0, 0, 0}, 0, 0}

DECLSTRUCT(WMPlexAttachParams){
    int flags;
    int index;
    WRectangle geom;
    WSizePolicy szplcy;
    uint level;
};


DECLCLASS(WMPlex){
    WWindow win;
    int flags;
    
    WStacking *mgd;
    
    int mx_count;
    WLListNode *mx_current;
    WLListNode *mx_list;
    WMPlexPHolder *misc_phs;
    
    Watch stdispwatch;
    WMPlexSTDispInfo stdispinfo;
};


/* Create/destroy */

extern WMPlex *create_mplex(WWindow *parent, const WFitParams *fp);
extern bool mplex_init(WMPlex *mplex, WWindow *parent,
                       const WFitParams *fp);
extern bool mplex_do_init(WMPlex *mplex, WWindow *parent, 
                          const WFitParams *fp, Window win);
extern void mplex_deinit(WMPlex *mplex);

/* Resize and reparent */

extern bool mplex_fitrep(WMPlex *mplex, WWindow *par, const WFitParams *fp);
extern void mplex_do_fit_managed(WMPlex *mplex, WFitParams *fp);
extern void mplex_fit_managed(WMPlex *mplex);

/* Mapping */

extern void mplex_map(WMPlex *mplex);
extern void mplex_unmap(WMPlex *mplex);

/* Attach */

extern WRegion *mplex_attach_simple(WMPlex *mplex, WRegion *reg, 
                                    int flags);

extern WRegion *mplex_attach(WMPlex *mplex, WRegion *reg, ExtlTab param);
extern WRegion *mplex_attach_new_(WMPlex *mplex, WMPlexAttachParams *partmpl,
                                  int mask, ExtlTab param);
extern WRegion *mplex_attach_new(WMPlex *mplex, ExtlTab param);

extern WRegion *mplex_do_attach_pholder(WMPlex *mplex, WMPlexPHolder *ph,
                                        WRegionAttachData *data);
extern WRegion *mplex_do_attach(WMPlex *mplex, WMPlexAttachParams *param,
                                WRegionAttachData *data);
extern WRegion *mplex_do_attach_new(WMPlex *mplex, WMPlexAttachParams *param,
                                    WRegionCreateFn *fn, void *fn_param);

extern void mplex_managed_remove(WMPlex *mplex, WRegion *reg);
extern void mplex_child_removed(WMPlex *mplex, WRegion *sub);

extern bool mplex_rescue_clientwins(WMPlex *mplex, WRescueInfo *info);

extern WPHolder *mplex_prepare_manage(WMPlex *mplex, const WClientWin *cwin,
                                      const WManageParams *param, int redir);

/* Switch */

extern bool mplex_managed_prepare_focus(WMPlex *mplex, WRegion *sub, 
                                        int flags, WPrepareFocusResult *res);
extern bool mplex_do_prepare_focus(WMPlex *mplex, WStacking *disp, 
                                   WStacking *sub, int flags, 
                                   WPrepareFocusResult *res);

extern void mplex_switch_nth(WMPlex *mplex, uint n);
extern void mplex_switch_next(WMPlex *mplex);
extern void mplex_switch_prev(WMPlex *mplex);
extern bool mplex_is_hidden(WMPlex *mplex, WRegion *reg);
extern bool mplex_set_hidden(WMPlex *mplex, WRegion *reg, int sp);

/* Focus */

extern void mplex_do_set_focus(WMPlex *mplex, bool warp);

/* Stacking */

extern bool mplex_managed_rqorder(WMPlex *mplex, WRegion *sub, 
                                  WRegionOrder order);

/* Misc */

extern WRegion *mplex_current(WMPlex *mplex);

extern bool mplex_managed_i(WMPlex *mplex, ExtlFn iterfn);

extern int mplex_mx_count(WMPlex *mplex);
extern WRegion *mplex_mx_nth(WMPlex *mplex, uint n);
extern bool mplex_mx_i(WMPlex *mplex, ExtlFn iterfn);
extern WRegion *mplex_mx_current(WMPlex *mplex);

extern void mplex_call_changed_hook(WMPlex *mplex, WHook *hook, 
                                    int mode, bool sw, WRegion *reg);

extern void mplex_remanage_stdisp(WMPlex *mplex);

/* Note: only the size policy field is changed; actual geometry is not
 * yet changed.
 */
extern void mplex_set_szplcy(WMPlex *mplex, WRegion *sub, WSizePolicy szplcy);
extern WSizePolicy mplex_get_szplcy(WMPlex *mplex, WRegion *sub);

/* Dynfuns */

DYNFUN void mplex_managed_geom(const WMPlex *mplex, WRectangle *geom);
DYNFUN void mplex_size_changed(WMPlex *mplex, bool wchg, bool hchg);
DYNFUN void mplex_managed_changed(WMPlex *mplex, int what, bool sw,
                                  WRegion *mgd);
DYNFUN int mplex_default_index(WMPlex *mplex);

/* Save/load */

extern ExtlTab mplex_get_configuration(WMPlex *mplex);
extern WRegion *mplex_load(WWindow *par, const WFitParams *fp, ExtlTab tab);
extern void mplex_load_contents(WMPlex *frame, ExtlTab tab);


/* Sticky status display support */

extern bool mplex_set_stdisp(WMPlex *mplex, WRegion *stdisp, 
                             const WMPlexSTDispInfo *info);
extern void mplex_get_stdisp(WMPlex *mplex, WRegion **stdisp, 
                             WMPlexSTDispInfo *info);

extern WRegion *mplex_set_stdisp_extl(WMPlex *mplex, ExtlTab t);
extern ExtlTab mplex_get_stdisp_extl(WMPlex *mplex);

DYNFUN void region_manage_stdisp(WRegion *reg, WRegion *stdisp, 
                                 const WMPlexSTDispInfo *info);
DYNFUN void region_unmanage_stdisp(WRegion *reg, bool permanent, bool nofocus);

/* Stacking list stuff */

typedef WStackingIterTmp WMPlexIterTmp;

extern void mplex_iter_init(WMPlexIterTmp *tmp, WMPlex *ws);
extern WRegion *mplex_iter(WMPlexIterTmp *tmp);
extern WStacking *mplex_iter_nodes(WMPlexIterTmp *tmp);

extern WStacking *mplex_get_stacking(WMPlex *ws);
extern WStacking **mplex_get_stackingp(WMPlex *ws);

#define FOR_ALL_MANAGED_BY_MPLEX(MPLEX, VAR, TMP) \
    for(mplex_iter_init(&(TMP), MPLEX),           \
         VAR=mplex_iter(&(TMP));                  \
        VAR!=NULL;                                \
        VAR=mplex_iter(&(TMP)))

#define FOR_ALL_NODES_IN_MPLEX(MPLEX, VAR, TMP) \
    for(mplex_iter_init(&(TMP), MPLEX),         \
         VAR=mplex_iter_nodes(&(TMP));          \
        VAR!=NULL;                              \
        VAR=mplex_iter_nodes(&(TMP)))

extern WStacking *mplex_find_stacking(WMPlex *mplex, WRegion *reg);

#endif /* ION_IONCORE_MPLEX_H */
