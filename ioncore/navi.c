/*
 * ion/ioncore/navi.c
 *
 * Copyright (c) Tuomo Valkonen 2006-2009.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>

#include "common.h"
#include "extlconv.h"
#include "region.h"
#include "navi.h"


WRegion *region_navi_first(WRegion *reg, WRegionNavi nh,
                           WRegionNaviData *data)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_navi_first, reg,
                 (reg, nh, data));
    return ret;
}


WRegion *region_navi_next(WRegion *reg, WRegion *mgd, WRegionNavi nh,
                          WRegionNaviData *data)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_navi_next, reg,
                 (reg, mgd, nh, data));
    return ret;
}


bool ioncore_string_to_navi(const char *str, WRegionNavi *nh)
{
    if(str==NULL){
        warn(TR("Invalid parameter."));
        return FALSE;
    }

    if(!strcmp(str, "any")){
        *nh=REGION_NAVI_ANY;
    }else if (!strcmp(str, "end") ||
              !strcmp(str, "last") ||
              !strcmp(str, "next")){
        *nh=REGION_NAVI_END;
    }else if (!strcmp(str, "beg") ||
              !strcmp(str, "first") ||
              !strcmp(str, "prev")){
        *nh=REGION_NAVI_BEG;
    }else if(!strcmp(str, "left")){
        *nh=REGION_NAVI_LEFT;
    }else if(!strcmp(str, "right")){
        *nh=REGION_NAVI_RIGHT;
    }else if(!strcmp(str, "top") ||
             !strcmp(str, "above") ||
             !strcmp(str, "up")){
        *nh=REGION_NAVI_TOP;
    }else if(!strcmp(str, "bottom") ||
             !strcmp(str, "below") ||
             !strcmp(str, "down")){
        *nh=REGION_NAVI_BOTTOM;
    }else{
        warn(TR("Invalid direction parameter."));
        return FALSE;
    }

    return TRUE;
}


WRegionNavi ioncore_navi_reverse(WRegionNavi nh)
{
    if(nh==REGION_NAVI_BEG)
        return REGION_NAVI_END;
    else if(nh==REGION_NAVI_END)
        return REGION_NAVI_BEG;
    else if(nh==REGION_NAVI_LEFT)
        return REGION_NAVI_RIGHT;
    else if(nh==REGION_NAVI_RIGHT)
        return REGION_NAVI_LEFT;
    else if(nh==REGION_NAVI_TOP)
        return REGION_NAVI_BOTTOM;
    else if(nh==REGION_NAVI_BOTTOM)
        return REGION_NAVI_TOP;
    else
        return REGION_NAVI_ANY;
}


DECLSTRUCT(WRegionNaviData){
    WRegionNavi nh;
    bool descend;
    ExtlFn ascend_filter;
    ExtlFn descend_filter;
    WRegion *startpoint;
    bool nowrap;
    Obj *no_ascend;
    Obj *no_descend;
    bool nofront;
};


static bool may_ascend(WRegion *to, WRegion *from, WRegionNaviData *data)
{
    if(data->ascend_filter!=extl_fn_none()){
        bool r, v;
        extl_protect(NULL);
        r=extl_call(data->ascend_filter, "oo", "b", to, from, &v);
        extl_unprotect(NULL);
        return (r && v);
    }else if(data->no_ascend!=NULL){
        return (data->no_ascend!=(Obj*)from);
    }else{
        /* Set to TRUE for cycling out of nested workspaces etc. */
        return !OBJ_IS(from, WMPlex);
    }
}


static bool may_descend(WRegion *to, WRegion *from, WRegionNaviData *data)
{
    if(data->descend_filter!=extl_fn_none()){
        bool r, v;
        extl_protect(NULL);
        r=extl_call(data->descend_filter, "oo", "b", to, from, &v);
        extl_unprotect(NULL);
        return (r && v);
    }else if(data->no_descend!=NULL){
        return (data->no_descend!=(Obj*)from);
    }else{
        /* Set to TRUE for cycling into nested workspaces etc. */
        return !OBJ_IS(to, WMPlex);
    }
}


static WRegion *region_navi_descend(WRegion *reg, WRegionNaviData *data)
{
    if(data->descend){
        return region_navi_first(reg, data->nh, data);
    }else{
        WRegion *nxt;

        data->descend=TRUE;
        data->nh=ioncore_navi_reverse(data->nh);

        nxt=region_navi_first(reg, data->nh, data);

        data->descend=FALSE;
        data->nh=ioncore_navi_reverse(data->nh);

        return nxt;
    }
}


