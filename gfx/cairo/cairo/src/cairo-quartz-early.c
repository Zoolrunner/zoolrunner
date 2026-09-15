/* Included by cairo-quartz2-surface.c; distributed under its MPL/LGPL terms.
 *
 * Software backing for the original Quartz API. Application paint callbacks
 * provide a new canvas and a native damage clip. Cairo owns the pixels for the
 * lifetime of this surface, including across flushes; presentation copies the
 * completed canvas through that clip. No unavailable framebuffer readback API
 * is used. Offscreen surfaces use exactly the same image renderer.
 */

typedef struct {
    cairo_surface_t base;
    cairo_image_surface_t *image;
    CGContextRef context;
    cairo_bool_t y_grows_down;
    cairo_bool_t dirty;
} cairo_early_quartz_surface_t;

static void
early_release_pixels (void *info, const void *data, size_t size)
{
    free ((void *) data);
}

static cairo_status_t
early_flush (void *abstract_surface)
{
    cairo_early_quartz_surface_t *surface = abstract_surface;
    cairo_image_surface_t *image = surface->image;
    CGColorSpaceRef colorspace;
    CGDataProviderRef provider;
    CGImageRef snapshot;
    unsigned char *pixels;
    size_t bytes;

    if (!surface->context || !surface->dirty || !image->width || !image->height)
        return CAIRO_STATUS_SUCCESS;
    bytes = (size_t) image->stride * image->height;
    pixels = malloc (bytes);
    if (!pixels)
        return CAIRO_STATUS_NO_MEMORY;
    memcpy (pixels, image->data, bytes);
    provider = CGDataProviderCreateWithData (NULL, pixels, bytes,
                                              early_release_pixels);
    if (!provider) {
        free (pixels);
        return CAIRO_STATUS_NO_MEMORY;
    }
    colorspace = CGColorSpaceCreateDeviceRGB ();
    if (!colorspace) {
        CGDataProviderRelease (provider);
        return CAIRO_STATUS_NO_MEMORY;
    }
    snapshot = CGImageCreate (image->width, image->height, 8, 32,
                              image->stride, colorspace,
                              kCGImageAlphaPremultipliedFirst |
                              kCGBitmapByteOrder32Host,
                              provider, NULL, false,
                              kCGRenderingIntentDefault);
    CGColorSpaceRelease (colorspace);
    CGDataProviderRelease (provider);
    if (!snapshot)
        return CAIRO_STATUS_NO_MEMORY;

    CGContextSaveGState (surface->context);
    /* A CGImage's first row is its upper row. AppKit's flipped view context
     * needs this correction; the unflipped printer context does not. */
    if (!surface->y_grows_down) {
        CGContextTranslateCTM (surface->context, 0, image->height);
        CGContextScaleCTM (surface->context, 1, -1);
    }
    /* Copy is necessary for CLEAR and translucent pixels, and makes repeated
     * flushes idempotent. The caller's native damage clip remains in force. */
    {
        extern void CGContextSetCompositeOperation (CGContextRef, int);
        CGContextSetCompositeOperation (surface->context, 1);
    }
    CGContextDrawImage (surface->context,
                        CGRectMake (0, 0, image->width, image->height), snapshot);
    CGContextRestoreGState (surface->context);
    CGImageRelease (snapshot);
    surface->dirty = FALSE;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
early_finish (void *abstract_surface)
{
    cairo_early_quartz_surface_t *surface = abstract_surface;
    cairo_status_t status = early_flush (surface);
    cairo_surface_destroy (&surface->image->base);
    if (surface->context)
        CGContextRelease (surface->context);
    return status;
}

static cairo_status_t
early_acquire_source (void *abstract_surface, cairo_image_surface_t **image,
                      void **extra)
{
    cairo_early_quartz_surface_t *surface = abstract_surface;
    *image = surface->image;
    *extra = NULL;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
early_extents (void *abstract_surface, cairo_rectangle_t *rect)
{
    cairo_early_quartz_surface_t *surface = abstract_surface;
    rect->x = rect->y = 0;
    rect->width = surface->image->width;
    rect->height = surface->image->height;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
early_acquire_dest (void *abstract_surface, cairo_rectangle_t *interest,
                    cairo_image_surface_t **image, cairo_rectangle_t *rect,
                    void **extra)
{
    early_extents (abstract_surface, rect);
    return early_acquire_source (abstract_surface, image, extra);
}

static void
early_release_dest (void *abstract_surface, cairo_rectangle_t *interest,
                    cairo_image_surface_t *image, cairo_rectangle_t *rect,
                    void *extra)
{
    cairo_early_quartz_surface_t *surface = abstract_surface;
    surface->dirty = TRUE;
}

static cairo_surface_t *
early_similar (void *surface, cairo_content_t content, int width, int height)
{
    return cairo_quartzgl_surface_create (_cairo_format_from_content (content),
                                          width, height, TRUE);
}

static cairo_int_status_t
early_show_glyphs (void *surface, cairo_operator_t op, cairo_pattern_t *source,
                    const cairo_glyph_t *glyphs, int count,
                    cairo_scaled_font_t *font)
{
    cairo_path_fixed_t *path = _cairo_path_fixed_create ();
    cairo_status_t status;
    if (!path)
        return CAIRO_STATUS_NO_MEMORY;
    status = _cairo_scaled_font_glyph_path (font, (cairo_glyph_t *) glyphs,
                                           count, path);
    if (!status)
        status = _cairo_surface_fill (surface, op, source, path,
                                      CAIRO_FILL_RULE_WINDING, 0.1,
                                      font->options.antialias);
    _cairo_path_fixed_destroy (path);
    return status;
}

static const cairo_surface_backend_t early_backend = {
    early_similar, early_finish, early_acquire_source, NULL,
    early_acquire_dest, early_release_dest,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, early_extents,
    NULL, NULL, early_flush, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, early_show_glyphs, NULL
};

static cairo_surface_t *
early_create (cairo_format_t format, unsigned int width, unsigned int height,
               CGContextRef context, cairo_bool_t y_grows_down)
{
    cairo_early_quartz_surface_t *surface;
    cairo_surface_t *image;
    if (width > INT_MAX / 4 || height > INT_MAX ||
        (height && (size_t) width * 4 > (size_t) -1 / height)) {
        _cairo_error (CAIRO_STATUS_NO_MEMORY);
        return (cairo_surface_t *) &_cairo_surface_nil;
    }
    image = cairo_image_surface_create (format, width, height);
    if (cairo_surface_status (image))
        return image;
    surface = calloc (1, sizeof (*surface));
    if (!surface) {
        cairo_surface_destroy (image);
        _cairo_error (CAIRO_STATUS_NO_MEMORY);
        return (cairo_surface_t *) &_cairo_surface_nil;
    }
    _cairo_surface_init (&surface->base, &early_backend);
    surface->image = (cairo_image_surface_t *) image;
    surface->context = context;
    surface->y_grows_down = y_grows_down;
    if (context)
        CGContextRetain (context);
    return &surface->base;
}

cairo_surface_t *
cairo_quartzgl_surface_create (cairo_format_t format, unsigned int width,
                               unsigned int height, cairo_bool_t y_grows_down)
{
    return early_create (format, width, height, NULL, y_grows_down);
}

cairo_surface_t *
cairo_quartzgl_surface_create_for_cg_context (CGContextRef context,
                                              unsigned int width,
                                              unsigned int height,
                                              cairo_bool_t y_grows_down)
{
    if (!context) {
        _cairo_error (CAIRO_STATUS_NULL_POINTER);
        return (cairo_surface_t *) &_cairo_surface_nil;
    }
    return early_create (CAIRO_FORMAT_ARGB32, width, height, context,
                          y_grows_down);
}

#ifdef CAIRO_QUARTZGL_HAS_AGL
cairo_surface_t *
cairo_quartzgl_surface_create_for_agl_context (AGLContext context,
                                               unsigned int width,
                                               unsigned int height,
                                               cairo_bool_t y_grows_down)
{
    /* CGGLContextCreate did not exist in Cheetah. Report a surface error. */
    _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    return (cairo_surface_t *) &_cairo_surface_nil;
}
#endif

cairo_bool_t
cairo_surface_is_quartzgl (cairo_surface_t *surface)
{
    return surface->backend == &early_backend;
}
