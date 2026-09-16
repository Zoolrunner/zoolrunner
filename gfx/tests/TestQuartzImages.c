/* Compare decoded image and toolbar sprite painting with the software renderer. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cairo-quartz2.h"

#define SIDE 32
#define WIDTH 4
#define HEIGHT 12

static void draw(cairo_t *cr, cairo_surface_t *source, int crop, int scale)
{
    int states[] = { 0, 2, 1, 0 };
    int i;
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    for (i = 0; i < (crop ? 4 : 1); ++i) {
        cairo_save(cr);
        cairo_translate(cr, 3, 5);
        cairo_scale(cr, scale, scale);
        cairo_rectangle(cr, 0, 0, WIDTH, crop ? 4 : HEIGHT);
        cairo_clip(cr);
        /* A hover changes which row of the toolbar sprite is displayed. */
        cairo_set_source_surface(cr, source, 0, crop ? -4 * states[i] : 0);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
        cairo_paint(cr);
        cairo_restore(cr);
    }
}

static cairo_surface_t *make_source(int kind, uint32_t *pixels)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    int x, y;
    if (!kind)
        return cairo_image_surface_create_for_data((unsigned char *)pixels,
            CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT, WIDTH * 4);
    surface = cairo_quartzgl_surface_create(CAIRO_FORMAT_ARGB32,
                                            WIDTH, HEIGHT, kind == 1);
    cr = cairo_create(surface);
    for (y = 0; y < HEIGHT; ++y) {
        for (x = 0; x < WIDTH; ++x) {
            uint32_t p = pixels[y * WIDTH + x];
            cairo_set_source_rgb(cr, ((p >> 16) & 255) / 255.,
                                    ((p >> 8) & 255) / 255., (p & 255) / 255.);
            cairo_rectangle(cr, x, y, 1, 1);
            cairo_fill(cr);
        }
    }
    cairo_destroy(cr);
    return surface;
}

int main(void)
{
    uint32_t pixels[WIDTH * HEIGHT], actual[SIDE * SIDE], expected[SIDE * SIDE];
    int kind, flipped, crop, scale, x, y, i, cases = 0, failures = 0;
    for (y = 0; y < HEIGHT; ++y)
        for (x = 0; x < WIDTH; ++x)
            pixels[y * WIDTH + x] = 0xff000000U | ((y + 1) * 16U << 16) |
                                   ((x + 1) * 48U << 8) | (HEIGHT - y) * 16U;
    for (kind = 0; kind < 3; ++kind)
      for (flipped = 0; flipped < 2; ++flipped)
       for (crop = 0; crop < 2; ++crop)
        for (scale = 1; scale <= 2; ++scale) {
            CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
            CGContextRef cg;
            cairo_surface_t *dst, *ref, *source, *refsource;
            cairo_t *cr;
            int differences = 0;
            memset(actual, 0, sizeof(actual));
            memset(expected, 0, sizeof(expected));
            cg = CGBitmapContextCreate(actual, SIDE, SIDE, 8, SIDE * 4, cs,
                kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
            CGColorSpaceRelease(cs);
            if (!cg) return 2;
            /* Use the same sampling on both renderers so this checks row
             * orientation and cropping, rather than interpolation kernels. */
            CGContextSetInterpolationQuality(cg, kCGInterpolationNone);
            if (flipped) {
                CGContextTranslateCTM(cg, 0, SIDE);
                CGContextScaleCTM(cg, 1, -1);
            }
            dst = cairo_quartzgl_surface_create_for_cg_context(cg, SIDE, SIDE, !flipped);
            ref = cairo_image_surface_create_for_data((unsigned char *)expected,
                CAIRO_FORMAT_ARGB32, SIDE, SIDE, SIDE * 4);
            source = make_source(kind, pixels);
            refsource = make_source(0, pixels);
            cr = cairo_create(ref);
            draw(cr, refsource, crop, scale);
            if (cairo_status(cr)) return 3;
            cairo_destroy(cr);
            cr = cairo_create(dst);
            draw(cr, source, crop, scale);
            if (cairo_status(cr)) return 4;
            cairo_surface_flush(dst);
            CGContextFlush(cg);
            for (i = 0; i < sizeof(actual); ++i)
                if (abs((int)((unsigned char *)actual)[i] -
                        (int)((unsigned char *)expected)[i]) > 1) ++differences;
            printf("Quartz image source=%d flipped=%d crop=%d scale=%d differing bytes=%d\n",
                   kind, flipped, crop, scale, differences);
            failures += differences != 0;
            ++cases;
            cairo_destroy(cr);
            cairo_surface_destroy(source);
            cairo_surface_destroy(refsource);
            cairo_surface_destroy(dst);
            cairo_surface_destroy(ref);
            CGContextRelease(cg);
        }
    printf("QUARTZ-IMAGES cases=%d failures=%d\n", cases, failures);
    return failures ? 1 : 0;
}
