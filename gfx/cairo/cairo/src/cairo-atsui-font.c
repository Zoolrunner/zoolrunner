/* cairo - a vector graphics library with display and print output
 *
 * Copyright Â© 2004 Calum Robinson
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Calum Robinson
 *
 * Contributor(s):
 *  Calum Robinson <calumr@mac.com>
 */

#include <stdlib.h>
#include <math.h>
#include "cairo-atsui.h"
#include "cairoint.h"
#include "cairo.h"
#include "cairo-quartz-private.h"
#include "ATSUICompat.h"
#include "QuartzCompat.h"

/*
 * FixedToFloat/FloatToFixed are 10.3+ SDK items - include definitions
 * here so we can use older SDKs.
 */
#ifndef FixedToFloat
#define fixed1              ((Fixed) 0x00010000L)
#define FixedToFloat(a)     ((float)(a) / fixed1)
#define FloatToFixed(a)     ((Fixed)((float)(a) * fixed1))
#endif

typedef struct _cairo_atsui_font_face cairo_atsui_font_face_t;
typedef struct _cairo_atsui_font cairo_atsui_font_t;

static cairo_status_t _cairo_atsui_font_create_scaled (cairo_font_face_t *font_face,
						       ATSUFontID font_id,
						       ATSUStyle style,
						       const cairo_matrix_t *font_matrix,
						       const cairo_matrix_t *ctm,
						       const cairo_font_options_t *options,
						       cairo_scaled_font_t **font_out);

#if defined(__LP64__)
static cairo_status_t _cairo_atsui_font_create_scaled_cg
    (cairo_font_face_t *font_face, CGFontRef cg_font,
     const cairo_matrix_t *font_matrix, const cairo_matrix_t *ctm,
     const cairo_font_options_t *options, cairo_scaled_font_t **font_out);
#endif

struct _cairo_atsui_font {
    cairo_scaled_font_t base;

    ATSUStyle style;
    ATSUStyle unscaled_style;
    ATSUFontID fontID;
#if defined(__LP64__)
    CGFontRef cg_font;
#endif
};


struct _cairo_atsui_font_face {
  cairo_font_face_t base;
  ATSUFontID font_id;
#if defined(__LP64__)
  CGFontRef cg_font;
#endif
};

static void
_cairo_atsui_font_face_destroy (void *abstract_face)
{
#if defined(__LP64__)
    cairo_atsui_font_face_t *font_face = abstract_face;
    if (font_face->cg_font)
        CGFontRelease (font_face->cg_font);
#endif
}

static cairo_status_t
_cairo_atsui_font_face_scaled_font_create (void	*abstract_face,
					   const cairo_matrix_t	*font_matrix,
					   const cairo_matrix_t	*ctm,
					   const cairo_font_options_t *options,
					   cairo_scaled_font_t **font)
{
    cairo_atsui_font_face_t *font_face = abstract_face;
#if defined(__LP64__)
    if (font_face->cg_font)
        return _cairo_atsui_font_create_scaled_cg
            (&font_face->base, font_face->cg_font, font_matrix, ctm, options,
             font);
#endif
    OSStatus err;
    ATSUAttributeTag styleTags[] = { kATSUFontTag };
    ATSUAttributeValuePtr styleValues[] = { &font_face->font_id };
    ByteCount styleSizes[] = {  sizeof(ATSUFontID) };
    ATSUStyle style;

    err = ATSUCreateStyle (&style);
    err = ATSUSetAttributes(style,
                            sizeof(styleTags) / sizeof(styleTags[0]),
                            styleTags, styleSizes, styleValues);

    return _cairo_atsui_font_create_scaled (&font_face->base, font_face->font_id, style,
					    font_matrix, ctm, options, font);
}

static const cairo_font_face_backend_t _cairo_atsui_font_face_backend = {
    _cairo_atsui_font_face_destroy,
    _cairo_atsui_font_face_scaled_font_create
};

cairo_public cairo_font_face_t *
cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id)
{
  cairo_atsui_font_face_t *font_face;

  font_face = malloc (sizeof (cairo_atsui_font_face_t));
  if (!font_face) {
    _cairo_error (CAIRO_STATUS_NO_MEMORY);
    return (cairo_font_face_t *)&_cairo_font_face_nil;
  }

  font_face->font_id = font_id;
#if defined(__LP64__)
  font_face->cg_font = NULL;
#endif

    _cairo_font_face_init (&font_face->base, &_cairo_atsui_font_face_backend);

    return &font_face->base;
}

