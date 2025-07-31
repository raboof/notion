/*
 * notion/de/font.c
 *
 * Copyright (c) the Notion team 2013.
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/log.h>
#include "font.h"
#ifdef HAVE_X11_XFT
#include <fontconfig/fontconfig.h>
#endif /* HAVE_X11_XFT */
#ifdef HAVE_X11_BMF
#include "fontset.h"
#endif /* HAVE_X11_BMF */
#include "brush.h"

/*{{{ Load/free */


static DEFont *fonts=NULL;


const char *de_default_fontname()
{
#ifdef HAVE_X11_XFT
        return "xft:sans-serif:size=12";
#else
    if(ioncore_g.use_mb)
        return "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*";
    else
        return "fixed";
#endif
}

DEFont *de_load_font(const char *fontname)
{
    DEFont *fnt;
    const char *default_fontname=de_default_fontname();
#ifdef HAVE_X11_XFT
    XftFont *font=NULL;
#endif
#ifdef HAVE_X11_BMF
    XFontSet fontset=NULL;
    XFontStruct *fontstruct=NULL;
#endif

    assert(fontname!=NULL);

    /* There shouldn't be that many fonts... */
    for(fnt=fonts; fnt!=NULL; fnt=fnt->next){
        if(strcmp(fnt->pattern, fontname)==0){
            fnt->refcount++;
            return fnt;
        }
    }

#ifdef HAVE_X11_XFT
    LOG(DEBUG, FONT, "Loading font %s via XFT", fontname);
    if(strncmp(fontname, "xft:", 4)==0){
        font=XftFontOpenName(ioncore_g.dpy, DefaultScreen(ioncore_g.dpy), fontname+4);
    }else{
#ifdef HAVE_X11_BMF
        goto bitmap_font;
#else
        font=XftFontOpenXlfd(ioncore_g.dpy, DefaultScreen(ioncore_g.dpy), fontname);
#endif
    }

    if(font==NULL){
        if(strcmp(fontname, default_fontname)!=0){
            warn(TR("Could not load font \"%s\", trying \"%s\""),
                fontname, default_fontname);
            fnt=de_load_font(default_fontname);
            if(fnt==NULL)
                LOG(WARN, FONT, TR("Failed to load fallback font."));
            return fnt;
        }
        return NULL;
    }else{
        /* FcPatternPrint(font->pattern); */
        /* If you're thinking of commenting the above in, have a look at the
        FC_DEBUG environment variable supported by Fontconfig. */
    }
#endif /* HAVE_X11_XFT */

#ifdef HAVE_X11_BMF
#ifdef HAVE_X11_XFT
bitmap_font:
    if(font==NULL){
#endif

    if(ioncore_g.use_mb){
        LOG(DEBUG, FONT, "Loading fontset %s", fontname);
        fontset=de_create_font_set(fontname);
        if(fontset!=NULL){
            if(XContextDependentDrawing(fontset)){
                warn(TR("Fontset for font pattern '%s' implements context "
                        "dependent drawing, which is unsupported. Expect "
                        "clutter."), fontname);
            }
        }
    }else{
        LOG(DEBUG, FONT, "Loading fontstruct %s", fontname);
        fontstruct=XLoadQueryFont(ioncore_g.dpy, fontname);
    }

    if(fontstruct==NULL && fontset==NULL){
        if(strcmp(fontname, default_fontname)!=0){
            DEFont *fnt;
            LOG(WARN, FONT, TR("Could not load font \"%s\", trying \"%s\""),
                 fontname, default_fontname);
            fnt=de_load_font(default_fontname);
            if(fnt==NULL)
                LOG(WARN, FONT, TR("Failed to load fallback font."));
            return fnt;
        }
        return NULL;
    }
#ifdef HAVE_X11_XFT
    }
#endif /* HAVE_X11_XFT */
#endif /* HAVE_X11_BMF */

    fnt=ALLOC(DEFont);

    if(fnt==NULL)
        return NULL;

#ifdef HAVE_X11_XFT
    fnt->font=font;
#endif
#ifdef HAVE_X11_BMF
    fnt->fontset=fontset;
    fnt->fontstruct=fontstruct;
#endif
    fnt->pattern=scopy(fontname);
    fnt->next=NULL;
    fnt->prev=NULL;
    fnt->refcount=1;

    LINK_ITEM(fonts, fnt, next, prev);

    return fnt;
}


