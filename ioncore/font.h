/*
 * ion/ioncore/font.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FONT_H
#define ION_IONCORE_FONT_H

#include <libtu/types.h>
#include <X11/Xlib.h>

#ifdef CF_UTF8
#ifdef CF_LIBUTF8
#include <libutf8.h>
#else
#include <wchar.h>
#endif
#endif

#ifdef CF_XFT

#include <X11/Xft/Xft.h>

#define MAX_FONT_WIDTH(X) ((X)->max_advance_width)
#define MAX_FONT_HEIGHT(X) ((X)->height)
#define FONT_BASELINE(X) ((X)->ascent)

#define free_font XftFontClose
extern int text_width(WFontPtr font, const char *str, int len);

#else /* !CF_XFT */

#ifdef CF_UTF8

extern int fontset_height(XFontSet fntset);
extern int fontset_max_width(XFontSet fntset);
extern int fontset_text_width(XFontSet fntset, const char *str, int len);
extern int fontset_baseline(XFontSet fntset);

#define MAX_FONT_WIDTH(X) fontset_max_width(X)
#define MAX_FONT_HEIGHT(X) fontset_height(X)
#define FONT_BASELINE(X) fontset_baseline(X)
/*((X)->max_bounds.ascent+(X)->max_bounds.descent)*/

#define free_font XFreeFontSet
#define text_width fontset_text_width

#else /* !CF_UTF8 */

#define MAX_FONT_WIDTH(X) ((X)->max_bounds.width)
#define MAX_FONT_HEIGHT(X) ((X)->ascent+(X)->descent)
#define FONT_BASELINE(X) ((X)->ascent)

#define free_font XFreeFont
#define text_width XTextWidth

#endif /* !CF_UTF8 */
#endif /* !CF_XFT */

extern WFontPtr load_font(Display *dpy, const char *fontname);
extern char *make_label(WFontPtr fnt, const char *str, int maxw);
extern bool add_shortenrule(const char *rx, const char *rule);

extern int str_nextoff(const char *p);
extern int str_prevoff(const char *p, int pos);

#ifdef CF_UTF8
extern wchar_t str_wchar_at(char *p, int max);
#endif

#endif /* ION_IONCORE_FONT_H */