#if defined(__LP64__)
cairo_public cairo_font_face_t *
cairo_atsui_font_face_create_for_cgfont (CGFontRef cg_font)
{
    cairo_atsui_font_face_t *font_face;

    if (!cg_font)
        return (cairo_font_face_t *)&_cairo_font_face_nil;

    font_face = malloc (sizeof (cairo_atsui_font_face_t));
    if (!font_face) {
        _cairo_error (CAIRO_STATUS_NO_MEMORY);
        return (cairo_font_face_t *)&_cairo_font_face_nil;
    }

    font_face->font_id = kATSUInvalidFontID;
    font_face->cg_font = CGFontRetain (cg_font);
    _cairo_font_face_init (&font_face->base, &_cairo_atsui_font_face_backend);
    return &font_face->base;
}
#endif



static CGAffineTransform
CGAffineTransformMakeWithCairoFontScale(const cairo_matrix_t *scale)
{
    return CGAffineTransformMake(scale->xx, scale->yx,
                                 scale->xy, scale->yy,
                                 0, 0);
}

static ATSUStyle
CreateSizedCopyOfStyle(ATSUStyle inStyle, const cairo_matrix_t *scale)
{
    ATSUStyle style;
    OSStatus err;

    // Set the style's size
    CGAffineTransform theTransform =
        CGAffineTransformMakeWithCairoFontScale(scale);
    Fixed theSize =
        FloatToFixed(CGSizeApplyAffineTransform
                     (CGSizeMake(1.0, 1.0), theTransform).height);
    const ATSUAttributeTag theFontStyleTags[] = { kATSUSizeTag };
    const ByteCount theFontStyleSizes[] = { sizeof(Fixed) };
    ATSUAttributeValuePtr theFontStyleValues[] = { &theSize };

    err = ATSUCreateAndCopyStyle(inStyle, &style);

    err = ATSUSetAttributes(style,
                            sizeof(theFontStyleTags) /
                            sizeof(ATSUAttributeTag), theFontStyleTags,
                            theFontStyleSizes, theFontStyleValues);

    return style;
}

static cairo_status_t
_cairo_atsui_font_set_metrics (cairo_atsui_font_t *font)
{
    ATSFontRef atsFont;
    ATSFontMetrics metrics;
    OSStatus err;

    atsFont = FMGetATSFontRefFromFont(font->fontID);

    if (atsFont) {
        err =
            ATSFontGetHorizontalMetrics(atsFont, kATSOptionFlagsDefault,
                                        &metrics);

        if (err == noErr) {
	    	cairo_font_extents_t extents;
	    
            extents.ascent = metrics.ascent;
            extents.descent = -metrics.descent;
            extents.height = metrics.capHeight;
            extents.max_x_advance = metrics.maxAdvanceWidth;

            // The FT backend doesn't handle max_y_advance either, so we'll ignore it for now. 
            extents.max_y_advance = 0.0;

	    	_cairo_scaled_font_set_metrics (&font->base, &extents);

            return CAIRO_STATUS_SUCCESS;
        }
    }

    return CAIRO_STATUS_NULL_POINTER;
}

#if defined(__LP64__)
static cairo_status_t
_cairo_atsui_font_create_scaled_cg (cairo_font_face_t *font_face,
                                    CGFontRef cg_font,
                                    const cairo_matrix_t *font_matrix,
                                    const cairo_matrix_t *ctm,
                                    const cairo_font_options_t *options,
                                    cairo_scaled_font_t **font_out)
{
    cairo_atsui_font_t *font;
    cairo_font_extents_t extents;
    int units;

    font = calloc (1, sizeof (cairo_atsui_font_t));
    if (!font)
        return CAIRO_STATUS_NO_MEMORY;

    _cairo_scaled_font_init (&font->base, font_face, font_matrix, ctm, options,
                             &cairo_atsui_scaled_font_backend);
    font->fontID = kATSUInvalidFontID;
    font->cg_font = CGFontRetain (cg_font);
    units = CGFontGetUnitsPerEm (cg_font);
    if (units <= 0)
        units = 1000;

    extents.ascent = (double) CGFontGetAscent (cg_font) / units;
    extents.descent = -(double) CGFontGetDescent (cg_font) / units;
    extents.height = ((double) CGFontGetAscent (cg_font) -
                      CGFontGetDescent (cg_font) +
                      CGFontGetLeading (cg_font)) / units;
    extents.max_x_advance = CGFontGetFontBBox (cg_font).size.width / units;
    extents.max_y_advance = 0.0;
    _cairo_scaled_font_set_metrics (&font->base, &extents);

    *font_out = &font->base;
    return CAIRO_STATUS_SUCCESS;
}
#endif

