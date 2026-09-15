/* Regression: gradient source classification and repeated stroke cleanup.
 * Run using check-quartz-strokes.sh and an existing native Cocoa object tree. */
#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cairo.h"
#include "cairo-quartz2.h"
int main(void) {
    uint32_t pixels[64*64];
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef cg = CGBitmapContextCreate(pixels,64,64,8,64*4,space,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    cairo_surface_t *surface = cairo_quartzgl_surface_create_for_cg_context(cg,64,64,1);
    cairo_t *cr = cairo_create(surface);
    int i;
    assert(cg && cairo_status(cr)==CAIRO_STATUS_SUCCESS);
    for (i=0;i<500;i++) {
        cairo_pattern_t *pattern;
        cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);cairo_paint(cr);
        cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
        pattern=cairo_pattern_create_linear(8,32,56,32);
        cairo_pattern_add_color_stop_rgb(pattern,0,1,0,0);
        cairo_pattern_add_color_stop_rgb(pattern,1,0,0,1);
        cairo_set_source(cr,pattern);
        cairo_set_line_width(cr,8);
        cairo_move_to(cr,8,32);cairo_line_to(cr,56,32);cairo_stroke(cr);
        cairo_pattern_destroy(pattern);
        assert(cairo_status(cr)==CAIRO_STATUS_SUCCESS);
        CGContextFlush(cg);
        assert(((pixels[32*64+12]>>16)&255) > (pixels[32*64+12]&255));
        assert((pixels[32*64+52]&255) > ((pixels[32*64+52]>>16)&255));
    }
    cairo_destroy(cr);cairo_surface_destroy(surface);
    CGContextRelease(cg);CGColorSpaceRelease(space);
    puts("500 gradient strokes passed pixel checks");
    return 0;
}
