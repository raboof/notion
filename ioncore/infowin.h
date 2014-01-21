/*
 * ion/ioncore/infowin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_INFOWIN_H
#define ION_IONCORE_INFOWIN_H

#include "common.h"
#include "window.h"
#include "gr.h"
#include "rectangle.h"

#define INFOWIN_BUFFER_LEN 256

DECLCLASS(WInfoWin){
    WWindow wwin;
    GrBrush *brush;
    char *buffer;
    char *style;
    GrStyleSpec attr;
};

#define INFOWIN_BRUSH(INFOWIN) ((INFOWIN)->brush)
#define INFOWIN_BUFFER(INFOWIN) ((INFOWIN)->buffer)

extern bool infowin_init(WInfoWin *p, WWindow *parent, const WFitParams *fp,
                         const char *style);
extern WInfoWin *create_infowin(WWindow *parent, const WFitParams *fp,
                                const char *style);

extern void infowin_deinit(WInfoWin *p);

extern void infowin_set_text(WInfoWin *p, const char *s, int maxw);
extern GrStyleSpec *infowin_stylespec(WInfoWin *p);

extern WRegion *infowin_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

extern void infowin_updategr(WInfoWin *p);

#endif /* ION_IONCORE_INFOWIN_H */
