# Quartz opacity regression

`TestQuartzOpacity.c` compares native Quartz bitmap output against opaque
reference colors. It tests overlapping children, nested half-opacity groups,
translation, clipping, explicit device offsets, and both Quartz Y-axis
orientations. The eight painting cases must pass within one byte of rounding error. A ninth
case checks that an overflowing bitmap width fails without allocation.
This avoids monitor profiles affecting screenshot color comparisons.

After building the macOS XULRunner target, run from the source root:

```sh
clang -Iobj-zoolrunner-macos-arm64-xulrunner/dist/include/cairo \
  -include config/macos/CarbonCompat.h gfx/tests/TestQuartzOpacity.c \
  obj-zoolrunner-macos-arm64-xulrunner/gfx/cairo/cairo/src/libmozcairo.a \
  obj-zoolrunner-macos-arm64-xulrunner/gfx/cairo/libpixman/src/libmozlibpixman.a \
  -framework Cocoa -framework Carbon -o /tmp/zoolrunner-quartz-opacity
/tmp/zoolrunner-quartz-opacity
```

This optional macOS test uses the bundled libraries already built by the
project. It does not configure a separate Cairo build or add a build system.
The browser-level companion is
[opacity-paint.html](../../layout/html/tests/style/opacity-paint.html), which
also exercises view display lists and text. Inspect scrolling, resize, and
hover-menu repainting in the browser. On-screen colors can differ from the
solid-color references because of display color management.

The view renderer now queries an optional `nsIRenderingContextFilter`
interface. Existing `nsIRenderingContext` and `nsIThebesRenderingContext` IIDs
and method layouts are unchanged; backends without the new interface continue
to use their existing black/white blender. Quartz implements solid alpha masks
for OVER and returns unsupported for other mask/operator combinations so Cairo
can use its fallback. Bitmap allocation checks protect the new group path
against size overflow and failed native context creation.
