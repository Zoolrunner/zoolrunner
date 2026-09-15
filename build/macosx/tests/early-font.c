/* Exercise the original ATSUI glyph API and Cairo's early outline drawing. */
#include <Carbon/Carbon.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "ATSUICompat.h"
#include "QuartzCompat.h"
#include "cairo.h"
#include "cairo-quartz2.h"

static float low_y, high_y;
static void point(const Float32Point *p)
{
    if (p->y < low_y) low_y = p->y;
    if (p->y > high_y) high_y = p->y;
}
static OSStatus move_to(const Float32Point *p, void *data)
{ point(p); ++*(int *)data; return noErr; }
static OSStatus line_to(const Float32Point *p, void *data)
{ point(p); ++*(int *)data; return noErr; }
static OSStatus curve_to(const Float32Point *a, const Float32Point *b,
                        const Float32Point *c, void *data)
{ point(a); point(b); point(c); ++*(int *)data; return noErr; }
static OSStatus close_path(void *data)
{ ++*(int *)data; return noErr; }

int main(void)
{
    ATSUFontID font;
    ATSUStyle style;
    ATSUTextLayout layout;
    ATSUGlyphInfoArray *info;
    UniChar text[] = {'A','b','c'};
    Fixed size = FloatToFixed(24);
    ATSUAttributeTag tags[] = {kATSUFontTag, kATSUSizeTag};
    ByteCount sizes[] = {sizeof(font), sizeof(size)};
    ATSUAttributeValuePtr values[] = {&font, &size};
    OSStatus status, callback_status = noErr;
    int callbacks = 0;
    uint32_t pixels[128 * 64];
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef cg = CGBitmapContextCreate(pixels, 128, 64, 8, 128*4,
        space, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    cairo_surface_t *surface;
    cairo_t *cr;
    int x, y, count = 0, top = 64, bottom = -1;
    setbuf(stdout, NULL);
    status = ATSUFindFontFromName("Helvetica", 9, kFontFamilyName,
                                  kFontNoPlatformCode, kFontNoScriptCode,
                                  kFontNoLanguageCode, &font);
    printf("ATSU font lookup: %ld font=%lu ATS=%lu\n", (long)status,
            (unsigned long)font, (unsigned long)FMGetATSFontRefFromFont(font));
    assert(status == noErr);
    assert(ATSUCreateStyle(&style) == noErr);
    assert(ATSUSetAttributes(style, 2, tags, sizes, values) == noErr);
    assert(ATSUCreateTextLayout(&layout) == noErr);
    assert(ATSUSetTextPointerLocation(layout, text, 0, 3, 3) == noErr);
    assert(ATSUSetRunStyle(layout, style, 0, 3) == noErr);
    info = zr_ATSUCopyGlyphInfo(layout, 3);
    assert(info && info->numGlyphs >= 3);
    printf("ATSU glyphs: %lu first=%u x=%g y=%g\n", (unsigned long)info->numGlyphs,
            info->glyphs[0].glyphID, info->glyphs[0].idealX, info->glyphs[0].deltaY);
    status = ATSUGlyphGetCubicPaths(style, info->glyphs[0].glyphID,
             NewATSCubicMoveToUPP(move_to), NewATSCubicLineToUPP(line_to),
             NewATSCubicCurveToUPP(curve_to), NewATSCubicClosePathUPP(close_path),
             &callbacks, &callback_status);
    printf("ATSU outline: status=%ld callback=%ld count=%d\n", (long)status,
            (long)callback_status, callbacks);
    size = FloatToFixed(1);
    assert(ATSUSetAttributes(style, 2, tags, sizes, values) == noErr);
    {
        ATSStyleRenderingOptions rendering = 0;
        ATSUAttributeTag tag = kATSUStyleRenderingOptionsTag;
        ByteCount bytes = sizeof(rendering);
        ATSUAttributeValuePtr value = &rendering;
        assert(ATSUSetAttributes(style, 1, &tag, &bytes, &value) == noErr);
    }
    callback_status = noErr;
    callbacks = 0;
    low_y = 10000;
    high_y = -10000;
    status = ATSUGlyphGetCubicPaths(style, info->glyphs[0].glyphID,
             NewATSCubicMoveToUPP(move_to), NewATSCubicLineToUPP(line_to),
             NewATSCubicCurveToUPP(curve_to), NewATSCubicClosePathUPP(close_path),
             &callbacks, &callback_status);
    printf("ATSU unit outline: status=%ld callback=%ld count=%d\n", (long)status,
            (long)callback_status, callbacks);
    printf("ATSU unit outline y bounds: %g to %g\n", low_y, high_y);
    free(info);
    ATSUDisposeTextLayout(layout);
    ATSUDisposeStyle(style);
    assert(cg);
    surface = cairo_quartzgl_surface_create_for_cg_context(cg, 128, 64, 1);
    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_select_font_face(cr, "Helvetica", CAIRO_FONT_SLANT_NORMAL,
                            CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 24);
    cairo_set_source_rgb(cr, 0, 0, 0);
    /* The font cache ignores CTM translation. Glyph outlines must be relative
     * to their origin; only the positioned glyph receives this translation. */
    cairo_translate(cr, 13, 7);
    cairo_move_to(cr, 8 - 13, 40 - 7);
    cairo_show_text(cr, "Abc 42");
    printf("Cairo text status: %d\n", cairo_status(cr));
    assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
    cairo_surface_flush(surface);
    for (y = 0; y < 64; ++y) {
        for (x = 0; x < 128; ++x) {
            if ((pixels[y*128+x] & 0xffffff) != 0xffffff) {
                ++count;
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
        }
    }
    printf("Cairo glyph ink: pixels=%d top=%d bottom=%d baseline=40\n", count, top, bottom);
    assert(count > 100 && top >= 10 && top < 35 && bottom <= 42);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    CGContextRelease(cg);
    CGColorSpaceRelease(space);
    puts("Original ATSUI shaping and Cairo outline rendering passed");
    return 0;
}