WRegion *region_navi_cont(WRegion *reg, WRegion *res, WRegionNaviData *data)
{
    if(res==NULL){
        if(data->descend){
            return (reg==data->startpoint ? NULL : reg);
        }else{
            WRegion *mgr=REGION_MANAGER(reg);
            WRegion *nxt=NULL;

            if(mgr!=NULL && may_ascend(mgr, reg, data)){
                if(data->nowrap){
                    /* tail-recursive case */
                    return region_navi_next(mgr, reg, data->nh, data);
                }else{
                    nxt=region_navi_next(mgr, reg, data->nh, data);
                }
            }

            if(nxt==NULL && !data->nowrap){
                /* wrap */
                nxt=region_navi_descend(reg, data);
            }

            return nxt;
        }
    }else{
        if(may_descend(res, reg, data)){
            return region_navi_descend(res, data);
        }else{
            return res;
        }
    }
}


static bool get_param(WRegionNaviData *data, const char *dirstr, ExtlTab param)
{
    if(!ioncore_string_to_navi(dirstr, &data->nh))
        return FALSE;

    data->ascend_filter=extl_fn_none();
    data->descend_filter=extl_fn_none();
    data->no_ascend=NULL;
    data->no_descend=NULL;

    extl_table_gets_o(param, "no_ascend", &data->no_ascend);
    extl_table_gets_o(param, "no_descend", &data->no_descend);
    extl_table_gets_f(param, "ascend_filter", &data->ascend_filter);
    extl_table_gets_f(param, "descend_filter", &data->descend_filter);
    data->nowrap=extl_table_is_bool_set(param, "nowrap");
    data->nofront=extl_table_is_bool_set(param, "nofront");

    return TRUE;
}


static WRegion *release(WRegionNaviData *data, WRegion *res)
{
    extl_unref_fn(data->ascend_filter);
    extl_unref_fn(data->descend_filter);

    return res;
}


/*EXTL_DOC
 * Find region next from \var{reg} in direction \var{dirstr}
 * (\codestr{up}, \codestr{down}, \codestr{left}, \codestr{right},
 * \codestr{next}, \codestr{prev}, or \codestr{any}). The table \var{param}
 * may contain the boolean field \var{nowrap}, instructing not to wrap
 * around, and the \type{WRegion}s \var{no_ascend} and \var{no_descend},
 * and boolean functions \var{ascend_filter} and \var{descend_filter}
 * on \var{WRegion} pairs (\var{to}, \var{from}), are used to decide when
 * to descend or ascend into another region.
 */
EXTL_EXPORT
WRegion *ioncore_navi_next(WRegion *reg, const char *dirstr, ExtlTab param)
{
    WRegionNaviData data;
    WRegion *mgr;

    if(reg==NULL){
        /* ??? */
        return NULL;
    }

    if(!get_param(&data, dirstr, param))
        return NULL;

    mgr=REGION_MANAGER(reg);

    if(mgr==NULL)
        return FALSE;

    data.startpoint=reg;
    data.descend=FALSE;

    return release(&data, region_navi_next(mgr, reg, data.nh, &data));
}


/*EXTL_DOC
 * Find first region within \var{reg} in direction \var{dirstr}.
 * For information on \var{param}, see \fnref{ioncore.navi_next}.
 */
EXTL_EXPORT
WRegion *ioncore_navi_first(WRegion *reg, const char *dirstr, ExtlTab param)
{
    WRegionNaviData data;

    if(reg==NULL)
        return NULL;

    if(!get_param(&data, dirstr, param))
        return NULL;

    data.startpoint=reg;
    data.descend=TRUE;

    return release(&data, region_navi_first(reg, data.nh, &data));
}


static WRegion *do_goto(WRegion *res)
{
    if(res!=NULL)
        region_goto(res);

    return res;
}


/*EXTL_DOC
 * Go to region next from \var{reg} in direction \var{dirstr}.
 * For information on \var{param}, see \fnref{ioncore.navi_next}.
 * Additionally this function supports the boolean \var{nofront}
 * field, for not bringing the object to front.
 */
EXTL_EXPORT
WRegion *ioncore_goto_next(WRegion *reg, const char *dirstr, ExtlTab param)
{
    return do_goto(ioncore_navi_next(reg, dirstr, param));
}


/*EXTL_DOC
 * Go to first region within \var{reg} in direction \var{dirstr}.
 * For information on \var{param}, see \fnref{ioncore.navi_next}.
 * Additionally this function supports the boolean \var{nofront} field,
 * for not bringing the object to front.
 */
EXTL_EXPORT
WRegion *ioncore_goto_first(WRegion *reg, const char *dirstr, ExtlTab param)
{
    return do_goto(ioncore_navi_first(reg, dirstr, param));
}