bool de_set_font_for_style(DEStyle *style, DEFont *font)
{
    if(style->font!=NULL)
        de_free_font(style->font);

    style->font=font;
    font->refcount++;

#ifdef HAVE_X11_BMF
    if(style->font->fontstruct!=NULL){
        XSetFont(ioncore_g.dpy, style->normal_gc,
                 style->font->fontstruct->fid);
    }
#endif /* HAVE_X11_BMF */
    return TRUE;
}


bool de_load_font_for_style(DEStyle *style, const char *fontname)
{
    if(style->font!=NULL)
        de_free_font(style->font);

    style->font=de_load_font(fontname);

    if(style->font==NULL)
        return FALSE;

#ifndef HAVE_X11_XFT
    if(style->font->fontstruct!=NULL){
        XSetFont(ioncore_g.dpy, style->normal_gc,
                 style->font->fontstruct->fid);
    }

#endif /* ! HAVE_X11_XFT */
    return TRUE;
}


void de_free_font(DEFont *font)
{
    if(--font->refcount!=0)
        return;

#ifdef HAVE_X11_XFT
    if(font->font!=NULL)
        XftFontClose(ioncore_g.dpy, font->font);
#endif /* HAVE_X11_XFT */
#if HAVE_X11_BMF
    if(font->fontset!=NULL)
        XFreeFontSet(ioncore_g.dpy, font->fontset);
    if(font->fontstruct!=NULL)
        XFreeFont(ioncore_g.dpy, font->fontstruct);
#endif /* HAVE_X11_BMF */
    if(font->pattern!=NULL)
        free(font->pattern);

    UNLINK_ITEM(fonts, font, next, prev);
    free(font);
}


/*}}}*/


/*{{{ Lengths */


void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte)
{
    if(brush->d->font==NULL){
        DE_RESET_FONT_EXTENTS(fnte);
        return;
    }

    defont_get_font_extents(brush->d->font, fnte);
}


void defont_get_font_extents(DEFont *font, GrFontExtents *fnte)
{
#ifdef HAVE_X11_XFT
    if(font->font!=NULL){
        fnte->max_height=font->font->ascent+font->font->descent;
        fnte->max_width=font->font->max_advance_width;
        fnte->baseline=font->font->ascent;
        return;
    }
#endif /* HAVE_X11_XFT */
#ifdef HAVE_X11_BMF
    if(font->fontset!=NULL){
        XFontSetExtents *ext=XExtentsOfFontSet(font->fontset);
        if(ext==NULL)
            goto fail;
        fnte->max_height=ext->max_logical_extent.height;
        fnte->max_width=ext->max_logical_extent.width;
        fnte->baseline=-ext->max_logical_extent.y;
        return;
    }else if(font->fontstruct!=NULL){
        XFontStruct *fnt=font->fontstruct;
        fnte->max_height=fnt->ascent+fnt->descent;
        fnte->max_width=fnt->max_bounds.width;
        fnte->baseline=fnt->ascent;
        return;
    }
fail:
#endif /* HAVE_X11_BMF */
    DE_RESET_FONT_EXTENTS(fnte);
}


uint debrush_get_text_width(DEBrush *brush, const char *text, uint len)
{
    if(brush->d->font==NULL || text==NULL || len==0)
        return 0;

    return defont_get_text_width(brush->d->font, text, len);
}


uint defont_get_text_width(DEFont *font, const char *text, uint len)
{
#ifdef HAVE_X11_XFT
    if(font->font!=NULL){
        XGlyphInfo extents;
        if(ioncore_g.enc_utf8)
            XftTextExtentsUtf8(ioncore_g.dpy, font->font, (XftChar8*)text, len, &extents);
        else
            XftTextExtents8(ioncore_g.dpy, font->font, (XftChar8*)text, len, &extents);
        return extents.xOff;
    }
#endif /* HAVE_X11_XFT */

#ifdef HAVE_X11_BMF
    if(font->fontset!=NULL){
        XRectangle lext;
#ifdef CF_DE_USE_XUTF8
        if(ioncore_g.enc_utf8)
            Xutf8TextExtents(font->fontset, text, len, NULL, &lext);
        else
#endif
            XmbTextExtents(font->fontset, text, len, NULL, &lext);
        return lext.width;
    }else if(font->fontstruct!=NULL){
        return XTextWidth(font->fontstruct, text, len);
    }
#endif /* HAVE_X11_BMF */
    return 0;
}