static cairo_status_t
_cairo_atsui_font_create_scaled (cairo_font_face_t *font_face,
				 ATSUFontID font_id,
				 ATSUStyle style,
				 const cairo_matrix_t *font_matrix,
				 const cairo_matrix_t *ctm,
				 const cairo_font_options_t *options,
				 cairo_scaled_font_t **font_out)
{
    cairo_atsui_font_t *font = NULL;
    OSStatus err;
    cairo_status_t status;

    font = malloc(sizeof(cairo_atsui_font_t));

    _cairo_scaled_font_init(&font->base, font_face, font_matrix, ctm, options,
			    &cairo_atsui_scaled_font_backend);

    font->style = CreateSizedCopyOfStyle(style, &font->base.scale);

    Fixed theSize = FloatToFixed(1.0);
    const ATSUAttributeTag theFontStyleTags[] = { kATSUSizeTag };
    const ByteCount theFontStyleSizes[] = { sizeof(Fixed) };
    ATSUAttributeValuePtr theFontStyleValues[] = { &theSize };
    err = ATSUSetAttributes(style,
                            sizeof(theFontStyleTags) /
                            sizeof(ATSUAttributeTag), theFontStyleTags,
                            theFontStyleSizes, theFontStyleValues);

    font->unscaled_style = style;

    font->fontID = font_id;

    *font_out = &font->base;

    status = _cairo_atsui_font_set_metrics (font);
    if (status) {
	cairo_scaled_font_destroy (&font->base);
	return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_atsui_font_create_toy(cairo_toy_font_face_t *toy_face,
			     const cairo_matrix_t *font_matrix,
			     const cairo_matrix_t *ctm,
			     const cairo_font_options_t *options,
			     cairo_scaled_font_t **font_out)
{
#if defined(__LP64__)
    CFStringRef familyName;
    CGFontRef cg_font;
    cairo_status_t status;

    familyName = CFStringCreateWithCString (kCFAllocatorDefault,
                                             toy_face->family,
                                             kCFStringEncodingUTF8);
    if (!familyName)
        return CAIRO_STATUS_NO_MEMORY;
    cg_font = CGFontCreateWithFontName (familyName);
    CFRelease (familyName);
    if (!cg_font)
        return CAIRO_STATUS_NULL_POINTER;
    status = _cairo_atsui_font_create_scaled_cg
        (&toy_face->base, cg_font, font_matrix, ctm, options, font_out);
    CGFontRelease (cg_font);
    return status;
#else
    ATSUStyle style;
    ATSUFontID fontID;
    OSStatus err;
    Boolean isItalic, isBold;
    const char *family = toy_face->family;

    err = ATSUCreateStyle(&style);

    switch (toy_face->weight) {
    case CAIRO_FONT_WEIGHT_BOLD:
        isBold = true;
        break;
    case CAIRO_FONT_WEIGHT_NORMAL:
    default:
        isBold = false;
        break;
    }

    switch (toy_face->slant) {
    case CAIRO_FONT_SLANT_ITALIC:
        isItalic = true;
        break;
    case CAIRO_FONT_SLANT_OBLIQUE:
        isItalic = false;
        break;
    case CAIRO_FONT_SLANT_NORMAL:
    default:
        isItalic = false;
        break;
    }

    err = ATSUFindFontFromName(family, strlen(family),
                               kFontFamilyName,
                               kFontNoPlatformCode,
                               kFontRomanScript,
                               kFontNoLanguageCode, &fontID);

    if (err != noErr) {
	// couldn't get the font - remap css names and try again

	if (!strcmp(family, "serif"))
	    family = "Times";
	else if (!strcmp(family, "sans-serif"))
	    family = "Helvetica";
	else if (!strcmp(family, "cursive"))
	    family = "Apple Chancery";
	else if (!strcmp(family, "fantasy"))
	    family = "Gadget";
	else if (!strcmp(family, "monospace"))
	    family = "Courier";
	else // anything else - return error instead?
	    family = "Courier";

	err = ATSUFindFontFromName(family, strlen(family),
				   kFontFamilyName,
				   kFontNoPlatformCode,
				   kFontRomanScript,
				   kFontNoLanguageCode, &fontID);
    }

    /* Need to specify more tags, e.g. kATSUHangingInhibitFactorTag */
    /* Should use ATSU Variations to exactly specify weight and slant */
    ATSUAttributeTag styleTags[] =
        { kATSUQDItalicTag, kATSUQDBoldfaceTag, kATSUFontTag };
    ATSUAttributeValuePtr styleValues[] =
        { &isItalic, &isBold, &fontID };
    ByteCount styleSizes[] =
        { sizeof(Boolean), sizeof(Boolean), sizeof(ATSUFontID) };

    err = ATSUSetAttributes(style,
                            sizeof(styleTags) / sizeof(styleTags[0]),
                            styleTags, styleSizes, styleValues);

    return _cairo_atsui_font_create_scaled (&toy_face->base, fontID, style,
					    font_matrix, ctm, options, font_out);
#endif
}

static void
_cairo_atsui_font_fini(void *abstract_font)
{
    cairo_atsui_font_t *font = abstract_font;

    if (font == NULL)
        return;

#if defined(__LP64__)
    if (font->cg_font)
        CGFontRelease (font->cg_font);
#endif
    if (font->style)
        ATSUDisposeStyle(font->style);
    if (font->unscaled_style)
        ATSUDisposeStyle(font->unscaled_style);
}

cairo_bool_t
_cairo_scaled_font_is_atsui (cairo_scaled_font_t *sfont)
{
    return (sfont->backend == &cairo_atsui_scaled_font_backend);
}

ATSUStyle
_cairo_atsui_scaled_font_get_atsu_style (cairo_scaled_font_t *sfont)
{
    cairo_atsui_font_t *afont = (cairo_atsui_font_t *) sfont;

    return afont->style;
}

ATSUFontID
_cairo_atsui_scaled_font_get_atsu_font_id (cairo_scaled_font_t *sfont)
{
    cairo_atsui_font_t *afont = (cairo_atsui_font_t *) sfont;

    return afont->fontID;
}

#if defined(__LP64__)
CGFontRef
_cairo_atsui_scaled_font_get_cgfont (cairo_scaled_font_t *sfont)
{
    cairo_atsui_font_t *afont = (cairo_atsui_font_t *) sfont;
    return afont->cg_font;
}
#endif

static cairo_status_t
_cairo_atsui_font_init_glyph_metrics (cairo_atsui_font_t *font,
				      cairo_scaled_glyph_t *scaled_glyph)
{
#if defined(__LP64__)
   cairo_text_extents_t extents;
   CGGlyph glyph = (CGGlyph) _cairo_scaled_glyph_index (scaled_glyph);
   int advance = 0;
   CGRect bounds;
   int units = CGFontGetUnitsPerEm (font->cg_font);
   if (units <= 0)
       units = 1000;
   if (!CGFontGetGlyphAdvances (font->cg_font, &glyph, 1, &advance))
       advance = 0;
   bounds = CGRectZero;
   CGFontGetGlyphBBoxes (font->cg_font, &glyph, 1, &bounds);
   extents.x_bearing = bounds.origin.x / units;
   extents.y_bearing = -(bounds.origin.y + bounds.size.height) / units;
   extents.width = bounds.size.width / units;
   extents.height = bounds.size.height / units;
   extents.x_advance = (double) advance / units;
   extents.y_advance = 0.0;
   _cairo_scaled_glyph_set_metrics (scaled_glyph, &font->base, &extents);
   return CAIRO_STATUS_SUCCESS;
#else
   cairo_text_extents_t extents;
   OSStatus err;
   GlyphID theGlyph = _cairo_scaled_glyph_index (scaled_glyph);
   ATSGlyphIdealMetrics metricsH, metricsV;
   ATSUStyle style;
   ATSUVerticalCharacterType verticalType = kATSUStronglyVertical;
   const ATSUAttributeTag theTag[] = { kATSUVerticalCharacterTag };
   const ByteCount theSizes[] = { sizeof(verticalType) };
   ATSUAttributeValuePtr theValues[] = { &verticalType };

   ATSUCreateAndCopyStyle(font->unscaled_style, &style);

   err = ATSUGlyphGetIdealMetrics(style,
				  1, &theGlyph, 0, &metricsH);
   err = ATSUSetAttributes(style, 1, theTag, theSizes, theValues);
   err = ATSUGlyphGetIdealMetrics(style,
				  1, &theGlyph, 0, &metricsV);

   extents.x_bearing = metricsH.sideBearing.x;
   extents.y_bearing = metricsV.advance.y;
   extents.width = 
      metricsH.advance.x - metricsH.sideBearing.x - metricsH.otherSideBearing.x;
   extents.height = 
     -metricsV.advance.y - metricsV.sideBearing.y - metricsV.otherSideBearing.y;
   extents.x_advance = metricsH.advance.x;
   extents.y_advance = 0;

  _cairo_scaled_glyph_set_metrics (scaled_glyph,
				   &font->base,
				   &extents);

  return CAIRO_STATUS_SUCCESS;
#endif
}

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
typedef struct {
    cairo_path_fixed_t *path;
    cairo_matrix_t scale;
    OSStatus callback_error;
} early_atsui_outline_t;
#endif

static cairo_path_fixed_t *
_outline_path (void *closure)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    return ((early_atsui_outline_t *) closure)->path;
#else
    return closure;
#endif
}

static cairo_point_t
_outline_point (const Float32Point *point, void *closure)
{
    double x = point->x, y = point->y;
    cairo_point_t result;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    /* ATSUI outlines already use QuickDraw's downward-positive coordinates,
     * as does Cairo. Transform the unhinted unit outline into device space. */
    x /= 1000.0;
    y /= 1000.0;
    cairo_matrix_transform_point (&((early_atsui_outline_t *) closure)->scale,
                                   &x, &y);
#endif
    result.x = _cairo_fixed_from_double (x);
    result.y = _cairo_fixed_from_double (y);
    return result;
}

static OSStatus
_outline_result (void *closure, cairo_status_t status)
{
    OSStatus result = status ? memFullErr : noErr;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    /* 10.0 ATSUI writes an indeterminate oCallbackResult even when every
     * callback succeeds. Keep our own error, including allocation failures. */
    early_atsui_outline_t *outline = closure;
    if (result != noErr)
        outline->callback_error = result;
#endif
    return result;
}

static OSStatus
_move_to (const Float32Point *point, void *closure)
{
    cairo_point_t p = _outline_point (point, closure);
    cairo_path_fixed_t *path = _outline_path (closure);
    if (_cairo_path_fixed_close_path (path) ||
        _cairo_path_fixed_move_to (path, p.x, p.y))
        return _outline_result (closure, CAIRO_STATUS_NO_MEMORY);
    return noErr;
}

static OSStatus
_line_to (const Float32Point *point, void *closure)
{
    cairo_point_t p = _outline_point (point, closure);
    return _outline_result (closure,
        _cairo_path_fixed_line_to (_outline_path (closure), p.x, p.y));
}

static OSStatus
_curve_to (const Float32Point *point1, const Float32Point *point2,
           const Float32Point *point3, void *closure)
{
    cairo_point_t a = _outline_point (point1, closure);
    cairo_point_t b = _outline_point (point2, closure);
    cairo_point_t c = _outline_point (point3, closure);
    return _outline_result (closure,
        _cairo_path_fixed_curve_to (_outline_path (closure), a.x, a.y,
                                   b.x, b.y, c.x, c.y));
}

static OSStatus
_close_path (void *closure)
{
    return _outline_result (closure,
        _cairo_path_fixed_close_path (_outline_path (closure)));
}

static cairo_status_t 
_cairo_atsui_scaled_font_init_glyph_path (cairo_atsui_font_t *scaled_font,
					  cairo_scaled_glyph_t *scaled_glyph)
{
#if defined(__LP64__)
    /* Quartz surfaces consume CGGlyphs directly and do not require outlines. */
    return CAIRO_INT_STATUS_UNSUPPORTED;
#else
    static ATSCubicMoveToUPP moveProc = NULL;
    static ATSCubicLineToUPP lineProc = NULL;
    static ATSCubicCurveToUPP curveProc = NULL;
    static ATSCubicClosePathUPP closePathProc = NULL;
    OSStatus err, callback_error = noErr;
    cairo_path_fixed_t *path;
    void *closure;
    ATSUStyle outline_style = scaled_font->style;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    early_atsui_outline_t outline;
#endif

    path = _cairo_path_fixed_create ();
    if (!path)
	return CAIRO_STATUS_NO_MEMORY;

    if (moveProc == NULL) {
        moveProc = NewATSCubicMoveToUPP(_move_to);
        lineProc = NewATSCubicLineToUPP(_line_to);
        curveProc = NewATSCubicCurveToUPP(_curve_to);
        closePathProc = NewATSCubicClosePathUPP(_close_path);
    }

    closure = path;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    {
        /* ATSUI Programming Guide, "Retrieving and Drawing Glyph Outlines":
         * request large outlines and scale down to avoid one-point hinting.
         * The earliest ATSUI does not reliably disable it with options=0. */
        Fixed size = FloatToFixed(1000);
        ATSUAttributeTag tag = kATSUSizeTag;
        ByteCount bytes = sizeof(size);
        ATSUAttributeValuePtr value = &size;
        outline_style = NULL;
        err = ATSUCreateAndCopyStyle(scaled_font->unscaled_style, &outline_style);
        if (err == noErr)
            err = ATSUSetAttributes(outline_style, 1, &tag, &bytes, &value);
        if (err != noErr) {
            if (outline_style)
                ATSUDisposeStyle(outline_style);
            _cairo_path_fixed_destroy(path);
            return err == memFullErr ? CAIRO_STATUS_NO_MEMORY :
                CAIRO_INT_STATUS_UNSUPPORTED;
        }
    }
    outline.path = path;
    outline.scale = scaled_font->base.scale;
    /* Positioned glyphs already include the CTM translation. The font cache
     * deliberately ignores it, so it must not enter the cached outline.
     * Retain any translation explicitly supplied by the font matrix. */
    outline.scale.x0 -= scaled_font->base.ctm.x0;
    outline.scale.y0 -= scaled_font->base.ctm.y0;
    outline.callback_error = noErr;
    closure = &outline;
#endif
    err = ATSUGlyphGetCubicPaths(outline_style,
				 _cairo_scaled_glyph_index (scaled_glyph),
				 moveProc,
				 lineProc,
				 curveProc,
				 closePathProc, closure, &callback_error);
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    ATSUDisposeStyle(outline_style);
    callback_error = outline.callback_error;
#endif
    if (err != noErr || callback_error != noErr) {
        _cairo_path_fixed_destroy(path);
        return callback_error == memFullErr ? CAIRO_STATUS_NO_MEMORY :
            CAIRO_INT_STATUS_UNSUPPORTED;
    }

    _cairo_scaled_glyph_set_path (scaled_glyph, &scaled_font->base, path);

    return CAIRO_STATUS_SUCCESS;
#endif
}

static cairo_status_t
_cairo_atsui_font_scaled_glyph_init (void			*abstract_font,
				     cairo_scaled_glyph_t	*scaled_glyph,
				     cairo_scaled_glyph_info_t	 info)
{
    cairo_atsui_font_t *scaled_font = abstract_font;
    cairo_status_t status;

    if ((info & CAIRO_SCALED_GLYPH_INFO_METRICS) != 0) {
      status = _cairo_atsui_font_init_glyph_metrics (scaled_font, scaled_glyph);
      if (status)
	return status;
    }

    if ((info & CAIRO_SCALED_GLYPH_INFO_PATH) != 0) {
	status = _cairo_atsui_scaled_font_init_glyph_path (scaled_font, scaled_glyph);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t 
_cairo_atsui_font_text_to_glyphs (void		*abstract_font,
				  double	 x,
				  double	 y,
				  const char	*utf8,
				  cairo_glyph_t **glyphs, 
				  int		*num_glyphs)
{
#if defined(__LP64__)
    /* CoreText shaping is performed by gfxAtsuiTextRun on this target. */
    return CAIRO_INT_STATUS_UNSUPPORTED;
#else
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    uint16_t *utf16;
    int n16;
    OSStatus err;
    ATSUTextLayout textLayout;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    ATSUGlyphInfoArray *info;
#else
    ATSLayoutRecord *layoutRecords;
#endif
    cairo_atsui_font_t *font = abstract_font;
    ItemCount glyphCount;
    int i;

    status = _cairo_utf8_to_utf16 ((unsigned char *)utf8, -1, &utf16, &n16);
    if (status)
	return status;

    err = ATSUCreateTextLayout(&textLayout);

    err = ATSUSetTextPointerLocation(textLayout, utf16, 0, n16, n16);

    // Set the style for all of the text
    err = ATSUSetRunStyle(textLayout,
			  font->style, kATSUFromTextBeginning, kATSUToTextEnd);

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
    info = zr_ATSUCopyGlyphInfo(textLayout, n16);
    if (!info) {
        free(utf16);
        ATSUDisposeTextLayout(textLayout);
        return CAIRO_STATUS_NO_MEMORY;
    }
    glyphCount = info->numGlyphs;
    *num_glyphs = 0;
    *glyphs = malloc((glyphCount ? glyphCount : 1) * sizeof(cairo_glyph_t));
    if (!*glyphs) {
        free(info);
        free(utf16);
        ATSUDisposeTextLayout(textLayout);
        return CAIRO_STATUS_NO_MEMORY;
    }
    for (i = 0; i < glyphCount; ++i) {
        /* ATSUI uses 0xffff for deleted glyphs/end-of-line markers. */
        if (info->glyphs[i].glyphID == 0xffff)
            continue;
        (*glyphs)[*num_glyphs].index = info->glyphs[i].glyphID;
        (*glyphs)[*num_glyphs].x = x + info->glyphs[i].idealX;
        (*glyphs)[*num_glyphs].y = y - info->glyphs[i].deltaY;
        ++*num_glyphs;
    }
    free(info);
    free(utf16);
#else
    err = ATSUDirectGetLayoutDataArrayPtrFromTextLayout(textLayout,
							0,
							kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
							(void *)&layoutRecords,
							&glyphCount);

    *num_glyphs = glyphCount - 1;
    *glyphs = 
	(cairo_glyph_t *) malloc(*num_glyphs * (sizeof (cairo_glyph_t)));
    if (*glyphs == NULL) {
	return CAIRO_STATUS_NO_MEMORY;
    }

    for (i = 0; i < *num_glyphs; i++) {
	(*glyphs)[i].index = layoutRecords[i].glyphID;
	(*glyphs)[i].x = x + FixedToFloat(layoutRecords[i].realPos);
	(*glyphs)[i].y = y;
    }

    free (utf16);

    ATSUDirectReleaseLayoutDataArrayPtr(NULL, 
					kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
					(void *) &layoutRecords);
#endif
    ATSUDisposeTextLayout(textLayout);
    
    return CAIRO_STATUS_SUCCESS;
#endif
}

static cairo_int_status_t 
_cairo_atsui_font_old_show_glyphs (void		       *abstract_font,
				   cairo_operator_t    	op,
				   cairo_pattern_t     *pattern,
				   cairo_surface_t     *generic_surface,
				   int                 	source_x,
				   int                 	source_y,
				   int			dest_x,
				   int			dest_y,
				   unsigned int		width,
				   unsigned int		height,
				   const cairo_glyph_t *glyphs,
				   int                 	num_glyphs)
{
    cairo_atsui_font_t *font = abstract_font;
    CGContextRef myBitmapContext;
    CGColorSpaceRef colorSpace;
    cairo_image_surface_t *destImageSurface;
    int i, bits_per_comp, alpha;
    void *extra = NULL;

    cairo_rectangle_t rect = {dest_x, dest_y, width, height};
    _cairo_surface_acquire_dest_image(generic_surface,
				      &rect,
				      &destImageSurface,
				      &rect,
				      &extra);

    // Create a CGBitmapContext for the dest surface for drawing into
    if (destImageSurface->depth == 1) {
        colorSpace = CGColorSpaceCreateDeviceGray();
        bits_per_comp = 1;
        alpha = kCGImageAlphaNone;
    } else if (destImageSurface->depth == 8) {
        colorSpace = CGColorSpaceCreateDeviceGray();
        bits_per_comp = 8;
        alpha = kCGImageAlphaNone;
    } else if (destImageSurface->depth == 24) {
        colorSpace = CGColorSpaceCreateDeviceRGB();
        bits_per_comp = 8;
        alpha = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
    } else if (destImageSurface->depth == 32) {
        colorSpace = CGColorSpaceCreateDeviceRGB();
        bits_per_comp = 8;
        alpha = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
    } else {
        // not reached
        return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    myBitmapContext = CGBitmapContextCreate(destImageSurface->data,
                                            destImageSurface->width,
                                            destImageSurface->height,
                                            bits_per_comp,
                                            destImageSurface->stride,
                                            colorSpace,
                                            alpha);
    CGContextTranslateCTM(myBitmapContext, 0, destImageSurface->height);
    CGContextScaleCTM(myBitmapContext, 1.0f, -1.0f);

#if defined(__LP64__)
    CGFontRef cgFont = font->cg_font;
#else
    ATSFontRef atsFont = FMGetATSFontRefFromFont(font->fontID);
    CGFontRef cgFont = CGFontCreateWithPlatformFont(&atsFont);
#endif

    CGContextSetFont(myBitmapContext, cgFont);

    CGAffineTransform textTransform =
        CGAffineTransformMakeWithCairoFontScale(&font->base.scale);

    textTransform = CGAffineTransformScale(textTransform, 1.0f, -1.0f);

    CGContextSetFontSize(myBitmapContext, 1.0);
    CGContextSetTextMatrix(myBitmapContext, textTransform);

    if (pattern->type == CAIRO_PATTERN_SOLID &&
	_cairo_pattern_is_opaque_solid(pattern))
    {
	cairo_solid_pattern_t *solid = (cairo_solid_pattern_t *)pattern;
	CGContextSetRGBFillColor(myBitmapContext,
				 solid->color.red,
				 solid->color.green,
				 solid->color.blue, 1.0f);
    } else {
	CGContextSetRGBFillColor(myBitmapContext, 0.0f, 0.0f, 0.0f, 0.0f);
    }

	if (_cairo_surface_is_quartz (generic_surface)) {
		cairo_quartz_surface_t *surface = (cairo_quartz_surface_t *)generic_surface;
		if (surface->clip_region) {
			pixman_box16_t *boxes = pixman_region_rects (surface->clip_region);
			int num_boxes = pixman_region_num_rects (surface->clip_region);
			CGRect stack_rects[10];
			CGRect *rects;
			int i;
			
			if (num_boxes > 10)
				rects = malloc (sizeof (CGRect) * num_boxes);
			else
				rects = stack_rects;
				
			for (i = 0; i < num_boxes; i++) {
				rects[i].origin.x = boxes[i].x1;
				rects[i].origin.y = boxes[i].y1;
				rects[i].size.width = boxes[i].x2 - boxes[i].x1;
				rects[i].size.height = boxes[i].y2 - boxes[i].y1;
			}
			
			CGContextClipToRects (myBitmapContext, rects, num_boxes);
			
			if (rects != stack_rects)
				free(rects);
		}
	} else {
		/* XXX: Need to get the text clipped */
	}
	
    // TODO - bold and italic text
    //
    // We could draw the text using ATSUI and get bold, italics
    // etc. for free, but ATSUI does a lot of text layout work
    // that we don't really need...

	
    for (i = 0; i < num_glyphs; i++) {
        CGGlyph theGlyph = glyphs[i].index;
		
        CGContextShowGlyphsAtPoint(myBitmapContext,
				   glyphs[i].x,
                                   glyphs[i].y,
                                   &theGlyph, 1);
    }


    CGColorSpaceRelease(colorSpace);
    CGContextRelease(myBitmapContext);

    _cairo_surface_release_dest_image(generic_surface,
				      &rect,
				      destImageSurface,
				      &rect,
				      extra);

    return CAIRO_STATUS_SUCCESS;
}

const cairo_scaled_font_backend_t cairo_atsui_scaled_font_backend = {
    _cairo_atsui_font_create_toy,
    _cairo_atsui_font_fini,
    _cairo_atsui_font_scaled_glyph_init,
    _cairo_atsui_font_text_to_glyphs,
    NULL, /* ucs4_to_index */
    _cairo_atsui_font_old_show_glyphs,
};
