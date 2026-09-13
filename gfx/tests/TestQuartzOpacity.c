/* Native bitmap regression for the bundled Quartz Cairo opacity path.
 * Build/run instructions: README-quartz-opacity.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cairo-quartz2.h"

#define SIDE 128
static unsigned char actual[SIDE * SIDE * 4];
static unsigned char expected[SIDE * SIDE * 4];

static void rect(cairo_t *cr, double x, double y, double w, double h)
{
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
}

static int render(unsigned char *data, int flipped, int reference, int nested, int offset)
{
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef cg = CGBitmapContextCreate(data, SIDE, SIDE, 8, SIDE * 4,
        space, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    cairo_surface_t *surface;
    cairo_t *cr;
    int status;
    CGColorSpaceRelease(space);
    if (!cg) return 1;
    surface = cairo_quartzgl_surface_create_for_cg_context(cg, SIDE, SIDE, flipped);
    if (offset) cairo_surface_set_device_offset(surface, 9, 13);
    cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_translate(cr, 7, 11);
    cairo_rectangle(cr, 3, 5, 80, 90);
    cairo_clip(cr);
    if (!reference) {
        cairo_push_group(cr);
        if (nested) cairo_push_group(cr);
    }
    cairo_set_source_rgb(cr, 1, reference ? (nested ? .75 : .5) : 0,
                               reference ? (nested ? .75 : .5) : 0);
    rect(cr, 0, 0, 40, 40);
    rect(cr, 20, 20, 80, 80);
    if (!reference) {
        if (nested) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, .5);
        }
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, .5);
    }
    status = cairo_status(cr);
    CGContextFlush(cg);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    CGContextRelease(cg);
    return status;
}

int main(void)
{
    int flipped, nested, offset, failed = 0, cases = 0;
    size_t i;
    cairo_surface_t *oversized;
    for (flipped = 0; flipped < 2; ++flipped) {
        for (nested = 0; nested < 2; ++nested) {
          for (offset = 0; offset < 2; ++offset) {
            int differences = 0;
            memset(actual, 0, sizeof actual);
            memset(expected, 0, sizeof expected);
            if (render(actual, flipped, 0, nested, offset) ||
                render(expected, flipped, 1, nested, offset)) return 2;
            for (i = 0; i < sizeof actual; ++i)
                if (abs((int)actual[i] - (int)expected[i]) > 1) ++differences;
            printf("Quartz opacity flipped=%d nested=%d offset=%d differing bytes=%d\n",
                   flipped, nested, offset, differences);
            if (differences) ++failed;
            ++cases;
          }
        }
    }
    oversized = cairo_quartzgl_surface_create(CAIRO_FORMAT_ARGB32,
                                               ~0U, 2, 0);
    if (cairo_surface_status(oversized) == CAIRO_STATUS_SUCCESS) ++failed;
    cairo_surface_destroy(oversized);
    ++cases;
    printf("QUARTZ-OPACITY cases=%d failures=%d\n", cases, failed);
    return failed ? 1 : 0;
}
