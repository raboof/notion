/*
 * wmcore/font.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_FONT_H
#define WMCORE_FONT_H

#include <libtu/types.h>
#include <X11/Xlib.h>

#ifdef CF_XFT

#include <X11/Xft/Xft.h>

#define FONT_HEIGHT(X) ((X)->height)
#define MAX_FONT_WIDTH(X) ((X)->max_advance_width)
#define MAX_FONT_HEIGHT(X) ((X)->height)

#define free_font XftFontClose
extern int text_width(WFont *font, const char *str, int len);

#else

#define FONT_HEIGHT(X) ((X)->ascent+(X)->descent)
#define MAX_FONT_WIDTH(X) ((X)->max_bounds.width)
#define MAX_FONT_HEIGHT(X) ((X)->max_bounds.ascent+(X)->max_bounds.descent)

#define free_font XFreeFont
#define text_width XTextWidth

#endif

#define FONT_BASELINE(X) ((X)->ascent)

extern WFont *load_font(Display *dpy, const char *fontname);
extern char *make_label(WFont *fnt, const char *str, int maxw);
extern bool add_shortenrule(const char *rx, const char *rule);

#endif /* WMCORE_FONT_H */