/*}}}*/


/*{{{ String drawing */


#ifdef HAVE_X11_XFT
static void debrush_do_draw_string_default_xft(
        DEBrush *brush,
        int x, int y, const char *str,
        int len, bool needfill,
        DEColourGroup *colours)
{
    Window win = brush->win;
    GC gc=brush->d->normal_gc;
    XftDraw *draw;
    XftFont *font;

    if(brush->d->font==NULL)
        return;

    font=brush->d->font->font;
    draw=debrush_get_draw(brush, win);

    if(needfill){
        XGlyphInfo extents;
        if(ioncore_g.enc_utf8){
            XftTextExtentsUtf8(ioncore_g.dpy, font, (XftChar8*)str, len,
                               &extents);
        }else{
            XftTextExtents8(ioncore_g.dpy, font, (XftChar8*)str, len, &extents);
        }
        XftDrawRect(draw, &(colours->bg), x-extents.x, y-extents.y,
                    extents.width+10, extents.height);
    }

    if(ioncore_g.enc_utf8){
        XftDrawStringUtf8(draw, &(colours->fg), font, x, y, (XftChar8*)str,
                          len);
    }else{
        XftDrawString8(draw, &(colours->fg), font, x, y, (XftChar8*)str, len);
    }
}
#endif /* HAVE_X11_XFT */

#ifdef HAVE_X11_BMF
void debrush_do_draw_string_default_bmf(
        DEBrush *brush,
        int x, int y, const char *str,
        int len, bool needfill,
        DEColourGroup *colours)
{
    GC gc=brush->d->normal_gc;

    if(brush->d->font==NULL)
        return;

    XSetForeground(ioncore_g.dpy, gc, PIXEL(colours->fg));

    if(!needfill){
        if(brush->d->font->fontset!=NULL){
#ifdef CF_DE_USE_XUTF8
            if(ioncore_g.enc_utf8)
                Xutf8DrawString(ioncore_g.dpy, brush->win,
                                brush->d->font->fontset,
                                gc, x, y, str, len);
            else
#endif
                XmbDrawString(ioncore_g.dpy, brush->win,
                              brush->d->font->fontset,
                              gc, x, y, str, len);
        }else if(brush->d->font->fontstruct!=NULL){
            XDrawString(ioncore_g.dpy, brush->win, gc, x, y, str, len);
        }
    }else{
        XSetBackground(ioncore_g.dpy, gc, PIXEL(colours->bg));
        if(brush->d->font->fontset!=NULL){
#ifdef CF_DE_USE_XUTF8
            if(ioncore_g.enc_utf8)
                Xutf8DrawImageString(ioncore_g.dpy, brush->win,
                                     brush->d->font->fontset,
                                     gc, x, y, str, len);
            else
#endif
                XmbDrawImageString(ioncore_g.dpy, brush->win,
                                   brush->d->font->fontset,
                                   gc, x, y, str, len);
        }else if(brush->d->font->fontstruct!=NULL){
            XDrawImageString(ioncore_g.dpy, brush->win, gc, x, y, str, len);
        }
    }
}
#endif /* HAVE_X11_BMF */

void debrush_do_draw_string_default(
        DEBrush *brush,
        int x, int y, const char *str,
        int len, bool needfill,
        DEColourGroup *colours)
{
    if (brush->d->font) {
#ifdef HAVE_X11_XFT
        if (brush->d->font->font) {
            debrush_do_draw_string_default_xft(brush, x, y, str, len, needfill, colours);
            return;
        }
#endif
#ifdef HAVE_X11_BMF
        {
            debrush_do_draw_string_default_bmf(brush, x, y, str, len, needfill, colours);
            return;
        }
#endif
    }
}


void debrush_do_draw_string(DEBrush *brush, int x, int y,
                            const char *str, int len, bool needfill,
                            DEColourGroup *colours)
{
    CALL_DYN(debrush_do_draw_string, brush, (brush, x, y, str, len,
                                             needfill, colours));
}


void debrush_draw_string(DEBrush *brush, int x, int y,
                         const char *str, int len, bool needfill)
{
    DEColourGroup *cg=debrush_get_current_colour_group(brush);
    if(cg!=NULL)
        debrush_do_draw_string(brush, x, y, str, len, needfill, cg);
}


/*}}}*/

