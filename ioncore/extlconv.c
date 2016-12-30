/*
 * ion/ioncore/extlconv.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libextl/extl.h>
#include "common.h"
#include "extlconv.h"


/*{{{ Object list */


bool extl_iter_objlist_(ExtlFn fn, ObjIterator *iter, void *st)
{
    Obj *obj;

    while(1){
        obj=iter(st);
        if(obj==NULL)
            break;
        if(!extl_iter_obj(fn, obj))
            return FALSE;
    }

    return TRUE;
}


bool extl_iter_objlist(ExtlFn fn, ObjList *list)
{
    ObjListIterTmp tmp;

    objlist_iter_init(&tmp, list);

    return extl_iter_objlist_(fn, (ObjIterator*)objlist_iter, &tmp);
}


bool extl_iter_obj(ExtlFn fn, Obj *obj)
{
    bool ret1, ret2=FALSE;

    extl_protect(NULL);

    ret1=extl_call(fn, "o", "b", obj, &ret2);

    extl_unprotect(NULL);

    return (ret1 && ret2);
}


/*}}}*/


/*{{{ Booleans */


bool extl_table_is_bool_set(ExtlTab tab, const char *entry)
{
    bool b;

    if(extl_table_gets_b(tab, entry, &b))
        return b;
    return FALSE;
}


/*}}}*/


/*{{{ Rectangles */


bool extl_table_to_rectangle(ExtlTab tab, WRectangle *rectret)
{
    if(!extl_table_gets_i(tab, "x", &(rectret->x)) ||
       !extl_table_gets_i(tab, "y", &(rectret->y)) ||
       !extl_table_gets_i(tab, "w", &(rectret->w)) ||
       !extl_table_gets_i(tab, "h", &(rectret->h)))
       return FALSE;

    return TRUE;
}


ExtlTab extl_table_from_rectangle(const WRectangle *rect)
{
    ExtlTab tab=extl_create_table();

    extl_table_sets_i(tab, "x", rect->x);
    extl_table_sets_i(tab, "y", rect->y);
    extl_table_sets_i(tab, "w", rect->w);
    extl_table_sets_i(tab, "h", rect->h);

    return tab;
}


void extl_table_sets_rectangle(ExtlTab tab, const char *nam,
                               const WRectangle *rect)
{
    ExtlTab g=extl_table_from_rectangle(rect);
    extl_table_sets_t(tab, nam, g);
    extl_unref_table(g);
}


bool extl_table_gets_rectangle(ExtlTab tab, const char *nam,
                               WRectangle *rect)
{
    ExtlTab g;
    bool ok;

    if(!extl_table_gets_t(tab, nam, &g))
        return FALSE;

    ok=extl_table_to_rectangle(g, rect);

    extl_unref_table(g);

    return ok;
}


/*}}}*/


/*{{{ Size policy */


bool extl_table_gets_sizepolicy(ExtlTab tab, const char *nam,
                                WSizePolicy *szplcy)
{
    char *tmpstr;
    bool ret=FALSE;

    if(extl_table_gets_s(tab, nam, &tmpstr)){
        ret=string2sizepolicy(tmpstr, szplcy);
        free(tmpstr);
    }

    return ret;
}


/*}}}*/

