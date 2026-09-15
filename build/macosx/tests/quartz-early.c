/* Compare the original-Quartz software bridge with Cairo's image renderer. */
#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cairo.h"
#include "cairo-quartz2.h"
#include "QuartzCompat.h"

#define SIDE 64
static void draw(cairo_t *cr)
{
    cairo_pattern_t *gradient;
    double dashes[] = { 2, 3, 5 };
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_rectangle(cr, 0, 0, 16, 8);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0, 0, 1);
    cairo_rectangle(cr, 40, 48, 24, 16);
    cairo_fill(cr);
    cairo_save(cr);
    cairo_rectangle(cr, 8, 8, 48, 40);
    cairo_clip(cr);
    gradient = cairo_pattern_create_radial(20, 22, 1, 30, 32, 28);
    cairo_pattern_add_color_stop_rgba(gradient, 0, 0, 1, 0, .75);
    cairo_pattern_add_color_stop_rgba(gradient, 1, 1, 0, 1, .25);
    cairo_set_source(cr, gradient);
    cairo_paint(cr);
    cairo_pattern_destroy(gradient);
    cairo_translate(cr, 30, 20);
    cairo_rotate(cr, .3);
    cairo_set_source_rgba(cr, 0, 0, 0, .5);
    cairo_set_dash(cr, dashes, 3, .5);
    cairo_set_line_width(cr, 3);
    cairo_move_to(cr, -25, -10);
    cairo_curve_to(cr, -10, 40, 10, -30, 25, 30);
    cairo_stroke(cr);
    cairo_restore(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle(cr, 24, 54, 4, 4);
    cairo_fill(cr);
    assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
}

int main(void)
{
    uint32_t expected[SIDE*SIDE], actual[SIDE*SIDE], previous[SIDE*SIDE];
    cairo_surface_t *reference = cairo_image_surface_create_for_data(
        (unsigned char *)expected, CAIRO_FORMAT_ARGB32, SIDE, SIDE, SIDE*4);
    cairo_t *cr = cairo_create(reference);
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    int flipped, clipped, i;
    draw(cr);
    cairo_destroy(cr);
    for (flipped = 0; flipped < 2; ++flipped) {
        for (clipped = 0; clipped < 2; ++clipped) {
            CGContextRef cg;
            cairo_surface_t *surface;
            for (i = 0; i < SIDE*SIDE; ++i)
                actual[i] = 0xff2468ac;
            cg = CGBitmapContextCreate(actual, SIDE, SIDE, 8, SIDE*4, space,
                kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
            assert(cg);
            if (flipped) {
                CGContextTranslateCTM(cg, 0, SIDE);
                CGContextScaleCTM(cg, 1, -1);
            }
            if (clipped) {
                CGRect rects[] = { {{0, 0}, {16, SIDE}}, {{40, 0}, {24, SIDE}} };
                CGContextClipToRects(cg, rects, 2);
            }
            surface = cairo_quartzgl_surface_create_for_cg_context(
                cg, SIDE, SIDE, !flipped);
            cr = cairo_create(surface);
            draw(cr);
            cairo_surface_flush(surface);
            assert(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
            for (i = 0; i < SIDE*SIDE; ++i) {
                uint32_t want = clipped && i%SIDE >= 16 && i%SIDE < 40 ?
                    0xff2468ac : expected[i];
                if (actual[i] != want) {
                    fprintf(stderr, "flip=%d clip=%d pixel=%d got=%08x want=%08x\n",
                        flipped, clipped, i, actual[i], want);
                    return 1;
                }
            }
            memcpy(previous, actual, sizeof(actual));
            cairo_surface_flush(surface);
            cairo_destroy(cr);
            cairo_surface_destroy(surface);
            assert(!memcmp(previous, actual, sizeof(actual)));
            CGContextRelease(cg);
        }
    }
    cairo_surface_destroy(reference);
    CGColorSpaceRelease(space);
    puts("Early Quartz: orientation, native damage holes, alpha, clear, gradients and strokes passed");
    return 0;
}
